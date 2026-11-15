/* xboxtex OUT.pak DIR [-raw] [-list] [-stubs OUT_DIR]: the Xbox's texture
 * pack, from the disc's files under DIR.
 *
 * the console's textures are PVR chunks, "PVRT", loose in .PVR files and in
 * the PVM archives the .PRS files compress. the port's renderer decodes each
 * from video memory as the game first draws it and the host turns it into
 * what the NV2A reads, at the game's cost and without mipmaps. this does
 * both ahead: each chunk's largest level decoded as the renderer decodes
 * it (xbox/port/render.c), its mipmaps, where the console's has them, made
 * anew from it, down to one texel, and all of it stored as the NV2A takes
 * it: DXT1 where every texel is opaque, swizzled A1R5G5B5 where each is
 * opaque or clear, DXT3 else; the 2D textures (two_d) and all under -raw
 * without loss, swizzled R5G6B5 and A4R4G4B4 in DXT's place. the renderer
 * finds a texture by its key, xbox/port/texhash.h's, over the bytes video
 * memory holds, which are the chunk's own.
 *
 * left to the renderer, as they are: palettized textures, whose colors are
 * the palette's at the time, strided ones and YUV and bump maps. the disc
 * has no palettized ones: but for some 485 small-codebook VQ textures, now
 * converted too, the pack holds all of it.
 *
 * -stubs OUT_DIR: each .PRS of a PVM archive or of a texture written again under OUT_DIR, at its path under DIR, with each texture the
 * pack holds cut down to its key (texhash.h's TEXSTUB): the game loads the
 * file as ever, and the port finds the pack's texture by the key it reads
 * there, with no texel read from the disc or hashed. the Xbox's disc then
 * holds these in the Dreamcast files' place */
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "../../port/texhash.h"

static int raw_only, list;
static const char *stubs_dir, *root_dir;

/* PRS, Sega's LZ77: control bits, lowest first; 1 a literal; 01 a long
 * copy, 13 bits of offset and 3 of length, or a byte of length more, and
 * the end at 0, 0; 00 a short copy, 2 bits of length and a byte of offset */
static uint8_t *prs_decode(const uint8_t *in, size_t in_size, size_t *out_size)
{
    size_t cap = in_size * 8 + 4096, n = 0, at = 0;
    uint8_t *out = malloc(cap);
    unsigned control = 0;
    int bits = 0;
#define BIT(to)                                                                                                       \
    do {                                                                                                              \
        if (bits == 0) {                                                                                              \
            if (at >= in_size) goto bad;                                                                              \
            control = in[at++];                                                                                       \
            bits = 8;                                                                                                 \
        }                                                                                                             \
        (to) = control & 1;                                                                                           \
        control >>= 1;                                                                                                \
        bits--;                                                                                                       \
    } while (0)

    for (;;) {
        int bit, offset, length;

        BIT(bit);
        if (bit) {
            if (at >= in_size) goto bad;
            if (n >= cap) out = realloc(out, cap *= 2);
            out[n++] = in[at++];
            continue;
        }
        BIT(bit);
        if (bit) {
            unsigned word;

            if (at + 2 > in_size) goto bad;
            word = in[at] | in[at + 1] << 8;
            at += 2;
            if (word == 0) break;
            offset = (int)(word >> 3) - 8192;
            length = word & 7;
            if (length == 0) {
                if (at >= in_size) goto bad;
                length = in[at++] + 1;
            } else {
                length += 2;
            }
        } else {
            int a, b;

            BIT(a);
            BIT(b);
            length = a * 2 + b + 2;
            if (at >= in_size) goto bad;
            offset = (int)in[at++] - 256;
        }
        if ((size_t)-offset > n) goto bad;
        while (n + (size_t)length > cap) out = realloc(out, cap *= 2);
        while (length-- > 0) {
            out[n] = out[n + offset];
            n++;
        }
    }
    *out_size = n;
    return out;
bad:
    free(out);
    return NULL;
#undef BIT
}

/* the textures */

struct entry {
    struct texhash key;
    uint32_t offset, bytes;
    uint8_t format, log_w, log_h, levels;
};
static struct entry *entries;
static size_t entry_count, entry_cap;
static FILE *pack;
static uint32_t pack_at;
static unsigned counts[5], skipped, repeats, stubbed;

