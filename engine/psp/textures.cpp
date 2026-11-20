/* the disc's textures for the PSP's model walk (draw.h): the walk draws from
 * the game's own records of texture memory, and names none to the client,
 * so it reads a texture here, by the index of the disc's the client's
 * Textures wrote at the first start */
#include <string>

#include "disc.h"
#include "draw.h"
#include "pvr.h"

extern "C" {
/* engine/psp/main.c's: the folder of the disc's files */
extern const char *native_disc;
}

namespace pc {
/* raster.cpp's: the raster's texture of image, or 0, and of a PVR's chunk,
 * with tried false where the chunk's format wants an image made of it */
uint32_t psp_texture_add(const Image &image);
uint32_t psp_texture_add_pvr(const std::vector<uint8_t> &chunk, bool &tried);
} // namespace pc

namespace {

pc::Disc disc;
pc::Textures textures;
bool indexed;
/* a texture's bytes as its file holds them. the room stays from one texture
 * to the next, taken while the heap is whole: a heap long in use has no
 * such stretch free, though it has the bytes. KEPT is the room it starts
 * with, a texture of 256 texels a side at 16 bits each, and the most it
 * keeps */
constexpr size_t KEPT = (256 * 256 * 2) + 4096;
std::vector<uint8_t> chunk;

} // namespace

extern "C" uint32_t psp_texture_load(uint32_t global_index, const char *name,
                                     uint32_t file)
{
    pc::Image image;
    std::string why;

    if (!indexed) {
        disc.add_root(native_disc);
        textures.index(disc, "SONICADV");
        indexed = true;
    }
    bool tried;

    if (chunk.capacity() < KEPT) {
        chunk.reserve(KEPT);
    }
    if (!textures.chunk(disc, global_index, name != nullptr ? name : "", file,
                        chunk, why)) {
        return 0;
    }
    uint32_t made = pc::psp_texture_add_pvr(chunk, tried);

    if (made == 0 && !tried && pc::pvr_decode(chunk, 0, image, why)) {
        made = pc::psp_texture_add(image);
    }
    /* a larger texture's room goes back, and the room kept is taken again
     * at once, from what that left free */
    if (chunk.capacity() > KEPT) {
        std::vector<uint8_t>().swap(chunk);
        chunk.reserve(KEPT);
    }
    return made;
}
