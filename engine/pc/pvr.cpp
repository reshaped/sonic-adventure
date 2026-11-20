#include "pvr.h"

#include <ctype.h>
#include <string.h>

#include <algorithm>

#include "sa_engine.h"

namespace pc {

namespace {

/* the most files read_ holds */
constexpr size_t MOST_READ = 8;

/* the PVR header's pixel formats and data layouts */
enum { ARGB1555 = 0, RGB565 = 1, ARGB4444 = 2, YUV422 = 3 };
enum {
    TWIDDLED = 0x01,
    TWIDDLED_MIPS = 0x02,
    VQ = 0x03,
    VQ_MIPS = 0x04,
    RECTANGLE = 0x09,
    STRIDE = 0x0b,
    RECTANGLE_TWIDDLED = 0x0d,
    SMALL_VQ = 0x10,
    SMALL_VQ_MIPS = 0x11,
    TWIDDLED_MIPS_ALT = 0x12
};
/* a PVMH's status, as Ninja reads it: set bits' fields follow each entry's
 * 2-byte number, in this order */
enum {
    PVMH_PVRNAME = 0x0001, /* a name of PVM_NAME_SIZE bytes */
    PVMH_CATEGORYCODE = 0x0002, /* 2 bytes */
    PVMH_ENTRYINFO = 0x0004, /* 2 bytes */
    PVMH_GLOBALINDEX = 0x0008 /* a 4-byte global index */
};
constexpr uint32_t PVM_NAME_SIZE = 28;

uint32_t fourcc(const char *s)
{
    return static_cast<uint32_t>(s[0]) | static_cast<uint32_t>(s[1]) << 8 |
           static_cast<uint32_t>(s[2]) << 16 |
           static_cast<uint32_t>(s[3]) << 24;
}

uint32_t read16(const uint8_t *at)
{
    return static_cast<uint32_t>(at[0] | at[1] << 8);
}

uint32_t read32(const uint8_t *at)
{
    return read16(at) | read16(at + 2) << 16;
}

bool power_of_two(uint32_t n)
{
    return n != 0 && (n & (n - 1)) == 0;
}

/* a texel's place in a twiddled square of side n. the place's bits
 * alternate y's and x's, the lowest from y */
uint32_t twiddle(uint32_t x, uint32_t y, uint32_t n)
{
    uint32_t at = 0;
    uint32_t bit = 0;

    for (uint32_t m = 1; m < n; m <<= 1) {
        if (y & m) {
            at |= 1U << bit;
        }
        bit++;
        if (x & m) {
            at |= 1U << bit;
        }
        bit++;
    }
    return at;
}

/* a twiddled rectangle is squares of its shorter side, in a row along its
 * longer one */
uint32_t twiddle_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    uint32_t n = w < h ? w : h;
    uint32_t square = w < h ? y / n : x / n;

    return square * n * n + twiddle(x % n, y % n, n);
}

void put(Image &image, uint32_t x, uint32_t y, uint32_t pixel, uint32_t format)
{
    uint8_t *p = &image.rgba[(y * image.width + x) * 4];
    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;
    uint32_t a = 255;

    switch (format) {
    case ARGB1555:
        a = pixel & 0x8000 ? 255 : 0;
        r = (pixel >> 10) & 31;
        g = (pixel >> 5) & 31;
        b = pixel & 31;
        r = r << 3 | r >> 2;
        g = g << 3 | g >> 2;
        b = b << 3 | b >> 2;
        break;
    case RGB565:
        r = (pixel >> 11) & 31;
        g = (pixel >> 5) & 63;
        b = pixel & 31;
        r = r << 3 | r >> 2;
        g = g << 2 | g >> 4;
        b = b << 3 | b >> 2;
        break;
    default: /* ARGB4444 */
        a = (pixel >> 12) & 15;
        r = (pixel >> 8) & 15;
        g = (pixel >> 4) & 15;
        b = pixel & 15;
        a |= a << 4;
        r |= r << 4;
        g |= g << 4;
        b |= b << 4;
        break;
    }
    p[0] = static_cast<uint8_t>(r);
    p[1] = static_cast<uint8_t>(g);
    p[2] = static_cast<uint8_t>(b);
    p[3] = static_cast<uint8_t>(a);
}

uint8_t clamp8(int v)
{
    return static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v);
}

