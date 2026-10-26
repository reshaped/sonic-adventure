#include <ninja.h>
#include <sg_xpt.h>

/* describe the pixels at tex in info. info->texaddr becomes tex, so info
 * points at the pixels rather than a copy of them. info->texsurface takes
 * width and height, and type in the high 16 bits of its Type */
void njSetTextureInfo(NJS_TEXINFO *info, Uint16 *tex, Sint32 type, Sint32 width,
                      Sint32 height)
{
    info->texaddr = tex;
    info->texsurface.nWidth = width;
    info->texsurface.nHeight = height;
    info->texsurface.Type = type << 16;
}

void njSetTextureName(NJS_TEXNAME *texname, void *addr, Uint32 globalIndex,
                      Uint32 attr)
{
    /* @hack redundant: attr tested for NJD_TEXATTR_TYPE_MEMORY with the same
     * store of addr in both arms, which keeps the image's TST R7,R3 at
     * 0x8c65ac82; without it, 6 of 6 words differ */
    if (attr & NJD_TEXATTR_TYPE_MEMORY) {
        texname->filename = addr;
    } else {
        texname->filename = addr;
    }
    texname->attr = attr;
    texname->texaddr = globalIndex;
}
