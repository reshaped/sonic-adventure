/* a texture's key in the Xbox's texture pack: what xbox/host/xbox/xboxtex.c gives
 * each texture it converts, from the disc's file, and the renderer gives
 * the same texture in video memory: 64 bits over its pixel format, its
 * kind and its sizes, then the bytes of its largest level, the codebook
 * first for a VQ one. the lengths are multiples of 4 */
#ifndef XBOX_PORT_TEXHASH_H
#define XBOX_PORT_TEXHASH_H

struct texhash {
    unsigned lo, hi;
};

static inline void texhash_word(struct texhash *h, unsigned word)
{
    h->lo = (h->lo ^ word) * 0x01000193u;
    h->hi = (h->hi + word) * 0x9e3779b1u;
    h->hi ^= h->hi >> 15;
}

/* format is the PowerVR's pixel format, 0 to 6; vq and raster (not
 * twiddled) its kind. both at once is a small VQ texture, whose codebook,
 * shorter than 2048 bytes, is hashed at its own length */
static inline void texhash_start(struct texhash *h, unsigned format, int vq, int raster, unsigned width,
                                 unsigned height)
{
    h->lo = 0x811c9dc5u;
    h->hi = 0x2545f491u;
    texhash_word(h, format | (vq ? 8u : 0) | (raster ? 16u : 0) | width << 8 | height << 20);
}

static inline void texhash_bytes(struct texhash *h, const unsigned char *bytes, unsigned count)
{
    unsigned k;

    for (k = 0; k + 4 <= count; k += 4) {
        unsigned word;

        __builtin_memcpy(&word, bytes + k, 4);
        texhash_word(h, word);
    }
}

/* the pack, D:\textures.pak: this header, the entries in the order of
 * their keys, hi then lo, then each texture's levels, the largest first,
 * as the NV2A reads them */
#define TEXPACK_MAGIC 0x31584554u /* "TEX1" */
#define TEXPACK_R5G6B5 0 /* swizzled */
#define TEXPACK_A1R5G5B5 1 /* swizzled */
#define TEXPACK_A4R4G4B4 2 /* swizzled */
#define TEXPACK_DXT1 3
#define TEXPACK_DXT3 4

/* a texture cut down to its key, in the Xbox disc's texture files
 * (xboxtex.c's -stubs): TEXSTUB_MAGIC and the key's lo and hi where the
 * texels were. a load that starts four bytes into a chunk's data, as a
 * mipmapped one's alias does, still meets the magic's last eight bytes
 * before the key */
#define TEXSTUB_MAGIC "XTX0XTEXKEY1"
#define TEXSTUB_BYTES 20

struct texpack_header {
    unsigned magic;
    unsigned count;
};

struct texpack_entry {
    unsigned lo, hi;
    unsigned offset; /* from the file's start */
    unsigned bytes;
    unsigned char format, log_w, log_h, levels;
};

#endif