/* the weights of YUV422's U and V in a texel's red, green and blue. each
 * is a multiple of 11/32, where BT.601 has 1.402, 0.344, 0.714 and 1.772 */
constexpr double V_TO_RED = 11.0 / 8;
constexpr double U_TO_GREEN = 11.0 / 32;
constexpr double V_TO_GREEN = 11.0 / 16;
constexpr double U_TO_BLUE = 55.0 / 32;

/* two texels from YUV422's pair of words, U and Y0 then V and Y1 */
void put_yuv(Image &image, uint32_t x, uint32_t y, uint32_t w0, uint32_t w1)
{
    int u = static_cast<int>(w0 & 0xff) - 128;
    int v = static_cast<int>(w1 & 0xff) - 128;
    int ys[2] = {static_cast<int>(w0 >> 8), static_cast<int>(w1 >> 8)};

    for (int i = 0; i < 2; i++) {
        uint8_t *p = &image.rgba[(y * image.width + x + i) * 4];

        p[0] = clamp8(ys[i] + (int)(V_TO_RED * v));
        p[1] = clamp8(ys[i] - (int)(U_TO_GREEN * u) - (int)(V_TO_GREEN * v));
        p[2] = clamp8(ys[i] + (int)(U_TO_BLUE * u));
        p[3] = 255;
    }
}

} // namespace

bool pvr_decode(const std::vector<uint8_t> &bytes, uint32_t at, Image &image,
                std::string &why)
{
    /* a chunk is its name, its length of what follows, 8 bytes of header
     * and its texels */
    size_t size = bytes.size();

    if (at > size || size - at < 16) {
        why = "a PVR header out of bounds";
        return false;
    }
    const uint8_t *chunk = bytes.data() + at;
    uint32_t length = read32(chunk + 4);
    uint32_t pixel = chunk[8];
    uint32_t layout = chunk[9];
    uint32_t w = read16(chunk + 12);
    uint32_t h = read16(chunk + 14);

    if (length < 8 || length > size - at - 8 || w == 0 || h == 0 ||
        w > PVR_SIDE_MOST || h > PVR_SIDE_MOST) {
        why = "a PVR header out of bounds";
        return false;
    }
    if (pixel > YUV422) {
        why = "pixel format " + std::to_string(pixel);
        return false;
    }
    bool small_vq = layout == SMALL_VQ || layout == SMALL_VQ_MIPS;
    bool vq = small_vq || layout == VQ || layout == VQ_MIPS;
    bool mips = layout == TWIDDLED_MIPS || layout == TWIDDLED_MIPS_ALT ||
                layout == VQ_MIPS || layout == SMALL_VQ_MIPS;
    bool rows = layout == RECTANGLE || layout == STRIDE;
    bool twiddled =
        layout == TWIDDLED || layout == RECTANGLE_TWIDDLED || (mips && !vq);

    if (!vq && !rows && !twiddled) {
        why = "data format " + std::to_string(layout);
        return false;
    }
    /* twiddling interleaves the bits of two powers of two. the smaller
     * levels halve a square. a VQ block is 2 by 2 texels, and a YUV422 pair
     * is two texels of a row */
    if ((!rows && (!power_of_two(w) || !power_of_two(h))) || (mips && w != h) ||
        (vq && (w < 2 || h < 2)) || (pixel == YUV422 && w % 2 != 0)) {
        why = "a size of " + std::to_string(w) + " by " + std::to_string(h) +
              ", which its format can't have";
        return false;
    }
    /* where the largest level starts in the texels, and the bytes it
     * takes. the smaller levels lie before it, the smallest first. twiddled,
     * they follow a texel's 2 bytes of padding. largest counts from the
     * front, since a chunk may end in padding. a VQ texture is a codebook
     * of 8-byte codes, each 2 by 2 texels. a byte a block follows, twiddled.
     * a VQ's 1 by 1 level takes a byte too */
    uint32_t codes = 256;
    uint32_t largest = 0;
    uint32_t taken = w * h * 2;

    if (small_vq) {
        codes = w <= 16            ? 16
                : w == 32          ? (mips ? 64 : 32)
                : w == 64 && !mips ? 128
                                   : 256;
    }
    if (vq) {
        largest = codes * 8;
        taken = (w / 2) * (h / 2);
        if (mips) {
            largest += 1;
            for (uint32_t s = 2; s < w; s <<= 1) {
                largest += (s / 2) * (s / 2);
            }
        }
    } else if (mips) {
        largest = 2;
        for (uint32_t s = 1; s < w; s <<= 1) {
            largest += s * s * 2;
        }
    }
    if (largest > length - 8 || taken > length - 8 - largest) {
        why = "a texture's texels past its chunk";
        return false;
    }
    /* every read below lies in the chunk, and every write in the image */
    const uint8_t *data = chunk + 16;
    const uint8_t *level = data + largest;

    image.width = w;
    image.height = h;
    image.rgba.assign(static_cast<size_t>(w) * h * 4, 0);
    if (vq) {
        for (uint32_t y = 0; y < h; y += 2) {
            for (uint32_t x = 0; x < w; x += 2) {
                uint32_t code = level[twiddle_rect(x / 2, y / 2, w / 2, h / 2)];
                const uint8_t *entry = data + (code % codes) * 8;

                if (pixel == YUV422) {
                    put_yuv(image, x, y, read16(entry), read16(entry + 4));
                    put_yuv(image, x, y + 1, read16(entry + 2),
                            read16(entry + 6));
                    continue;
                }
                put(image, x, y, read16(entry), pixel);
                put(image, x, y + 1, read16(entry + 2), pixel);
                put(image, x + 1, y, read16(entry + 4), pixel);
                put(image, x + 1, y + 1, read16(entry + 6), pixel);
            }
        }
        return true;
    }
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t i = twiddled ? twiddle_rect(x, y, w, h) : y * w + x;

            if (pixel != YUV422) {
                put(image, x, y, read16(level + i * 2), pixel);
            } else if ((x & 1) == 0) {
                uint32_t j = twiddled ? twiddle_rect(x + 1, y, w, h) : i + 1;

                put_yuv(image, x, y, read16(level + i * 2),
                        read16(level + j * 2));
            }
        }
    }
    return true;
}