static uint32_t from_16(unsigned format, unsigned t)
{
    unsigned a, r, g, b;

    switch (format) {
    case 0:
        a = (t & 0x8000) ? 255 : 0;
        r = (t >> 10) & 31, g = (t >> 5) & 31, b = t & 31;
        return a << 24 | (r << 3 | r >> 2) << 16 | (g << 3 | g >> 2) << 8 | (b << 3 | b >> 2);
    case 1:
        r = (t >> 11) & 31, g = (t >> 5) & 63, b = t & 31;
        return 0xff000000u | (r << 3 | r >> 2) << 16 | (g << 2 | g >> 4) << 8 | (b << 3 | b >> 2);
    default:
        a = (t >> 12) & 15, r = (t >> 8) & 15, g = (t >> 4) & 15, b = t & 15;
        return (a * 17) << 24 | (r * 17) << 16 | (g * 17) << 8 | (b * 17);
    }
}

/* as render.c's: y's bits lowest, in squares of the smaller side */
static uint32_t twiddle(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    uint32_t side = width < height ? width : height;
    uint32_t block = width < height ? y / side : x / side;
    uint32_t inside = 0, bit;
    uint32_t sx = x % side, sy = y % side;

    for (bit = 0; (1u << bit) < side; bit++) {
        inside |= ((sy >> bit) & 1) << (2 * bit);
        inside |= ((sx >> bit) & 1) << (2 * bit + 1);
    }
    return block * side * side + inside;
}

static unsigned log2_of(unsigned n)
{
    unsigned k = 0;

    while ((1u << k) < n) k++;
    return k;
}

static const uint32_t mip_offsets[11] = {0x00006, 0x00008, 0x00010, 0x00030, 0x000b0, 0x002b0,
                                         0x00ab0, 0x02ab0, 0x0aab0, 0x2aab0, 0xaaab0};
static const uint32_t vq_mip_offsets[11] = {0x00000, 0x00001, 0x00002, 0x00006, 0x00016, 0x00056,
                                            0x00156, 0x00556, 0x01556, 0x05556, 0x15556};

/* the NV2A's swizzle: x's bits lowest, the longer side's rest after */
static uint32_t swizzle(uint32_t x, uint32_t y, unsigned log_w, unsigned log_h)
{
    uint32_t i = 0;
    unsigned bit = 0, k;

    for (k = 0; k < log_w || k < log_h; k++) {
        if (k < log_w) i |= ((x >> k) & 1) << bit++;
        if (k < log_h) i |= ((y >> k) & 1) << bit++;
    }
    return i;
}

/* DXT's color block of a 4 by 4 block, with four colors: the ends along
 * the block's principal axis, each texel the nearest of the four */
