/* texture surfaces and transfers from files, cache entries and memory */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "ninja/njpvr.h"

/* njCheckFileName's extension table, which holds .PVR alone */
/* NOLINTBEGIN(clang-diagnostic-missing-braces) */
char fileext[1][5] = {
#include "ninja/njTexLoad/fileext.inc"
};
/* NOLINTEND(clang-diagnostic-missing-braces) */
/* the bytes of a square texture's mipmap levels at 2 bytes a pixel, from its
 * width down to 1 by 1, plus 2. the index is njCreateTextureSurface's level: 0
 * for a width of 1024, and 7 for 8 */
static Uint32 mipmap_sizes[12] = {
    0x002aaaac, 0x000aaaac, 0x0002aaac, 0x0000aaac, 0x00002aac, 0x00000aac,
    0x000002ac, 0x000000ac, 0x0000002c, 0x0000000c, 0x00000004, 0x00000002};

/* njTexFile's sector buffer */
Sint8 _nj_file_buffer[2048];

struct pvr_file_header {
    Uint8 name[4];
    Uint32 size;
    Uint32 type;
    Uint16 width;
    Uint16 height;
};

/* set *type to a texture type for a pixel depth of bit bits, and return 1.
 * a depth of 16 gives KM_TEXTURE_ARGB1555 pixels, and 32 gives
 * KM_TEXTURE_ARGB4444. depths of 1, 4, 8 and 24 give KM_TEXTURE_RGB565. the
 * type is KM_TEXTURE_RECTANGLE where width and height differ. a square
 * texture is twiddled, with mipmaps where cn is set. any other depth returns
 * -1 */
Sint32 njSetTexInfo(Sint32 *type, Sint32 width, Sint32 height, Sint32 bit,
                    Sint32 cn)
{
    switch (bit) {
    /* @hack spelling: an arm of its own, with the same store, for each of
     * cases 1, 4, 8 and 24, which keeps KM_TEXTURE_RGB565 in R14 from the
     * first test's slot (MOV #1,R14 at 0x8c66b3ee in 1ST_READ.BIN); as one arm
     * for the four, 50 of 51 words differ */
    case 1:
        *type = KM_TEXTURE_RGB565;
        break;
    case 4:
        *type = KM_TEXTURE_RGB565;
        break;
    case 8:
        *type = KM_TEXTURE_RGB565;
        break;
    case 16:
        *type = KM_TEXTURE_ARGB1555;
        break;
    case 24:
        *type = KM_TEXTURE_RGB565;
        break;
    case 32:
        *type = KM_TEXTURE_ARGB4444;
        break;
    default:
        return -1;
    }
    if (width != height) {
        *type |= KM_TEXTURE_RECTANGLE;
    } else if (cn) {
        *type |= KM_TEXTURE_TWIDDLED_MM;
    } else {
        *type |= KM_TEXTURE_TWIDDLED;
    }
    return 1;
}

/* fill surface for a texture of type, width by height. type holds a
 * KM_TEXTURE_ category in its NJD_TEXFMT_TYPE_MASK bits and a KM_TEXTURE_
 * pixel format in its NJD_TEXFMT_COLOR_MASK bits. it returns 1, or -1 for a
 * texture it can't describe. width and height must each be a power of two
 * from 8 to 1024, and a mipmapped texture must be square. the category can't
 * be KM_TEXTURE_TWIDDLED_RECTANGLE or one kamui.h doesn't define. one that
 * isn't palettized takes only ARGB1555, RGB565, ARGB4444, YUV422 and BUMP
 * pixels */
