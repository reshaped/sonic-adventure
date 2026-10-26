#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

/* the bits cleared for the surface's address, in 8-byte units: the 21-bit
 * address field, bits 0 to 20, and bits 21 and 22, the low end of the palette
 * bank field, which param doesn't hold where the mask clears them */
#define TEX_ADDRESS_BITS 0x007fffff

Uint32 ConvertUVnum(Uint32 size);

/* the TSP size code for a texture side of size pixels: log2(size) - 3, from 0
 * for 8 to 7 for 1024. any other size is returned unchanged */
Uint32 ConvertUVnum(Uint32 size)
{
    if (size == 8) {
        size = 0;
    } else if (size == 16) {
        size = 1;
    } else if (size == 32) {
        size = 2;
    } else if (size == 64) {
        size = 3;
    } else if (size == 128) {
        size = 4;
    } else if (size == 256) {
        size = 5;
    } else if (size == 512) {
        size = 6;
    } else if (size == 1024) {
        size = 7;
    }
    return size;
}

void njSetTextureParam(NJS_TEXMEMLIST *tex)
{
    NJS_TEXSURFACE *surface = &tex->texinfo.texsurface;
    Uint32 width;
    /* @hack redundant: register on param, which builds both words in R4 (OR
     * R0,R4 at 0x8c66aa4c for the first); without it, the second goes through
     * R5 and 32 of 67 words differ */
    register Uint32 param;

    width = ConvertUVnum(surface->nWidth) << 3;
    /* @hack redundant: a second mask of bits already clear, the AND at
     * 0x8c66aa40, which keeps width in R14 across the call; without it, width
     * goes to the frame and 29 of 67 words differ */
    width = width & ~TSP_V_SIZE;
    param = width | ConvertUVnum(surface->nHeight);
    tex->tspparambuffer = param;
    param = (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) << 31;
    param = (param & ~TEX_VQ) |
            ((surface->fSurfaceFlags & KM_SURFACEFLAGS_VQ) << 27);
    param = (param & ~TEX_FORMAT) | (surface->PixelFormat & TEX_FORMAT);
    param = (param & ~TEX_NOT_TWIDDLED) |
            ((~(surface->fSurfaceFlags & KM_SURFACEFLAGS_TWIDDLED) &
              KM_SURFACEFLAGS_TWIDDLED)
             << 24);
    param = (param & ~TEX_STRIDE) |
            ((surface->fSurfaceFlags & KM_SURFACEFLAGS_STRIDE) << 21);
    param = (param & ~TEX_ADDRESS_BITS) | ((Uint32)surface->pSurface >> 3);
    tex->texparambuffer = param;
}