static void dxt_color(const uint32_t texels[16], uint8_t out[8])
{
    float mean[3] = {0, 0, 0}, cov[6] = {0, 0, 0, 0, 0, 0}, axis[3] = {0.299F, 0.587F, 0.114F};
    float lo = 1e9F, hi = -1e9F;
    int ends[2][3], colors[4][3];
    unsigned c0, c1, bits = 0;
    int k, j;

    for (k = 0; k < 16; k++) {
        for (j = 0; j < 3; j++) mean[j] += (float)((texels[k] >> (16 - 8 * j)) & 255) / 16.0F;
    }
    for (k = 0; k < 16; k++) {
        float d[3];

        for (j = 0; j < 3; j++) d[j] = (float)((texels[k] >> (16 - 8 * j)) & 255) - mean[j];
        cov[0] += d[0] * d[0], cov[1] += d[0] * d[1], cov[2] += d[0] * d[2];
        cov[3] += d[1] * d[1], cov[4] += d[1] * d[2], cov[5] += d[2] * d[2];
    }
    for (k = 0; k < 8; k++) {
        float x = cov[0] * axis[0] + cov[1] * axis[1] + cov[2] * axis[2];
        float y = cov[1] * axis[0] + cov[3] * axis[1] + cov[4] * axis[2];
        float z = cov[2] * axis[0] + cov[4] * axis[1] + cov[5] * axis[2];
        float size = x * x + y * y + z * z;

        if (size < 1e-6F) break;
        size = 1.0F / __builtin_sqrtf(size);
        axis[0] = x * size, axis[1] = y * size, axis[2] = z * size;
    }
    for (k = 0; k < 16; k++) {
        float t = 0;

        for (j = 0; j < 3; j++) t += ((float)((texels[k] >> (16 - 8 * j)) & 255) - mean[j]) * axis[j];
        if (t < lo) lo = t;
        if (t > hi) hi = t;
    }
    for (j = 0; j < 3; j++) {
        float a = mean[j] + axis[j] * hi, b = mean[j] + axis[j] * lo;

        ends[0][j] = a < 0 ? 0 : a > 255 ? 255 : (int)(a + 0.5F);
        ends[1][j] = b < 0 ? 0 : b > 255 ? 255 : (int)(b + 0.5F);
    }
#define TO565(e) ((unsigned)(((e)[0] * 31 + 127) / 255) << 11 | (unsigned)(((e)[1] * 63 + 127) / 255) << 5 | (unsigned)(((e)[2] * 31 + 127) / 255))
    c0 = TO565(ends[0]);
    c1 = TO565(ends[1]);
    if (c0 < c1) {
        unsigned t = c0;

        c0 = c1;
        c1 = t;
    }
    for (k = 0; k < 2; k++) {
        unsigned c = k ? c1 : c0;
        unsigned r = c >> 11, g = (c >> 5) & 63, b = c & 31;

        colors[k][0] = (int)(r << 3 | r >> 2);
        colors[k][1] = (int)(g << 2 | g >> 4);
        colors[k][2] = (int)(b << 3 | b >> 2);
    }
    for (j = 0; j < 3; j++) {
        colors[2][j] = (2 * colors[0][j] + colors[1][j]) / 3;
        colors[3][j] = (colors[0][j] + 2 * colors[1][j]) / 3;
    }
    /* c0 == c1 would be the three-color mode: every texel takes c0 */
    for (k = 0; k < 16 && c0 != c1; k++) {
        int best = 0, best_d = 1 << 30, i;

        for (i = 0; i < 4; i++) {
            int d = 0;

            for (j = 0; j < 3; j++) {
                int e = (int)((texels[k] >> (16 - 8 * j)) & 255) - colors[i][j];

                d += e * e;
            }
            if (d < best_d) best_d = d, best = i;
        }
        bits |= (unsigned)best << (2 * k);
    }
    out[0] = (uint8_t)c0, out[1] = (uint8_t)(c0 >> 8), out[2] = (uint8_t)c1, out[3] = (uint8_t)(c1 >> 8);
    out[4] = (uint8_t)bits, out[5] = (uint8_t)(bits >> 8), out[6] = (uint8_t)(bits >> 16), out[7] = (uint8_t)(bits >> 24);
}

/* a level in the pack's format, at out: its bytes */
static size_t store_level(const uint32_t *argb, unsigned w, unsigned h, int format, uint8_t *out)
{
    unsigned x, y, k;

    if (format == TEXPACK_DXT1 || format == TEXPACK_DXT3) {
        size_t n = 0;

        for (y = 0; y < h; y += 4) {
            for (x = 0; x < w; x += 4) {
                uint32_t block[16];

                for (k = 0; k < 16; k++) {
                    unsigned bx = x + (k & 3), by = y + (k >> 2);

                    block[k] = argb[(by < h ? by : h - 1) * w + (bx < w ? bx : w - 1)];
                }
                if (format == TEXPACK_DXT3) {
                    for (k = 0; k < 8; k++) {
                        out[n++] = (uint8_t)((block[2 * k] >> 28) | (block[2 * k + 1] >> 28) << 4);
                    }
                }
                dxt_color(block, out + n);
                n += 8;
            }
        }
        return n;
    }
    {
        unsigned log_w = log2_of(w), log_h = log2_of(h);

        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                uint32_t c = argb[y * w + x];
                unsigned a = c >> 24, r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
                unsigned t = format == TEXPACK_R5G6B5     ? (r >> 3) << 11 | (g >> 2) << 5 | b >> 3
                             : format == TEXPACK_A1R5G5B5 ? (a >= 128 ? 0x8000u : 0) | (r >> 3) << 10 | (g >> 3) << 5 | b >> 3
                                                          : (a >> 4) << 12 | (r >> 4) << 8 | (g >> 4) << 4 | b >> 4;
                uint32_t i = swizzle(x, y, log_w, log_h);

                out[2 * i] = (uint8_t)t;
                out[2 * i + 1] = (uint8_t)(t >> 8);
            }
        }
        return (size_t)w * h * 2;
    }
}

/* the next level: each texel the mean of four, or of two where a side is
 * 1, its color weighted by alpha, so that clear texels don't darken it */