Sint32 njCreateTextureSurface(NJS_TEXSURFACE *surface, Sint32 width,
                              Sint32 height, Sint32 type)
{
    Sint32 category;
    Sint32 format;
    Sint32 level;
    Sint32 bpp;

    category = type & NJD_TEXFMT_TYPE_MASK;
    format = type & NJD_TEXFMT_COLOR_MASK;
    switch (category) {
    case KM_TEXTURE_TWIDDLED:
        surface->fSurfaceFlags = KM_SURFACEFLAGS_TWIDDLED;
        break;
    case KM_TEXTURE_TWIDDLED_MM:
        surface->fSurfaceFlags =
            KM_SURFACEFLAGS_TWIDDLED | KM_SURFACEFLAGS_MIPMAPED;
        break;
    case KM_TEXTURE_VQ:
    case KM_TEXTURE_SMALLVQ:
        surface->fSurfaceFlags = KM_SURFACEFLAGS_VQ;
        break;
    case KM_TEXTURE_VQ_MM:
    case KM_TEXTURE_SMALLVQ_MM:
        surface->fSurfaceFlags = KM_SURFACEFLAGS_VQ | KM_SURFACEFLAGS_MIPMAPED;
        break;
    case KM_TEXTURE_PALETTIZE4:
        surface->fSurfaceFlags = KM_SURFACEFLAGS_PALETTIZED;
        surface->PixelFormat = KM_PIXELFORMAT_PALETTIZED_4BPP;
        break;
    case KM_TEXTURE_PALETTIZE4_MM:
        surface->fSurfaceFlags =
            KM_SURFACEFLAGS_PALETTIZED | KM_SURFACEFLAGS_MIPMAPED;
        surface->PixelFormat = KM_PIXELFORMAT_PALETTIZED_4BPP;
        break;
    case KM_TEXTURE_PALETTIZE8:
        surface->fSurfaceFlags = KM_SURFACEFLAGS_PALETTIZED;
        surface->PixelFormat = KM_PIXELFORMAT_PALETTIZED_8BPP;
        break;
    case KM_TEXTURE_PALETTIZE8_MM:
        surface->fSurfaceFlags =
            KM_SURFACEFLAGS_PALETTIZED | KM_SURFACEFLAGS_MIPMAPED;
        surface->PixelFormat = KM_PIXELFORMAT_PALETTIZED_8BPP;
        break;
    case KM_TEXTURE_RECTANGLE:
        surface->fSurfaceFlags = 0;
        break;
    case KM_TEXTURE_STRIDE:
        surface->fSurfaceFlags = KM_SURFACEFLAGS_STRIDE;
        break;
    case KM_TEXTURE_BMP:
        surface->fSurfaceFlags = KM_SURFACEFLAGS_TWIDDLED;
        break;
    case KM_TEXTURE_BMP_MM:
        surface->fSurfaceFlags =
            KM_SURFACEFLAGS_TWIDDLED | KM_SURFACEFLAGS_MIPMAPED;
        break;
    default:
        return -1;
    }
    if (!(surface->fSurfaceFlags & KM_SURFACEFLAGS_PALETTIZED)) {
        switch (format) {
        case KM_TEXTURE_ARGB1555:
            surface->PixelFormat = KM_PIXELFORMAT_ARGB1555;
            bpp = 2;
            break;
        case KM_TEXTURE_RGB565:
            surface->PixelFormat = KM_PIXELFORMAT_RGB565;
            bpp = 2;
            break;
        case KM_TEXTURE_ARGB4444:
            surface->PixelFormat = KM_PIXELFORMAT_ARGB4444;
            bpp = 2;
            break;
        case KM_TEXTURE_YUV422:
            surface->PixelFormat = KM_PIXELFORMAT_YUV422;
            bpp = 1;
            break;
        case KM_TEXTURE_BUMP:
            surface->PixelFormat = KM_PIXELFORMAT_BUMP;
            bpp = 2;
            break;
        default:
            return -1;
        }
    }
    switch (width) {
    case 8:
        level = 7;
        break;
    case 16:
        level = 6;
        break;
    case 32:
        level = 5;
        break;
    case 64:
        level = 4;
        break;
    case 128:
        level = 3;
        break;
    case 256:
        level = 2;
        break;
    case 512:
        level = 1;
        break;
    case 1024:
        level = 0;
        break;
    default:
        return -1;
    }
    switch (height) {
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
    case 512:
    case 1024:
        break;
    default:
        return -1;
    }
    surface->Type = type << 16;
    surface->nWidth = width;
    surface->nHeight = height;
    if (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
        if (width == height) {
            surface->TextureSize = mipmap_sizes[level] + NJD_PVR_MM_DUMMYBYTE;
        } else {
            return -1;
        }
    } else {
        /* @bug uninitialized: only the pixel format switch sets bpp, and a
         * palettized surface skips it (BF at 0x8c66b52c in 1ST_READ.BIN). a
         * palettized surface without mipmaps then takes its TextureSize from
         * the caller's R12, which bpp's register still holds (pushed at
         * 0x8c66b458, read by MUL.L R12,R5 at 0x8c66b620). LOADER.BIN has the
         * same at 0x8ce1ee14, 0x8ce1ed40 and 0x8ce1ef08 */
        surface->TextureSize = width * height * bpp;
    }
    return 1;
}