uint32_t pvr_file_id(const std::string &path)
{
    std::string spelled = path;

    for (char &c : spelled) {
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }
    return sa_name_id(spelled.c_str());
}

void Textures::add(const std::string &path, bool compressed,
                   const std::vector<uint8_t> &bytes)
{
    Bytes b{bytes.data(), static_cast<uint32_t>(bytes.size())};
    uint32_t file = pvr_file_id(path);
    std::vector<std::string> names;
    std::vector<uint32_t> indices;
    uint32_t count = 1;
    uint32_t at = 0;

    if (b.u32(0) == fourcc("PVMH")) {
        uint32_t header = b.u32(4);
        uint32_t status = b.u16(8);

        count = b.u16(10);
        at = 12;
        for (uint32_t i = 0; i < count && b.ok; i++) {
            std::string name;

            at += 2;
            if (status & PVMH_PVRNAME) {
                for (uint32_t k = 0; k < PVM_NAME_SIZE && b.u8(at + k) != 0;
                     k++) {
                    name.push_back(static_cast<char>(b.u8(at + k)));
                }
                at += PVM_NAME_SIZE;
            }
            at += (status & PVMH_CATEGORYCODE ? 2 : 0) +
                  (status & PVMH_ENTRYINFO ? 2 : 0);
            indices.push_back(status & PVMH_GLOBALINDEX ? b.u32(at) : 0);
            at += status & PVMH_GLOBALINDEX ? 4 : 0;
            names.push_back(name);
        }
        /* the chunks follow the header. a header that passes the file
         * leaves none, since its end could wrap to the file's start */
        if (b.size < 8 || header > b.size - 8) {
            return;
        }
        at = 8 + header;
    } else if (b.u32(0) == fourcc("GBIX") || b.u32(0) == fourcc("PVRT")) {
        /* a PVR file alone, which a texlist names by its file's name. the
         * texlist may give it a global index as it loads it, as
         * b32ascii_texlist does */
        std::string name = path.substr(path.rfind('/') + 1);
        for (char &c : name) {
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        }
        names.push_back(name);
    } else {
        return;
    }
    /* the chunks: a PVRT for each entry, after a GBIX or not. a GBIX's index
     * wins over the entry's. any other chunk is passed by its length, as
     * njSearchPvrChunk passes it */
    uint32_t gbix = 0;
    uint32_t n = 0;

    while (at <= b.size && b.size - at >= 8 && n < count) {
        uint32_t tag = b.u32(at);
        uint32_t length = b.u32(at + 4);

        if (tag == fourcc("GBIX")) {
            gbix = b.u32(at + 8);
        } else if (tag == fourcc("PVRT")) {
            if (gbix == 0 && n < indices.size()) {
                gbix = indices[n];
            }
            places_.push_back({path, compressed,
                               n < names.size() ? names[n] : "", gbix, at,
                               file});
            n++;
            gbix = 0;
        }
        /* a chunk that passes the file is the last. its end could wrap past
         * 32 bits and turn the walk back */
        if (length > b.size - at - 8) {
            break;
        }
        at += 8 + length;
    }
}