static void halve(const uint32_t *in, unsigned w, unsigned h, uint32_t *out)
{
    unsigned ow = w > 1 ? w / 2 : 1, oh = h > 1 ? h / 2 : 1;
    unsigned x, y, k, j;

    for (y = 0; y < oh; y++) {
        for (x = 0; x < ow; x++) {
            unsigned sum[4] = {0, 0, 0, 0}, plain[3] = {0, 0, 0}, n = 0;

            for (k = 0; k < 4; k++) {
                unsigned sx = x * 2 + (k & 1), sy = y * 2 + (k >> 1);
                uint32_t c;
                unsigned a;

                if (sx >= w || sy >= h) continue;
                c = in[sy * w + sx];
                a = c >> 24;
                sum[3] += a;
                for (j = 0; j < 3; j++) {
                    sum[j] += ((c >> (16 - 8 * j)) & 255) * a;
                    plain[j] += (c >> (16 - 8 * j)) & 255;
                }
                n++;
            }
            {
                uint32_t c = ((sum[3] + n / 2) / n) << 24;

                for (j = 0; j < 3; j++) {
                    unsigned v = sum[3] ? (sum[j] + sum[3] / 2) / sum[3] : (plain[j] + n / 2) / n;

                    c |= v << (16 - 8 * j);
                }
                out[y * ow + x] = c;
            }
        }
    }
}

static int entry_order(const void *a, const void *b)
{
    const struct entry *x = a, *y = b;

    return x->key.hi != y->key.hi ? (x->key.hi < y->key.hi ? -1 : 1) : x->key.lo != y->key.lo ? (x->key.lo < y->key.lo ? -1 : 1) : 0;
}

/* the 2D textures, drawn a texel to a pixel: the stage's title cards
 * (?_STAGEnn.PVR), the HUD (CON_*), the menus and their names (AVA_*,
 * TITLE*, TX_*). stored without loss, in the Xbox's own 16-bit formats,
 * as the console's: because DXT's 4 by 4 blocks of two colors blur their
 * lettering and outlines */
static int two_d(const char *from)
{
    const char *name = strrchr(from, '/');

    name = name != NULL ? name + 1 : from;
    if (strlen(name) >= 9 && name[1] == '_' && strncasecmp(name + 2, "STAGE", 5) == 0) {
        return 1;
    }
    return strncasecmp(name, "CON_", 4) == 0 || strncasecmp(name, "AVA_", 4) == 0 ||
           strncasecmp(name, "TITLE", 5) == 0 || strncasecmp(name, "TX_", 3) == 0;
}