Sint32 njLoadTexMemFromFile(NJS_TEXNAME *texname, NJS_TEXINFO *info)
{
    Sint32 stat;
    Sint32 size;
    Sint32 texsize;
    Uint32 chunk;
    Uint32 offset;
    Uint32 category;
    /* @hack redundant: a 16-byte local that no instruction reads or writes,
     * which the frame keeps room for (ADD #-24,R15 at 0x8c66b648 in
     * 1ST_READ.BIN); without it, the frame is 16 bytes smaller and 2 of 134
     * words differ in each image */
    struct pvr_file_header header;

    if ((stat = njGetSurfacePvr(info, texname->attr, &size)) < 0) {
        return stat;
    }
    texsize = info->texsurface.TextureSize;
    size -= 8;
    if (size > texsize) {
        /* @hack spelling: the subtraction spelled out, which takes texsize
         * from the copy of size the test read, in R3 (SUB R14,R3 at
         * 0x8c66b66a in 1ST_READ.BIN); as -=, 109 of 134 words differ */
        size = size - texsize;
    } else {
        texsize = size;
        size = 0;
    }
    offset = 0;
    if ((Uint32)texsize <= _nj_texture_buffer_max_size) {
        chunk = texsize;
    } else {
        chunk = _nj_texture_buffer_max_size;
    }
    category = (info->texsurface.Type >> 16) & NJD_TEXFMT_TYPE_MASK;
    if (category != KM_TEXTURE_SMALLVQ && category != KM_TEXTURE_SMALLVQ_MM) {
        if (texsize > 0) {
            do {
                if (njTexFread(_nj_texture_buffer_st, chunk) < 0) {
                    kmFreeTexture((PKMSURFACEDESC)&info->texsurface);
                    return -1;
                }
                syCacheOCWB(_nj_texture_buffer_st, chunk);
                kmLoadTexturePart((PKMSURFACEDESC)&info->texsurface,
                                  (KMDWORD *)_nj_texture_buffer_st, offset,
                                  chunk);
                offset += chunk;
                if (texsize - offset < _nj_texture_buffer_max_size) {
                    chunk = texsize - offset;
                }
                while (kmQueryFinishLastTextureDMA() != 0) {
                }
            } while (offset != (Uint32)texsize);
        }
    } else {
        if (njTexFread(_nj_texture_buffer_st, texsize) < 0) {
            kmFreeTexture((PKMSURFACEDESC)&info->texsurface);
            return -1;
        }
        syCacheOCWB(_nj_texture_buffer_st, texsize);
        kmLoadTexture((PKMSURFACEDESC)&info->texsurface,
                      (KMDWORD *)_nj_texture_buffer_st, 0, 0);
        while (kmQueryFinishLastTextureDMA() != 0) {
        }
    }
    if (size > 0) {
        if (njTexFread(_nj_texture_buffer_st, size) < 0) {
            kmFreeTexture((PKMSURFACEDESC)&info->texsurface);
            return -1;
        }
    }
    return 1;
}

Sint32 njLoadCacheFromFile(NJS_TEXNAME *texname, NJS_TEXINFO *info)
{
    Sint32 stat;
    Sint32 size;
    Sint32 texsize;
    void *cache;
    /* @hack redundant: a 16-byte local that no instruction reads or writes,
     * which the frame keeps room for (ADD #-24,R15 at 0x8c66b75e in
     * 1ST_READ.BIN); without it, the frame is 16 bytes smaller and 2 of 53
     * words differ in each image */
    struct pvr_file_header header;
    Sint32 datasize;

    if ((stat = njGetInfoPvr(info, texname->attr, &size)) < 0) {
        return stat;
    }
    if (info->texsurface.fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
        texsize = info->texsurface.TextureSize - NJD_PVR_MM_DUMMYBYTE;
    } else {
        texsize = info->texsurface.TextureSize;
    }
    cache = njCacheTextureMalloc(texsize);
    if (cache == 0) {
        _nj_texerr_.texerr = NJD_TEXERR_MAINMEMORY;
        return -NJD_TEXERR_MAINMEMORY;
    }
    datasize = size - 8;
    if (njTexFread(cache, datasize) < 0) {
        return -1;
    }
    info->texaddr = cache;
    return 1;
}

Sint32 njLoadTexMemFromMemory(NJS_TEXNAME *texname, NJS_TEXINFO *info)
{
    Sint32 stat;

    if ((stat = njGetInfoMemory(info)) < 0) {
        return stat;
    }
    if ((stat = njTexDownloadMipMap(0, info)) < 0) {
        kmFreeTexture((PKMSURFACEDESC)&info->texsurface);
        return stat;
    }
    return 1;
}

Sint32 njLoadTexMemFromCache(NJS_TEXNAME *texname, NJS_TEXINFO *info)
{
    Sint32 stat;

    if ((stat = njGetInfoMemory(info)) < 0) {
        return stat;
    }
    if ((stat = njTexDownloadMipMap(0, info)) < 0) {
        kmFreeTexture((PKMSURFACEDESC)&info->texsurface);
        return stat;
    }
    return 1;
}

Sint32 njGetInfoMemory(NJS_TEXINFO *info)
{
    Sint32 type;
    Sint32 textype;
    Sint32 width;
    Sint32 height;

    width = info->texsurface.nWidth;
    height = info->texsurface.nHeight;
    type = info->texsurface.Type;
    textype = info->texsurface.Type >> 16;
    switch (textype) {
    case KM_TEXTURE_BMP:
        if (width == height) {
            textype = KM_TEXTURE_TWIDDLED | (textype & NJD_TEXFMT_COLOR_MASK);
        } else {
            textype = KM_TEXTURE_RECTANGLE | (textype & NJD_TEXFMT_COLOR_MASK);
        }
        break;
    case KM_TEXTURE_BMP_MM:
        textype = KM_TEXTURE_TWIDDLED_MM | (textype & NJD_TEXFMT_COLOR_MASK);
        break;
    }
    if (kmCreateTextureSurface((PKMSURFACEDESC)&info->texsurface, width, height,
                               textype) != 0) {
        _nj_texerr_.texerr = NJD_TEXERR_SURFACE;
        return -NJD_TEXERR_SURFACE;
    }
    info->texsurface.Type |= type;
    return 1;
}