void Textures::index(const Disc &disc, const std::string &dir)
{
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> plain;

    for (const char *suffix : {".PVM", ".PVR", ".PRS"}) {
        for (const std::string &path : disc.list(dir, suffix)) {
            if (!disc.read(path, bytes)) {
                continue;
            }
            if (strcmp(suffix, ".PRS") != 0) {
                add(path, false, bytes);
            } else if (prs_decode(bytes, plain) && plain.size() >= 4 &&
                       memcmp(plain.data(), "PVMH", 4) == 0) {
                add(path, true, plain);
            }
        }
    }
}

bool Textures::load(const Disc &disc, uint32_t global_index,
                    const std::string &name, uint32_t file, Image &image,
                    std::string &why) const
{
    const PvrPlace *found = nullptr;

    for (const PvrPlace &place : places_) {
        if ((file != 0 && place.file != file) ||
            (global_index == 0 ? place.name != name
                               : place.global_index != global_index)) {
            continue;
        }
        if (found == nullptr || (place.name == name && found->name != name)) {
            found = &place;
        }
    }
    /* where the index finds none, as for an index a texlist gave a lone PVR,
     * the name decides */
    for (const PvrPlace &place : places_) {
        if (found == nullptr && !name.empty() && place.name == name &&
            (file == 0 || place.file == file)) {
            found = &place;
        }
    }
    if (found == nullptr) {
        const std::string texture =
            name + ", global index " + std::to_string(global_index);
        auto named = std::find_if(
            places_.begin(), places_.end(),
            [file](const PvrPlace &place) { return place.file == file; });

        if (file == 0) {
            why = "no PVM holds " + texture;
        } else if (named != places_.end()) {
            why = named->path + " doesn't hold " + texture;
        } else {
            why = "no indexed file has the name id " + std::to_string(file) +
                  ", named for " + texture;
        }
        return false;
    }
    auto read =
        std::find_if(read_.begin(), read_.end(),
                     [found](const auto &r) { return r.first == found->path; });

    if (read == read_.end()) {
        std::vector<uint8_t> bytes;
        std::vector<uint8_t> plain;

        if (!disc.read(found->path, bytes) ||
            (found->compressed && !prs_decode(bytes, plain))) {
            why = "can't read " + found->path;
            return false;
        }
        if (read_.size() == MOST_READ) {
            read_.pop_back();
        }
        read_.emplace_back(found->path, found->compressed ? std::move(plain)
                                                          : std::move(bytes));
        read = read_.end() - 1;
    }
    std::rotate(read_.begin(), read, read + 1);
    return pvr_decode(read_.front().second, found->at, image, why);
}

} // namespace pc