/* a PVRT chunk's data: pixel format, data format, sizes */
static int texture(const char *from, const uint8_t *data, size_t size, unsigned pixel, unsigned kind, unsigned w,
                   unsigned h, struct texhash *key_out)
{
    /* a small VQ texture's codebook is shorter, by its size, and its
     * indices follow it as a VQ texture's follow its 2048 bytes */
    static const unsigned small_codebook[7] = {0, 0, 0, 128, 128, 256, 1024};
    static const unsigned small_codebook_mipmap[7] = {0, 0, 0, 128, 128, 512, 2048};
    int small = kind == 0x10 || kind == 0x11;
    int vq = kind == 3 || kind == 4 || small;
    int mip = kind == 2 || kind == 4 || kind == 0x12 || kind == 0x11;
    size_t codebook = 2048;
    int raster = kind == 9;
    unsigned level = log2_of(w);
    size_t start, top;
    struct texhash key;
    uint32_t *argb, *next;
    uint8_t *out;
    size_t n = 0, k;
    unsigned x, y, lw, lh, levels = 0;
    int format, clear = 0, partial = 0;

    if (pixel > 2 ||
        !(kind == 1 || kind == 2 || kind == 3 || kind == 4 || kind == 9 || kind == 0xd || kind == 0x12 || small) ||
        (mip && w != h) || (small && (w != h || level > 6 || small_codebook[level] == 0))) {
        skipped++;
        return 0;
    }
    if (small) {
        codebook = mip ? small_codebook_mipmap[level] : small_codebook[level];
    }
    /* video memory holds the chunk's data as it is, but a mipmapped
     * 16-bit one's four bytes on: the file has two bytes before its levels
     * where video memory has six, and the alias has the six */
    start = mip ? (vq ? codebook + vq_mip_offsets[level] : mip_offsets[level] - (kind == 0x12 ? 0 : 4))
                : vq ? codebook : 0;
    top = vq ? (size_t)w * h / 4 : (size_t)w * h * 2;
    if (start + top > size) {
        skipped++;
        return 0;
    }
    texhash_start(&key, pixel, vq, raster || small, w, h);
    if (vq) texhash_bytes(&key, data, (unsigned)codebook);
    texhash_bytes(&key, data + start, (unsigned)top);
    *key_out = key;
    for (k = 0; k < entry_count; k++) {
        if (entries[k].key.lo == key.lo && entries[k].key.hi == key.hi) {
            repeats++;
            return 1;
        }
    }
    argb = malloc((size_t)w * h * 4);
    next = malloc((size_t)w * h * 4);
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            unsigned t;

            if (vq) {
                unsigned index = data[start + twiddle(x / 2, y / 2, w / 2, h / 2)];
                const uint8_t *e = data + index * 8 + ((x & 1) * 2 + (y & 1)) * 2;

                t = e[0] | e[1] << 8;
            } else {
                const uint8_t *e = data + start + (raster ? y * w + x : twiddle(x, y, w, h)) * 2;

                t = e[0] | e[1] << 8;
            }
            argb[y * w + x] = from_16(pixel, t);
            clear |= argb[y * w + x] >> 24 == 0;
            partial |= argb[y * w + x] >> 24 != 0 && argb[y * w + x] >> 24 != 255;
        }
    }
    {
        int lossless = raw_only || two_d(from);

        format = partial ? (lossless ? TEXPACK_A4R4G4B4 : TEXPACK_DXT3)
                 : clear ? TEXPACK_A1R5G5B5
                         : (lossless ? TEXPACK_R5G6B5 : TEXPACK_DXT1);
    }
    out = malloc((size_t)w * h * 4 + 64);
    lw = w, lh = h;
    for (;;) {
        n += store_level(argb, lw, lh, format, out + n);
        levels++;
        /* mipmaps where the console's texture has them, and only there:
         * made anew from its largest level, as above */
        if (!mip || (lw == 1 && lh == 1)) break;
        halve(argb, lw, lh, next);
        {
            uint32_t *t = argb;

            argb = next;
            next = t;
        }
        lw = lw > 1 ? lw / 2 : 1;
        lh = lh > 1 ? lh / 2 : 1;
    }
    if (entry_count == entry_cap) entries = realloc(entries, (entry_cap = entry_cap ? entry_cap * 2 : 1024) * sizeof *entries);
    entries[entry_count].key = key;
    entries[entry_count].offset = pack_at;
    entries[entry_count].bytes = (uint32_t)n;
    entries[entry_count].format = (uint8_t)format;
    entries[entry_count].log_w = (uint8_t)log2_of(w);
    entries[entry_count].log_h = (uint8_t)log2_of(h);
    entries[entry_count].levels = (uint8_t)levels;
    entry_count++;
    fwrite(out, 1, n, pack);
    pack_at += (uint32_t)n;
    while (pack_at & 3) {
        fputc(0, pack);
        pack_at++;
    }
    counts[format]++;
    if (list) {
        printf("%08x%08x %ux%u pixel %u kind %u format %d %s\n", key.hi, key.lo, w, h, pixel, kind, format, from);
    }
    free(argb);
    free(next);
    free(out);
    return 1;
}

static void scan(const char *from, const uint8_t *d, size_t size)
{
    size_t at;

    for (at = 0; at + 16 <= size; at++) {
        uint32_t length;
        unsigned w, h;

        if (memcmp(d + at, "PVRT", 4) != 0) continue;
        memcpy(&length, d + at + 4, 4);
        w = d[at + 12] | d[at + 13] << 8;
        h = d[at + 14] | d[at + 15] << 8;
        if (length < 8 || length > size - at - 8 || w < 8 || w > 1024 || h < 8 || h > 1024 || (w & (w - 1)) ||
            (h & (h - 1))) {
            continue;
        }
        {
            struct texhash key;

            texture(from, d + at + 16, length - 8, d[at + 8], d[at + 9], w, h, &key);
        }
        at += 7 + length;
    }
}

/* a texture file's chunks, one after the other: a PVM archive's PVMH, then
 * GBIX and PVRT chunks, or a .PVR's own. each PVRT goes to the pack, as
 * scan's, and the file is written again at *stub, malloc'd, with each
 * texture the pack took cut down to texhash.h's TEXSTUB. 0 where the file
 * isn't chunks from end to end, or holds no texture: scan has it then */
