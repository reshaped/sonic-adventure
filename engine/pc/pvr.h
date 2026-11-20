/* the disc's textures: PVR files alone and in PVMs, decoded to RGBA.
 * pvr_decode takes 16-bit texels of ARGB1555, RGB565, ARGB4444 or YUV422.
 * they lie twiddled, in rows or vector quantized, with or without smaller
 * levels */
#ifndef ENGINE_PC_PVR_H
#define ENGINE_PC_PVR_H

#include <stdint.h>

#include <string>
#include <utility>
#include <vector>

#include "disc.h"

namespace pc {

struct Image {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgba; /* by rows, top first */
};

/* where a PVR lies: at is its PVRT chunk's offset in path's bytes, decoded
 * from PRS where compressed. a global_index of 0 is none. file is path's
 * name id, as pvr_file_id gives it */
struct PvrPlace {
    std::string path;
    bool compressed = false;
    std::string name;
    uint32_t global_index = 0;
    uint32_t at = 0;
    uint32_t file = 0;
};

/* the name id of a file of textures, as sa_name_id gives it for the path
 * in upper case. path lies under a disc's root, as "DIR/A.PVM". a host's
 * spelling of the file doesn't change the id */
uint32_t pvr_file_id(const std::string &path);

class Textures {
public:
    /* index every PVM in dir, then every PVR, then every PRS file that
     * holds a PVM. each kind goes in the order Disc::list gives */
    void index(const Disc &disc, const std::string &dir);
    /* index the textures of the PVM or PVR at path, given decompressed in
     * bytes. compressed says the disc holds it PRS-compressed */
    void add(const std::string &path, bool compressed,
             const std::vector<uint8_t> &bytes);
    /* decode the texture of this global index. among several with the
     * index, the one of this name wins. where none has it, or the index is
     * 0, the first of this name is taken. a lone PVR goes by its file's name
     * in lowercase. file is the pvr_file_id of the only file to take it
     * from, or 0 for any. among files sharing an index and name, the first
     * indexed wins. it returns false, with why, where none is found, its
     * file can't be read or pvr_decode fails */
    bool load(const Disc &disc, uint32_t global_index, const std::string &name,
              uint32_t file, Image &image, std::string &why) const;
    uint32_t count() const { return static_cast<uint32_t>(places_.size()); }

private:
    std::vector<PvrPlace> places_;
    /* the files last used, decompressed and the latest first, up to
     * pvr.cpp's MOST_READ. a PVM's textures load one by one, and a
     * texlist's may take turns between files. each file is read once while
     * it stays here */
    mutable std::vector<std::pair<std::string, std::vector<uint8_t>>> read_;
};

/* the longest side pvr_decode takes */
constexpr uint32_t PVR_SIDE_MOST = 1024;

/* decode the PVRT chunk at bytes[at] into image. on failure it returns
 * false, with why, and leaves image as it was. it fails where the chunk
 * passes bytes or its texels pass its length. it fails on a format or size
 * it doesn't decode. a side is 1 to PVR_SIDE_MOST, and a VQ's at least 2.
 * twiddled and VQ sides are powers of two. a texture with smaller levels is
 * square, and a YUV422 width is even */
bool pvr_decode(const std::vector<uint8_t> &bytes, uint32_t at, Image &image,
                std::string &why);

} // namespace pc

#endif
