/* the disc's files as the PC reads them from its roots. play.c's first root
 * is the build's, build/dc by default, so its files replace the disc's */
#ifndef ENGINE_PC_DISC_H
#define ENGINE_PC_DISC_H

#include <stdint.h>

#include <string>
#include <vector>

namespace pc {

class Disc {
public:
    void add_root(const std::string &root) { roots_.push_back(root); }
    /* the file at path, as "SONICADV/STG01.BIN", under the first root that
     * holds it. it returns false where none does or the read fails */
    bool read(const std::string &path, std::vector<uint8_t> &bytes) const;
    /* each dir/name under the roots whose name ends in suffix, once. the
     * first root's come first, and each root's are sorted by name */
    std::vector<std::string> list(const std::string &dir,
                                  const std::string &suffix) const;

private:
    std::vector<std::string> roots_;
};

/* decode in, compressed by SEGA's PRS, into out. it returns false where in
 * ends before the stream's end, or a copy reaches back before out's start */
bool prs_decode(const std::vector<uint8_t> &in, std::vector<uint8_t> &out);

/* little-endian reads of data's size bytes. a byte past them reads as 0
 * and sets ok to false */
struct Bytes {
    const uint8_t *data = nullptr;
    uint32_t size = 0;
    bool ok = true;

    uint8_t u8(uint32_t at)
    {
        if (at >= size) {
            ok = false;
            return 0;
        }
        return data[at];
    }
    uint16_t u16(uint32_t at)
    {
        return static_cast<uint16_t>(u8(at) | u8(at + 1) << 8);
    }
    uint32_t u32(uint32_t at)
    {
        return static_cast<uint32_t>(u16(at)) |
               static_cast<uint32_t>(u16(at + 2)) << 16;
    }
    float f32(uint32_t at);
};

} // namespace pc

#endif