static int chunks(const char *from, const uint8_t *d, size_t size, uint8_t **stub, size_t *stub_size)
{
    uint8_t *out = malloc(size + 64);
    size_t at = 0, n = 0, k;
    int textures = 0, cut = 0, pass;

    /* the first pass only checks the file's shape, so that a file scan is
     * to have gives nothing to the pack here */
    for (pass = 0; pass < 2; pass++) {
        at = 0, n = 0;
        while (at + 8 <= size) {
            uint32_t length;

            memcpy(&length, d + at + 4, 4);
            for (k = 0; k < 4 && d[at + k] >= 'A' && d[at + k] <= 'Z'; k++) {
            }
            if (k < 4 || length > size - at - 8) {
                break;
            }
            if (memcmp(d + at, "PVRT", 4) == 0 && length >= 8) {
                unsigned w = d[at + 12] | d[at + 13] << 8, h = d[at + 14] | d[at + 15] << 8;
                struct texhash key;

                textures++;
                if (pass == 1 && w >= 8 && w <= 1024 && h >= 8 && h <= 1024 && !(w & (w - 1)) && !(h & (h - 1)) &&
                    length >= 8 + TEXSTUB_BYTES &&
                    texture(from, d + at + 16, length - 8, d[at + 8], d[at + 9], w, h, &key)) {
                    uint32_t short_length = 8 + TEXSTUB_BYTES;

                    memcpy(out + n, "PVRT", 4);
                    memcpy(out + n + 4, &short_length, 4);
                    memcpy(out + n + 8, d + at + 8, 8);
                    memcpy(out + n + 16, TEXSTUB_MAGIC, 12);
                    memcpy(out + n + 28, &key.lo, 4);
                    memcpy(out + n + 32, &key.hi, 4);
                    n += 16 + TEXSTUB_BYTES;
                    at += 8 + length;
                    cut++;
                    continue;
                }
            }
            memcpy(out + n, d + at, 8 + length);
            n += 8 + length;
            at += 8 + length;
        }
        /* what's left is the last sector's padding, or the file isn't chunks */
        for (k = at; k < size; k++) {
            if (d[k] != 0) {
                free(out);
                return 0;
            }
        }
        if (textures == 0) {
            free(out);
            return 0;
        }
    }
    if (cut == 0) {
        free(out);
        *stub = NULL;
        return 1;
    }
    /* a search for one more chunk past the last reads zeros */
    memset(out + n, 0, 16);
    *stub = out;
    *stub_size = n + 16;
    return 1;
}

/* PRS with literals alone: a control byte of eight 1s before each eight
 * bytes, and the end's 0, 1 and two zero bytes. a stub is a few hundred
 * bytes, which no copy would shorten by much */
static uint8_t *prs_encode(const uint8_t *in, size_t size, size_t *out_size)
{
    uint8_t *out = malloc(size + size / 8 + 8);
    size_t n = 0, at = 0;

    while (size - at >= 8) {
        out[n++] = 0xff;
        memcpy(out + n, in + at, 8);
        n += 8;
        at += 8;
    }
    /* the rest's literals, then the bits 0 and 1 */
    out[n++] = (uint8_t)(((1u << (size - at)) - 1) | 2u << (size - at));
    memcpy(out + n, in + at, size - at);
    n += size - at;
    if (size - at == 7) {
        /* the end's second bit starts a new control byte */
        out[n++] = 1;
    }
    out[n++] = 0;
    out[n++] = 0;
    *out_size = n;
    return out;
}

static void write_stub(const char *path, const uint8_t *d, size_t size)
{
    char *to = malloc(strlen(stubs_dir) + strlen(path) + 2);
    char *slash;
    FILE *f;

    sprintf(to, "%s/%s", stubs_dir, path + strlen(root_dir) + 1);
    for (slash = to + 1; (slash = strchr(slash, '/')) != NULL; slash++) {
        *slash = '\0';
        mkdir(to, 0777);
        *slash = '/';
    }
    f = fopen(to, "wb");
    if (f == NULL) {
        perror(to);
        exit(1);
    }
    fwrite(d, 1, size, f);
    fclose(f);
    free(to);
}

