/* PNG files of RGBA, 8 bits a channel. their deflate blocks are stored, so the
 * writer needs no zlib */
#ifndef ENGINE_PC_PNG_H
#define ENGINE_PC_PNG_H

#include <stdint.h>

#include <string>
#include <vector>

namespace pc {

bool png_write(const std::string &path, uint32_t width, uint32_t height,
               const std::vector<uint8_t> &rgba);

} // namespace pc

/* the same, for a C caller such as play.c. it returns 0 where it fails */
extern "C" int pc_png_write(const char *path, uint32_t width, uint32_t height,
                            const uint8_t *rgba);

#endif