static void walk(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0) return;
    if (S_ISDIR(st.st_mode)) {
        struct dirent **names;
        int n = scandir(path, &names, NULL, alphasort), k;

        for (k = 0; k < n; k++) {
            if (names[k]->d_name[0] != '.') {
                char *child = malloc(strlen(path) + strlen(names[k]->d_name) + 2);

                sprintf(child, "%s/%s", path, names[k]->d_name);
                walk(child);
                free(child);
            }
            free(names[k]);
        }
        if (n >= 0) free(names);
        return;
    }
    {
        const char *dot = strrchr(path, '.');
        FILE *f;
        uint8_t *d;
        size_t size = (size_t)st.st_size;

        if (dot == NULL || (strcasecmp(dot, ".PRS") != 0 && strcasecmp(dot, ".PVR") != 0 && strcasecmp(dot, ".PVM") != 0 &&
                            strcasecmp(dot, ".BIN") != 0)) {
            return;
        }
        f = fopen(path, "rb");
        if (f == NULL) return;
        d = malloc(size + 1);
        size = fread(d, 1, size, f);
        fclose(f);
        if (strcasecmp(dot, ".PRS") == 0) {
            size_t n;
            uint8_t *plain = prs_decode(d, size, &n);

            if (plain != NULL) {
                uint8_t *stub = NULL;
                size_t stub_size = 0;

                if (!chunks(path, plain, n, &stub, &stub_size)) {
                    scan(path, plain, n);
                } else if (stub != NULL && stubs_dir != NULL) {
                    size_t packed_size;
                    uint8_t *packed = prs_encode(stub, stub_size, &packed_size);

                    write_stub(path, packed, packed_size);
                    stubbed++;
                    free(packed);
                }
                free(stub);
                free(plain);
            }
        } else {
            /* a loose .PVR stays whole: Ninja loads a file's texture in
             * parts (kmLoadTexturePart), which the port doesn't meet, and
             * the renderer keys it by video memory's bytes */
            scan(path, d, size);
        }
        free(d);
    }
}

int main(int argc, char **argv)
{
    struct texpack_header header = {TEXPACK_MAGIC, 0};
    char *blobs;
    FILE *out;
    size_t k;
    int a;

    if (argc < 3) {
        fprintf(stderr, "usage: xboxtex OUT.pak DIR [-raw] [-list] [-stubs OUT_DIR]\n");
        return 2;
    }
    for (a = 3; a < argc; a++) {
        raw_only |= strcmp(argv[a], "-raw") == 0;
        list |= strcmp(argv[a], "-list") == 0;
        if (strcmp(argv[a], "-stubs") == 0 && a + 1 < argc) {
            stubs_dir = argv[++a];
        }
    }
    root_dir = argv[2];
    blobs = malloc(strlen(argv[1]) + 8);
    sprintf(blobs, "%s.tmp", argv[1]);
    pack = fopen(blobs, "wb+");
    if (pack == NULL) {
        perror(blobs);
        return 1;
    }
    walk(argv[2]);
    /* the header and the entries, their offsets past both, then the levels */
    qsort(entries, entry_count, sizeof *entries, entry_order);
    out = fopen(argv[1], "wb");
    if (out == NULL) {
        perror(argv[1]);
        return 1;
    }
    header.count = (unsigned)entry_count;
    fwrite(&header, sizeof header, 1, out);
    for (k = 0; k < entry_count; k++) {
        struct texpack_entry e = {entries[k].key.lo, entries[k].key.hi,
                                  entries[k].offset + (unsigned)(sizeof header + entry_count * sizeof e), entries[k].bytes,
                                  entries[k].format, entries[k].log_w, entries[k].log_h, entries[k].levels};

        fwrite(&e, sizeof e, 1, out);
    }
    rewind(pack);
    {
        static char buffer[1 << 16];
        size_t n;

        while ((n = fread(buffer, 1, sizeof buffer, pack)) > 0) fwrite(buffer, 1, n, out);
    }
    fclose(pack);
    remove(blobs);
    fclose(out);
    fprintf(stderr,
            "xboxtex: %zu textures, %u MB: %u DXT1, %u A1R5G5B5, %u DXT3, %u R5G6B5, %u A4R4G4B4; %u repeats, %u left to "
            "the renderer; %u files cut down to their keys\n",
            entry_count, (pack_at + (1u << 19)) >> 20, counts[TEXPACK_DXT1], counts[TEXPACK_A1R5G5B5], counts[TEXPACK_DXT3],
            counts[TEXPACK_R5G6B5], counts[TEXPACK_A4R4G4B4], repeats, skipped, stubbed);
    return 0;
}
