/* Kamui texture surfaces: allocation, loading, reading back and freeing */
#include <kamui.h>
#include <sg_xpt.h>

/* the bytes of a full VQ codebook. a VQ texture's indices start this far into
 * its surface, a small VQ texture's too, though its codebook is shorter */
#define CODEBOOK_SIZE 0x800

/* where loads and reads of a 16-bit twiddled texture's mipmaps start: past
 * their first long, which no level reaches */
#define MIPMAP16_START 4

/* the video chip's registers, as offsets into its register block */
#define TEXTURE_CONTROL 0x0e4
#define TA_YUV_BASE 0x148
#define TA_YUV_CONTROL 0x14c

/* TEXTURE_CONTROL's stride field: the width of every stride texture, in units
 * of 1 << STRIDE_SHIFT texels */
#define STRIDE 0x1f
#define STRIDE_SHIFT 5

/* TA_YUV_CONTROL's fields. the last macroblock column is in bits 0 to 5, and
 * the last row in bits 8 to 13. bit 16 selects a run of textures rather than
 * one, and bit 24 a 4:2:2 source rather than 4:2:0 */
#define YUV_LAST 0x3f
#define YUV_ROW_SHIFT 8
#define YUV_TEXTURES_SHIFT 16
#define YUV_422_SHIFT 24

/* the bits of a surface's offset that kmLoadYUVTexture writes to TA_YUV_BASE:
 * an offset below 16 MB, on 8 bytes */
#define YUV_BASE_BITS 0x00fffff8

/* a macroblock's side in texels, and its bytes in a 4:2:0 and a 4:2:2
 * source */
#define MACROBLOCK_SIDE 16
#define MACROBLOCK_420 384
#define MACROBLOCK_422 512

/* the tile accelerator's YUV converter input, as kmiDMAtoTARequest's target */
#define TA_YUV_CONVERTER 0x10800000

/* the bytes of an operand cache line */
#define CACHE_LINE 32

static KMSTATUS describe_surface(PKMSURFACEDESC pSurfaceDesc, KMINT32 nWidth,
                                 KMINT32 nHeight, KMTEXTURETYPE nTextureType);
static int texture_level(int width, int height);
static void set_surface_size(PKMSURFACEDESC surface);
static KMDWORD plain_size(PKMSURFACEDESC surface);
static KMDWORD vq_size(PKMSURFACEDESC surface);
static KMDWORD palette_size(PKMSURFACEDESC surface);
static KMDWORD small_vq_size(PKMSURFACEDESC surface);
static KMINT32 load_mipmapped(KMDWORD offset, PKMDWORD texture, int width);
static KMINT32 get_mipmapped(PKMDWORD texture, KMDWORD offset, int width);
static KMINT32 load_small_vq(KMDWORD offset, KMBYTE *texture, int width,
                             KMDWORD flags);
static KMINT32 get_small_vq(KMBYTE *texture, KMDWORD offset, int width,
                            KMDWORD flags);
static KMSTATUS reload_palette4(PKMSURFACEDESC surface, KMBYTE *texture,
                                int width);
static KMSTATUS reload_palette8(PKMSURFACEDESC surface, KMBYTE *texture,
                                int width);
static KMSTATUS reload_twiddled(PKMSURFACEDESC surface, KMBYTE *texture,
                                int width);
static KMSTATUS reload_vq(PKMSURFACEDESC surface, KMBYTE *texture, int width);
static int nearest_3_bits(KMBYTE value);

/* the tables below are indexed by level. level n is 1 << n texels on a side,
 * from 0 for 1 texel to 10 for 1024 */

/* a small VQ texture's codebook bytes, without mipmaps and with them, up to
 * level 6, the largest small VQ texture */
static const KMDWORD small_codebook[7] = {0, 0, 0, 128, 128, 256, 1024};
static const KMDWORD small_codebook_mipmap[7] = {0, 0, 0, 128, 128, 512, 2048};
/* a 16-bit twiddled level's offset within the texture, and its bytes */
static const KMDWORD level16_offset[11] = {
    6, 8, 16, 48, 176, 688, 2736, 10928, 43696, 174768, 699056,
};
static const KMDWORD level16_size[11] = {
    2, 8, 32, 128, 512, 2048, 8192, 32768, 131072, 524288, 2097152,
};
/* the bytes of a 16-bit twiddled texture's mipmaps from MIPMAP16_START
 * through the end of that level. below level 3, smaller than any texture
 * describe_surface takes, the entry is 0 */
static const KMDWORD mipmap16_size[11] = {
    0, 0, 0, 172, 684, 2732, 10924, 43692, 174764, 699052, 2796204,
};
/* 1 for an odd level and 0 for an even one. no code reads it */
static const KMDWORD level_parity[11] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};
/* a VQ level's index bytes, and the offset from the first index where its
 * indices end and the next level's begin */
static const KMDWORD vq_level_size[11] = {
    0, 1, 4, 16, 64, 256, 1024, 4096, 16384, 65536, 262144,
};
static const KMDWORD vq_level_end[11] = {
    1, 2, 6, 22, 86, 342, 1366, 5462, 21846, 87382, 349526,
};
/* an 8-bit palettized level's offset within the texture, and its bytes */
static const KMDWORD level8_offset[11] = {
    3, 4, 8, 24, 88, 344, 1368, 5464, 21848, 87384, 349528,
};
static const KMDWORD level8_size[11] = {
    1, 4, 16, 64, 256, 1024, 4096, 16384, 65536, 262144, 1048576,
};
/* a 4-bit palettized level's offset within the texture, and its bytes */
static const KMDWORD level4_offset[11] = {
    1, 2, 4, 12, 44, 172, 684, 2732, 10924, 43692, 174764,
};
static const KMDWORD level4_size[11] = {
    1, 2, 8, 32, 128, 512, 2048, 8192, 32768, 131072, 524288,
};

KMSTATUS kmSetStrideWidth(KMINT32 nWidth)
{
    /* @hack spelling: the stride computed ahead of the register read. SHC then
     * keeps it on the stack across kmiReadRegister (MOV.L R4,@R15 at
     * 0x8c661e4e). computed in value's expression, after the read, 20 of 22
     * words differ */
    int stride = (nWidth & (STRIDE << STRIDE_SHIFT)) >> STRIDE_SHIFT;
    KMDWORD value;

    value = (kmiReadRegister(TEXTURE_CONTROL) & ~STRIDE) | stride;
    kmiWriteRegister(TEXTURE_CONTROL, value);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmCreateTextureSurface(PKMSURFACEDESC pSurfaceDesc, KMINT32 nWidth,
                                KMINT32 nHeight, KMTEXTURETYPE nTextureType)
{
    KMSTATUS status;

    status = describe_surface(pSurfaceDesc, nWidth, nHeight, nTextureType);
    if (status != KMSTATUS_SUCCESS) {
        return status;
    }
    if ((pSurfaceDesc->SurfaceType & 0xffff) ==
        KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        status = kmiVQalloc(pSurfaceDesc);
    } else {
        status = kmiTXalloc(pSurfaceDesc->uSize.dwTextureSize, pSurfaceDesc);
    }
    if (status == KMSTATUS_NOT_ENOUGH_MEMORY) {
        kmiDefaultInterruptHandler(TEXTURE_OVERFLOW_CALLBACK);
    }
    return status;
}

KMSTATUS kmCreateCombinedTextureSurface(PKMSURFACEDESC pSurfaceDesc1,
                                        PKMSURFACEDESC pSurfaceDesc2,
                                        KMINT32 nWidth, KMINT32 nHeight,
                                        KMTEXTURETYPE nTextureType)
{
    KMSTATUS status;

    status = describe_surface(pSurfaceDesc1, nWidth, nHeight, nTextureType);
    if (status != KMSTATUS_SUCCESS) {
        return status;
    }
    pSurfaceDesc2->SurfaceType = pSurfaceDesc1->SurfaceType;
    pSurfaceDesc2->BitDepth = pSurfaceDesc1->BitDepth;
    pSurfaceDesc2->PixelFormat = pSurfaceDesc1->PixelFormat;
    pSurfaceDesc2->u0.USize = pSurfaceDesc1->u0.USize;
    pSurfaceDesc2->u1.VSize = pSurfaceDesc1->u1.VSize;
    pSurfaceDesc2->uSize.dwTextureSize = pSurfaceDesc1->uSize.dwTextureSize;
    pSurfaceDesc2->fSurfaceFlags = pSurfaceDesc1->fSurfaceFlags;
    if ((pSurfaceDesc1->SurfaceType & 0xffff) ==
        KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        status = kmiVQalloc(pSurfaceDesc1);
        if (status == KMSTATUS_NOT_ENOUGH_MEMORY) {
            kmiDefaultInterruptHandler(TEXTURE_OVERFLOW_CALLBACK);
        } else {
            status = kmiVQalloc(pSurfaceDesc2);
            if (status == KMSTATUS_NOT_ENOUGH_MEMORY) {
                kmiVQFBfree(pSurfaceDesc1);
                kmiDefaultInterruptHandler(TEXTURE_OVERFLOW_CALLBACK);
            }
        }
    } else {
        status = kmiTXalloc(pSurfaceDesc1->uSize.dwTextureSize, pSurfaceDesc1);
        if (status == KMSTATUS_NOT_ENOUGH_MEMORY) {
            kmiDefaultInterruptHandler(TEXTURE_OVERFLOW_CALLBACK);
        } else {
            status =
                kmiTXalloc(pSurfaceDesc2->uSize.dwTextureSize, pSurfaceDesc2);
            if (status == KMSTATUS_NOT_ENOUGH_MEMORY) {
                kmiFBfree((KMINT32)pSurfaceDesc1->pSurface);
                kmiDefaultInterruptHandler(TEXTURE_OVERFLOW_CALLBACK);
            }
        }
    }
    return status;
}

/* one kmiTXalloc finds a block for all the surfaces, and is freed at once.
 * each surface then claims its part of the block with kmiABSalloc */
KMSTATUS kmCreateContiguousTextureSurface(PKMSURFACEDESC pSurfaceDesc,
                                          KMINT32 nTexture, KMINT32 nWidth,
                                          KMINT32 nHeight,
                                          KMTEXTURETYPE nTextureType)
{
    /* @hack redundant: status set before describe_surface's result replaces
     * it. SHC then forms &pSurfaceDesc[1] as MOV R14,R4 and ADD #40,R4
     * (0x8c661fd6, 0x8c661fda); without it, 2 of 132 words differ */
    KMSTATUS status = KMSTATUS_SUCCESS;
    KMDWORD size;
    PKMDWORD next;
    int i;
    int j;

    status = describe_surface(pSurfaceDesc, nWidth, nHeight, nTextureType);
    if (status != KMSTATUS_SUCCESS) {
        return status;
    }
    if ((pSurfaceDesc->SurfaceType & 0xffff) ==
        KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    /* @hack redundant: the loop's test made ahead of it too, as nTexture > 1.
     * the image tests twice, CMP/GT at 0x8c661fce and CMP/GE at 0x8c661fd4;
     * without it, 60 of 132 words differ */
    if (nTexture > 1) {
        /* @hack spelling: i = i + 1, which gives next R8 and kmiFBfree's
         * address R9 (0x8c662040, 0x8c662042); without it, the two swap and 8
         * of 132 words differ */
        for (i = 1; i < nTexture; i = i + 1) {
            pSurfaceDesc[i].SurfaceType = pSurfaceDesc->SurfaceType;
            pSurfaceDesc[i].BitDepth = pSurfaceDesc->BitDepth;
            pSurfaceDesc[i].PixelFormat = pSurfaceDesc->PixelFormat;
            pSurfaceDesc[i].u0.USize = pSurfaceDesc->u0.USize;
            pSurfaceDesc[i].u1.VSize = pSurfaceDesc->u1.VSize;
            pSurfaceDesc[i].uSize.dwTextureSize =
                pSurfaceDesc->uSize.dwTextureSize;
            pSurfaceDesc[i].fSurfaceFlags = pSurfaceDesc->fSurfaceFlags;
        }
    }
    size = pSurfaceDesc->uSize.dwTextureSize * nTexture;
    status = kmiTXalloc(size, pSurfaceDesc);
    if (status == KMSTATUS_NOT_ENOUGH_MEMORY) {
        kmiDefaultInterruptHandler(TEXTURE_OVERFLOW_CALLBACK);
    } else {
        next = pSurfaceDesc->pSurface;
        kmiFBfree((KMINT32)pSurfaceDesc->pSurface);
        for (i = 0; i < nTexture; i++) {
            pSurfaceDesc[i].pSurface = next;
            status = kmiABSalloc(pSurfaceDesc[i].uSize.dwTextureSize,
                                 &pSurfaceDesc[i]);
            next += pSurfaceDesc[i].uSize.dwTextureSize / sizeof(KMDWORD);
            if (status == KMSTATUS_NOT_ENOUGH_MEMORY) {
                /* @hack spelling: pSurfaceDesc stepped to the failed surface
                 * and read through. SHC then steps R14 itself (ADD R3,R14 at
                 * 0x8c662084), and keeps next in R8 through the allocation loop
                 * (ADD R3,R8 at 0x8c662074) rather than in a frame slot;
                 * without it, 62 of 132 words differ */
                pSurfaceDesc += i;
                /* @bug memory: the surfaces before the failed one stay
                 * allocated. the loop frees the failed surface's address, where
                 * kmiABSalloc allocated nothing, i times (JSR @R9 at
                 * 0x8c66208a), and none of the surfaces before it */
                for (j = 0; j < i; j++) {
                    kmiFBfree((KMINT32)pSurfaceDesc->pSurface);
                }
                kmiDefaultInterruptHandler(TEXTURE_OVERFLOW_CALLBACK);
                return status;
            }
        }
    }
    return status;
}

KMSTATUS kmLoadTexture(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture,
                       KMBOOLEAN fAutoMipMap, KMBOOLEAN fDither)
{
    if (fAutoMipMap == TRUE || fDither == TRUE) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    if ((pSurfaceDesc->SurfaceType & 0xffff) ==
        KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        load_small_vq((KMDWORD)pSurfaceDesc->pSurface, (KMBYTE *)pTexture,
                      pSurfaceDesc->u0.USize, pSurfaceDesc->fSurfaceFlags);
    } else if ((pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_TWIDDLED) &&
               (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) &&
               !(pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_VQ) &&
               !(pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_PALETTIZED)) {
        load_mipmapped((KMDWORD)pSurfaceDesc->pSurface, pTexture,
                       pSurfaceDesc->u0.USize);
    } else {
        kmiCopyToFrameBuffer((KMDWORD)pSurfaceDesc->pSurface, pTexture,
                             pSurfaceDesc->uSize.dwTextureSize);
    }
    /* @hack redundant: a call of nearest_3_bits that never runs. SHC leaves
     * out a static function nothing calls, and nothing else calls it. without
     * it, nearest_3_bits goes, the section runs 96 bytes short and its 23 of
     * 23 words differ */
    if (0) {
        nearest_3_bits(0);
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmLoadTextureBlock(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture,
                            KMUINT32 nBlockNum, KMUINT32 nBlockSize)
{
    KMDWORD start;
    KMDWORD rest;

    if ((pSurfaceDesc->SurfaceType & 0xffff) ==
        KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    } else if ((pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_TWIDDLED) &&
               (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) &&
               !(pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_VQ) &&
               !(pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_PALETTIZED)) {
        start = MIPMAP16_START;
    } else {
        start = 0;
    }
    nBlockSize *= 32;
    start += nBlockNum * nBlockSize;
    rest = pSurfaceDesc->uSize.dwTextureSize - start;
    if (start >= pSurfaceDesc->uSize.dwTextureSize) {
        return KMSTATUS_INVALID_BLOCKNUMBER;
    }
    if (rest > nBlockSize) {
        rest = nBlockSize;
    }
    start += (KMDWORD)pSurfaceDesc->pSurface;
    kmiCopyToFrameBuffer(start, pTexture, rest);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmLoadTexturePart(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture,
                           KMUINT32 nOffset, KMUINT32 nSize)
{
    KMDWORD position;
    KMDWORD skip;
    KMDWORD start;
    KMDWORD rest;

    position = nOffset & ~3;
    nSize &= ~3;
    if (nSize == 0) {
        return KMSTATUS_SUCCESS;
    }
    if ((pSurfaceDesc->SurfaceType & 0xffff) ==
        KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    } else if ((pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_TWIDDLED) &&
               (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) &&
               !(pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_VQ) &&
               !(pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_PALETTIZED)) {
        skip = MIPMAP16_START;
    } else {
        skip = 0;
    }
    start = position + skip;
    rest = pSurfaceDesc->uSize.dwTextureSize - start;
    if (start >= pSurfaceDesc->uSize.dwTextureSize) {
        return KMSTATUS_INVALID_ADDRESS;
    }
    if (rest > nSize) {
        rest = nSize;
    }
    start += (KMDWORD)pSurfaceDesc->pSurface;
    kmiCopyToFrameBuffer(start, pTexture, rest);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmLoadRectangleTexturePart(PKMSURFACEDESC pSurfaceDesc,
                                    PKMDWORD pTexture, KMUINT32 width,
                                    KMUINT32 height, KMUINT32 dst_X,
                                    KMUINT32 dst_Y, KMUINT32 src_X,
                                    KMUINT32 src_Y, KMUINT32 src_U)
{
    KMDWORD destination;
    KMDWORD row;
    KMDWORD pitch;
    KMDWORD source_pitch;
    KMDWORD texels;
    PKMDWORD source;
    KMUINT32 y;

    if ((pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_TWIDDLED) ||
        (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) ||
        (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_VQ) ||
        (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_PALETTIZED)) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    /* @hack spelling: the pitch made from USize through texels, then doubled
     * in place. SHC then doubles a copy of USize in R12 (MOV R7,R12 at
     * 0x8c662286); without it, 54 of 81 words differ */
    texels = pSurfaceDesc->u0.USize;
    pitch = texels;
    pitch <<= 1;
    row = width * 2;
    destination = (dst_Y * pitch) + (dst_X * 2);
    destination += (KMDWORD)pSurfaceDesc->pSurface;
    if (src_U == 0) {
        source = pTexture;
        source_pitch = row;
    } else {
        source_pitch = src_U * 2;
        source = pTexture + (((src_Y * source_pitch) + (src_X * 2)) / 4);
    }
    for (y = 0; y < height; y++) {
        kmiCopyToFrameBuffer(destination, source, row);
        destination += pitch;
        source += source_pitch / 4;
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmLoadYUVTexture(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture,
                          KMINT32 nWidth, KMINT32 nHeight, KMINT32 nFormat,
                          KMBOOLEAN bWaitEndOfDMA)
{
    KMBOOLEAN yuv422;
    KMBOOLEAN several;
    KMDWORD last_column;
    KMDWORD last_row;
    KMDWORD blocks;
    KMDWORD size;
    KMDWORD control;
    KMDWORD dma;
    PKMDWORD handle;
    KMSTATUS result;

    if ((KMDWORD)pTexture & (CACHE_LINE - 1)) {
        return KMSTATUS_INVALID_ADDRESS;
    }
    /* @bug logic: an RGB565 or ARGB4444 surface passes the format test as a
     * YUV422 one does. the test holds for a PixelFormat with either of
     * KM_PIXELFORMAT_YUV422's two bits (TST R3,R2 at 0x8c66232e), so the
     * converter's YUV422 texels can load into a surface of another format */
    if ((pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) ||
        (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_TWIDDLED) ||
        (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_VQ) ||
        (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_PALETTIZED) ||
        !(pSurfaceDesc->PixelFormat & KM_PIXELFORMAT_YUV422)) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    switch (nFormat) {
    case KM_TEXTURE_YUV420:
        yuv422 = FALSE;
        break;
    case KM_TEXTURE_YUV422:
        yuv422 = TRUE;
        break;
    default:
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    if (nWidth < 1 || nWidth > YUV_LAST + 1) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    if (nHeight < 1 || nHeight > YUV_LAST + 1) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    if (nWidth * nHeight >= 2) {
        several = TRUE;
        last_column = nWidth - 1;
        last_row = nHeight - 1;
    } else {
        several = FALSE;
        last_column = (pSurfaceDesc->u0.USize / MACROBLOCK_SIDE) - 1;
        last_row = (pSurfaceDesc->u1.VSize / MACROBLOCK_SIDE) - 1;
    }
    blocks = (last_column + 1) * (last_row + 1);
    if (!yuv422) {
        if (!several) {
            size = blocks * MACROBLOCK_420;
        } else {
            size = nWidth * nHeight * MACROBLOCK_420;
        }
    } else {
        if (!several) {
            size = blocks * MACROBLOCK_422;
        } else {
            size = nWidth * nHeight * MACROBLOCK_422;
        }
    }
    control = (yuv422 & 1) << YUV_422_SHIFT |
              (several & 1) << YUV_TEXTURES_SHIFT |
              (last_row & YUV_LAST) << YUV_ROW_SHIFT | (last_column & YUV_LAST);
    kmiWriteRegister(TA_YUV_CONTROL, control);
    kmiWriteRegister(TA_YUV_BASE,
                     (KMDWORD)pSurfaceDesc->pSurface & YUV_BASE_BITS);
    result = DMA_QUEUE_FULL;
    while (result == DMA_QUEUE_FULL) {
        result = kmiDMAtoTARequest(TA_YUV_CONVERTER, pTexture, size,
                                   DMA_SECOND_QUEUE | DMA_SOURCE_UP, &handle);
        /* @bug uninitialized: kmiDMAtoTARequest sets handle only when it
         * queues the transfer (MOV.L R14,@R2 at 0x8c663b26). a transfer refused
         * with DMA_QUEUE_FULL branches past that store (0x8c663af0). with a
         * size of 0, which a single macroblock gives on a surface under 16
         * texels wide or high, it returns DMA_EMPTY before that store
         * (0x8c663a52). either way, this reads the unset handle and the word it
         * points to (0x8c662424, 0x8c66242c) */
        dma = *handle;
    }
    if (result != DMA_QUEUED) {
        return KMSTATUS_HARDWARE_NOT_PRESENTED;
    }
    if (bWaitEndOfDMA == TRUE) {
        kmiWaitForFinishDMA(dma);
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmLoadVQCodebook(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture)
{
    KMSTATUS status = KMSTATUS_SUCCESS;
    KMDWORD size;
    int level;

    if ((pSurfaceDesc->SurfaceType & 0xffff) ==
        KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        level = texture_level(pSurfaceDesc->u0.USize, pSurfaceDesc->u0.USize);
        if (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
            size = small_codebook_mipmap[level];
        } else {
            size = small_codebook[level];
        }
        kmiCopyToFrameBuffer((KMDWORD)pSurfaceDesc->pSurface, pTexture, size);
    } else if (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_VQ) {
        kmiCopyToFrameBuffer((KMDWORD)pSurfaceDesc->pSurface, pTexture,
                             CODEBOOK_SIZE);
    } else {
        status = KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    return status;
}

KMSTATUS kmFreeTexture(PKMSURFACEDESC pSurfaceDesc)
{
    KMSTATUS status;

    if ((pSurfaceDesc->SurfaceType & 0xffff) ==
        KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        status = kmiVQFBfree(pSurfaceDesc);
    } else {
        status = kmiFBfree((KMINT32)pSurfaceDesc->pSurface);
    }
    return status;
}

KMSTATUS kmGetFreeTextureMem(PKMUINT32 pSizeOfTexture,
                             PKMUINT32 pMaxBlockSizeOfTexture)
{
    KMINT32 bank0;
    KMINT32 bank1;
    KMINT32 vq;
    KMINT32 largest;
    KMSTATUS status;

    status = kmiFBgetfreemem(&bank0, &bank1, &vq, &largest);
    *pSizeOfTexture = bank0 + bank1 + vq;
    *pMaxBlockSizeOfTexture = largest;
    return status;
}

KMSTATUS kmReLoadMipmap(PKMSURFACEDESC pSurfaceDesc, PKMVOID pTexture,
                        KMINT32 nMipmapCount)
{
    KMSTATUS status;

    if (pSurfaceDesc->PixelFormat == KM_PIXELFORMAT_PALETTIZED_4BPP &&
        (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED)) {
        status = reload_palette4(pSurfaceDesc, pTexture, nMipmapCount);
    } else if (pSurfaceDesc->PixelFormat == KM_PIXELFORMAT_PALETTIZED_8BPP &&
               (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED)) {
        status = reload_palette8(pSurfaceDesc, pTexture, nMipmapCount);
    } else if ((pSurfaceDesc->SurfaceType & 0xffff) ==
               KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        status = reload_vq(pSurfaceDesc, pTexture, nMipmapCount);
        /* @hack spelling: the VQ flag tested in a branch of its own below,
         * which calls reload_vq again (BSR at 0x8c6625ae). joined to this
         * branch's test by ||, 29 of 53 words differ */
    } else if (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_VQ) {
        status = reload_vq(pSurfaceDesc, pTexture, nMipmapCount);
    } else if ((pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_TWIDDLED) &&
               (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED)) {
        status = reload_twiddled(pSurfaceDesc, pTexture, nMipmapCount);
    } else {
        status = KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    return status;
}

/* a frame buffer's pSurface is an offset in the 32-bit view, which
 * kmiCopyFromPVRMemory takes. a texture's is in the 64-bit view, which
 * kmiCopyFromFrameBuffer takes */
KMSTATUS kmGetTexture(PKMDWORD pTexture, PKMSURFACEDESC pSurfaceDesc)
{
    if ((pSurfaceDesc->SurfaceType & 3) == KM_SURFACETYPE_FRAMEBUFFER) {
        kmiCopyFromPVRMemory(pTexture, pSurfaceDesc->pSurface,
                             pSurfaceDesc->uSize.dwTextureSize);
    } else if ((pSurfaceDesc->SurfaceType & 0xffff) ==
               KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        get_small_vq((KMBYTE *)pTexture, (KMDWORD)pSurfaceDesc->pSurface,
                     pSurfaceDesc->u0.USize, pSurfaceDesc->fSurfaceFlags);
    } else if ((pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_TWIDDLED) &&
               (pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) &&
               !(pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_VQ) &&
               !(pSurfaceDesc->fSurfaceFlags & KM_SURFACEFLAGS_PALETTIZED)) {
        get_mipmapped(pTexture, (KMDWORD)pSurfaceDesc->pSurface,
                      pSurfaceDesc->u0.USize);
    } else {
        kmiCopyFromFrameBuffer(pTexture, (KMDWORD)pSurfaceDesc->pSurface,
                               pSurfaceDesc->uSize.dwTextureSize);
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmGarbageCollectTexture(void)
{
    /* @hack spelling: the result through status, which keeps the call of
     * kmiFBGarbageCollect a JSR followed by an RTS (JSR @R2 at 0x8c662674);
     * without it, SHC jumps to it and 5 of 10 words differ */
    KMSTATUS status;

    kmiFBSortQueue();
    status = kmiFBGarbageCollect();
    return status;
}

/* describe pSurfaceDesc as an nWidth by nHeight texture of nTextureType, its
 * size included. returns KMSTATUS_SUCCESS, or KMSTATUS_INVALID_TEXTURE_TYPE
 * for any of three faults: in a kind that isn't palettized, a pixel format the
 * switch below doesn't name; a side that isn't a power of two from 8 to 1024;
 * or unequal sides in a kind that must be square. the description is filled in
 * either way, but a refused pixel format leaves PixelFormat as it was */
static KMSTATUS describe_surface(PKMSURFACEDESC pSurfaceDesc, KMINT32 nWidth,
                                 KMINT32 nHeight, KMTEXTURETYPE nTextureType)
{
    KMSTATUS status = KMSTATUS_SUCCESS;
    KMTEXTURETYPE kind = nTextureType & 0xff00;
    KMTEXTURETYPE format = nTextureType & 0xff;

    if (kind == KM_TEXTURE_SMALLVQ || kind == KM_TEXTURE_SMALLVQ_MM) {
        pSurfaceDesc->SurfaceType = KM_SURFACETYPE_SMALLVQ_TEXTURE;
    } else {
        pSurfaceDesc->SurfaceType = KM_SURFACETYPE_TEXTURE;
    }
    if (kind == KM_TEXTURE_PALETTIZE4 || kind == KM_TEXTURE_PALETTIZE4_MM) {
        if (kmiHardwareResource->palette_mode == KM_PALETTE_32BPP_ARGB8888) {
            pSurfaceDesc->BitDepth = KMI_BITDEPTH_32;
        } else {
            pSurfaceDesc->BitDepth = KMI_BITDEPTH_16;
        }
        pSurfaceDesc->PixelFormat = KM_PIXELFORMAT_PALETTIZED_4BPP;
    } else if (kind == KM_TEXTURE_PALETTIZE8 ||
               kind == KM_TEXTURE_PALETTIZE8_MM) {
        if (kmiHardwareResource->palette_mode == KM_PALETTE_32BPP_ARGB8888) {
            pSurfaceDesc->BitDepth = KMI_BITDEPTH_32;
        } else {
            pSurfaceDesc->BitDepth = KMI_BITDEPTH_16;
        }
        pSurfaceDesc->PixelFormat = KM_PIXELFORMAT_PALETTIZED_8BPP;
    } else {
        pSurfaceDesc->BitDepth = KMI_BITDEPTH_16;
        switch (format) {
        case KM_TEXTURE_ARGB1555:
            pSurfaceDesc->PixelFormat = KM_PIXELFORMAT_ARGB1555;
            break;
        case KM_TEXTURE_RGB565:
            pSurfaceDesc->PixelFormat = KM_PIXELFORMAT_RGB565;
            break;
        case KM_TEXTURE_ARGB4444:
            pSurfaceDesc->PixelFormat = KM_PIXELFORMAT_ARGB4444;
            break;
        case KM_TEXTURE_YUV422:
            pSurfaceDesc->PixelFormat = KM_PIXELFORMAT_YUV422;
            break;
        case KM_TEXTURE_BUMP:
            pSurfaceDesc->PixelFormat = KM_PIXELFORMAT_BUMP;
            break;
        default:
            status = KMSTATUS_INVALID_TEXTURE_TYPE;
            break;
        }
    }
    pSurfaceDesc->u0.USize = nWidth;
    pSurfaceDesc->u1.VSize = nHeight;
    switch (nWidth) {
    case KM_MAPSIZE_8:
    case KM_MAPSIZE_16:
    case KM_MAPSIZE_32:
    case KM_MAPSIZE_64:
    case KM_MAPSIZE_128:
    case KM_MAPSIZE_256:
    case KM_MAPSIZE_512:
    case KM_MAPSIZE_1024:
        break;
    default:
        status = KMSTATUS_INVALID_TEXTURE_TYPE;
        break;
    }
    switch (nHeight) {
    case KM_MAPSIZE_8:
    case KM_MAPSIZE_16:
    case KM_MAPSIZE_32:
    case KM_MAPSIZE_64:
    case KM_MAPSIZE_128:
    case KM_MAPSIZE_256:
    case KM_MAPSIZE_512:
    case KM_MAPSIZE_1024:
        break;
    default:
        status = KMSTATUS_INVALID_TEXTURE_TYPE;
        break;
    }
    /* a kind other than these must be square */
    if (kind != KM_TEXTURE_RECTANGLE && kind != KM_TEXTURE_STRIDE &&
        kind != KM_TEXTURE_TWIDDLED_RECTANGLE &&
        kind != KM_TEXTURE_PALETTIZE4 && kind != KM_TEXTURE_PALETTIZE8 &&
        nWidth != nHeight) {
        status = KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    pSurfaceDesc->fSurfaceFlags = 0;
    if (kind == KM_TEXTURE_BMP_MM || kind == KM_TEXTURE_TWIDDLED_MM ||
        kind == KM_TEXTURE_VQ_MM || kind == KM_TEXTURE_SMALLVQ_MM ||
        kind == KM_TEXTURE_PALETTIZE4_MM || kind == KM_TEXTURE_PALETTIZE8_MM) {
        pSurfaceDesc->fSurfaceFlags |= KM_SURFACEFLAGS_MIPMAPED;
    }
    if (kind == KM_TEXTURE_BMP || kind == KM_TEXTURE_BMP_MM ||
        kind == KM_TEXTURE_TWIDDLED || kind == KM_TEXTURE_TWIDDLED_MM ||
        kind == KM_TEXTURE_TWIDDLED_RECTANGLE) {
        pSurfaceDesc->fSurfaceFlags |= KM_SURFACEFLAGS_TWIDDLED;
    }
    if (kind == KM_TEXTURE_VQ || kind == KM_TEXTURE_VQ_MM ||
        kind == KM_TEXTURE_SMALLVQ || kind == KM_TEXTURE_SMALLVQ_MM) {
        /* @hack spelling: the VQ and twiddled flags set in two statements.
         * SHC then sets them with two ORs, at 0x8c6628f6 and 0x8c6628fa;
         * without it, 46 of 248 words differ */
        pSurfaceDesc->fSurfaceFlags |= KM_SURFACEFLAGS_VQ;
        pSurfaceDesc->fSurfaceFlags |= KM_SURFACEFLAGS_TWIDDLED;
    }
    if (kind == KM_TEXTURE_STRIDE) {
        pSurfaceDesc->fSurfaceFlags |= KM_SURFACEFLAGS_STRIDE;
    }
    if (kind == KM_TEXTURE_PALETTIZE4 || kind == KM_TEXTURE_PALETTIZE4_MM ||
        kind == KM_TEXTURE_PALETTIZE8 || kind == KM_TEXTURE_PALETTIZE8_MM) {
        /* @hack spelling: the palettized and twiddled flags set in two
         * statements too. SHC then sets them with two ORs, at 0x8c662924 and
         * 0x8c662928; without it, 28 of 248 words differ */
        pSurfaceDesc->fSurfaceFlags |= KM_SURFACEFLAGS_PALETTIZED;
        pSurfaceDesc->fSurfaceFlags |= KM_SURFACEFLAGS_TWIDDLED;
    }
    set_surface_size(pSurfaceDesc);
    return status;
}

/* the level of the shorter of width and height. a side that is no power of two
 * from 1 to 1024 is level 3, as 8 is */
static int texture_level(int width, int height)
{
    if (width >= height) {
        width = height;
    }
    switch (width) {
    case KM_MAPSIZE_1:
        return 0;
    case KM_MAPSIZE_2:
        return 1;
    case KM_MAPSIZE_4:
        return 2;
    case KM_MAPSIZE_16:
        return 4;
    case KM_MAPSIZE_32:
        return 5;
    case KM_MAPSIZE_64:
        return 6;
    case KM_MAPSIZE_128:
        return 7;
    case KM_MAPSIZE_256:
        return 8;
    case KM_MAPSIZE_512:
        return 9;
    case KM_MAPSIZE_1024:
        return 10;
    case KM_MAPSIZE_8:
    default:
        return 3;
    }
}

static void set_surface_size(PKMSURFACEDESC surface)
{
    KMDWORD size;

    if ((surface->SurfaceType & 0xffff) == KM_SURFACETYPE_SMALLVQ_TEXTURE) {
        size = small_vq_size(surface);
    } else if (surface->fSurfaceFlags & KM_SURFACEFLAGS_VQ) {
        size = vq_size(surface);
    } else if (surface->fSurfaceFlags & KM_SURFACEFLAGS_PALETTIZED) {
        size = palette_size(surface);
    } else {
        size = plain_size(surface);
    }
    surface->uSize.dwTextureSize = size;
}

/* the bytes of every level's texels, 2 a texel at a BitDepth of KMI_BITDEPTH_16
 * and 4 at any other. with mipmaps, a square texture's 1 by 1 level takes the
 * room of 4 texels, 3 of them padding */
static KMDWORD plain_size(PKMSURFACEDESC surface)
{
    KMDWORD u = surface->u0.USize;
    KMDWORD v = surface->u1.VSize;
    KMDWORD n;
    KMDWORD size = u * v;

    if (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
        u >>= 1;
        v >>= 1;
        n = u < v ? u : v;
        while (n > 1) {
            size += u * v;
            u >>= 1;
            v >>= 1;
            n >>= 1;
        }
        if (u == v) {
            size += 4;
        } else {
            size += u * v;
        }
    }
    if (surface->BitDepth == KMI_BITDEPTH_16) {
        size <<= 1;
    } else {
        size <<= 2;
    }
    return size;
}

/* the bytes of a VQ texture: the codebook, and an index byte for every 2 by 2
 * texels of every level. the indices are padded to a multiple of 32 bytes */
static KMDWORD vq_size(PKMSURFACEDESC surface)
{
    KMDWORD u = surface->u0.USize >> 1;
    KMDWORD v = surface->u1.VSize >> 1;
    KMDWORD n;
    KMDWORD size = u * v;

    if (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
        u >>= 1;
        v >>= 1;
        n = u < v ? u : v;
        while (n > 1) {
            size += u * v;
            u >>= 1;
            v >>= 1;
            n >>= 1;
        }
        if (u == v) {
            size += 2;
        } else {
            size += u * v;
        }
        size += 10;
    } else if (surface->u0.USize == 8 && surface->u1.VSize == 8) {
        size += 16;
    }
    size += CODEBOOK_SIZE;
    return size;
}

/* the bytes of a palettized texture. at 8 bits it takes a byte for every texel
 * of every level, and at 4 bits half that, with 1 kept as 1. with mipmaps,
 * the levels are padded to a multiple of 32 bytes */
static KMDWORD palette_size(PKMSURFACEDESC surface)
{
    KMDWORD u = surface->u0.USize;
    KMDWORD v = surface->u1.VSize;
    KMDWORD n;
    KMDWORD size = u * v;

    if (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
        u >>= 1;
        v >>= 1;
        n = u < v ? u : v;
        while (n > 1) {
            size += u * v;
            u >>= 1;
            v >>= 1;
            n >>= 1;
        }
        if (u == v) {
            size += 1;
        } else {
            size += u * v;
        }
        if (surface->PixelFormat == KM_PIXELFORMAT_PALETTIZED_4BPP) {
            size += 43;
        } else {
            size += 11;
        }
    }
    if (surface->PixelFormat == KM_PIXELFORMAT_PALETTIZED_4BPP) {
        /* @hack redundant: a size of 1 stored again, MOV R14,R5 at 0x8c662bb2,
         * from the 1 SHC keeps in R14 (0x8c662b30); without it, 50 of 53
         * words differ */
        if (size == 1) {
            size = 1;
        } else {
            size >>= 1;
        }
    }
    return size;
}

/* the bytes of a small VQ texture. its codebook takes small_codebook's or
 * small_codebook_mipmap's bytes for its level. its indices take a byte for
 * every 2 by 2 texels of every level, padded only with mipmaps, as vq_size
 * pads them */
static KMDWORD small_vq_size(PKMSURFACEDESC surface)
{
    KMDWORD u;
    KMDWORD v;
    KMDWORD n;
    KMDWORD size;
    int level;

    u = surface->u0.USize >> 1;
    v = surface->u1.VSize >> 1;
    size = u * v;
    level = texture_level(surface->u0.USize, surface->u1.VSize);

    if (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
        u >>= 1;
        v >>= 1;
        n = u < v ? u : v;
        while (n > 1) {
            size += u * v;
            u >>= 1;
            v >>= 1;
            n >>= 1;
        }
        if (u == v) {
            size += 2;
        } else {
            size += u * v;
        }
        size += 10;
        size += small_codebook_mipmap[level];
    } else {
        size += small_codebook[level];
    }
    return size;
}

/* load a 16-bit twiddled texture's mipmaps into the surface at offset.
 * texture's first byte goes to the surface's byte MIPMAP16_START */
static KMINT32 load_mipmapped(KMDWORD offset, PKMDWORD texture, int width)
{
    /* @hack spelling: the two addresses through locals, which has SHC add
     * MIPMAP16_START to offset ahead of the call of texture_level (ADD #4,R4
     * at 0x8c662c44); without them, 14 of 21 words differ */
    PKMDWORD source = texture;
    KMDWORD target = offset + MIPMAP16_START;
    int level = texture_level(width, width);

    /* @bug memory: the copy takes mipmap16_size[level] + MIPMAP16_START
     * bytes, the whole texture's size, from MIPMAP16_START on (ADD #4,R6 at
     * 0x8c662c60). it reads 4 bytes past the mipmaps in texture, and writes 4
     * bytes past the surface's dwTextureSize */
    return kmiCopyToFrameBuffer(target, source,
                                mipmap16_size[level] + MIPMAP16_START);
}

static KMINT32 get_mipmapped(PKMDWORD texture, KMDWORD offset, int width)
{
    /* @hack spelling: the two addresses through locals, which has SHC add
     * MIPMAP16_START to offset ahead of the call of texture_level (ADD #4,R5
     * at 0x8c662c6e); without them, 13 of 20 words differ */
    KMDWORD source = offset + MIPMAP16_START;
    PKMDWORD target = texture;
    int level = texture_level(width, width);

    return kmiCopyFromFrameBuffer(target, source, mipmap16_size[level]);
}

/* texture holds the codebook and the indices right after it. the codebook goes
 * to the surface at offset, and the indices CODEBOOK_SIZE bytes in */
static KMINT32 load_small_vq(KMDWORD offset, KMBYTE *texture, int width,
                             KMDWORD flags)
{
    /* @hack spelling: offset through target, which keeps texture in R13 and
     * codebook in R12 (MOV R5,R13 at 0x8c662ccc, MOV.L @(R0,R2),R12 at
     * 0x8c662ce8); without it, 9 of 48 words differ */
    KMDWORD target = offset;
    KMDWORD codebook;
    KMDWORD indices;
    int level = texture_level(width, width);

    if (flags & KM_SURFACEFLAGS_MIPMAPED) {
        codebook = small_codebook_mipmap[level];
        indices = vq_level_end[level] + 10;
    } else {
        codebook = small_codebook[level];
        indices = vq_level_size[level];
    }
    kmiCopyToFrameBuffer(target, (PKMDWORD)texture, codebook);
    texture += codebook;
    /* @hack spelling: the step written out, which keeps codebook in R12
     * (0x8c662ce8); without it, codebook takes R11 and 6 of 48 words differ */
    target = target + CODEBOOK_SIZE;
    return kmiCopyToFrameBuffer(target, (PKMDWORD)texture, indices);
}

/* the surface at offset holds the codebook, and the indices CODEBOOK_SIZE
 * bytes in. texture takes the codebook and the indices right after it */
static KMINT32 get_small_vq(KMBYTE *texture, KMDWORD offset, int width,
                            KMDWORD flags)
{
    /* @hack spelling: texture through target, which keeps offset in R13 and
     * codebook in R12 (MOV R5,R13 at 0x8c662d2c, MOV.L @(R0,R2),R12 at
     * 0x8c662d48); without it, 10 of 48 words differ */
    KMBYTE *target = texture;
    KMDWORD codebook;
    KMDWORD indices;
    int level = texture_level(width, width);

    if (flags & KM_SURFACEFLAGS_MIPMAPED) {
        codebook = small_codebook_mipmap[level];
        indices = vq_level_end[level] + 10;
    } else {
        codebook = small_codebook[level];
        indices = vq_level_size[level];
    }
    kmiCopyFromFrameBuffer((PKMDWORD)target, offset, codebook);
    offset += CODEBOOK_SIZE;
    /* @hack spelling: the step written out, which keeps codebook in R12
     * (0x8c662d48); without it, codebook takes R11 and 6 of 48 words differ */
    target = target + codebook;
    return kmiCopyFromFrameBuffer((PKMDWORD)target, offset, indices);
}

/* reload from texture the 4-bit level that is width texels on a side. a
 * 16-byte block the level shares with other levels goes through merge. the
 * block is read back into merge, the level's bytes are copied over their place,
 * and merge is written back whole. the levels of 4 texels or fewer share the
 * surface's first block. a larger level starts 4 bytes before the end of a
 * block and ends 12 bytes into one. its bytes between are copied straight in */
static KMSTATUS reload_palette4(PKMSURFACEDESC surface, KMBYTE *texture,
                                int width)
{
    /* @hack type: level KMDWORD, though texture_level returns an int. SHC then
     * keeps it in a frame slot (MOV.L R0,@(8,R15) at 0x8c662dd2). as an int,
     * 104 of 210 words differ */
    KMDWORD level;
    /* @hack type: from and to KMDWORD, though they hold merge's address as well
     * as the surface's offsets. SHC then loads pSurface straight into R5 for
     * kmiCopyFromFrameBuffer (MOV.L @(28,R13),R5 at 0x8c662de6); as KMBYTE *,
     * 151 of 210 words differ */
    KMDWORD from;
    KMDWORD to;
    KMBYTE merge[16];
    KMBYTE *in;
    int i;

    level = texture_level(width, width);
    if (width <= 4) {
        to = (KMDWORD)&merge[0];
        from = (KMDWORD)surface->pSurface;
        kmiCopyFromFrameBuffer((PKMDWORD)to, from, sizeof merge);
        in = texture;
        if (width == 1) {
            merge[1] = *in++;
        } else if (width == 2) {
            for (i = 0; i < 2; i++) {
                merge[2 + i] = *in++;
            }
        } else {
            for (i = 0; i < 8; i++) {
                merge[4 + i] = *in++;
            }
        }
        from = (KMDWORD)&merge[0];
        to = (KMDWORD)surface->pSurface;
        kmiPurgeCache((void *)from);
        kmiPurgeCache((void *)(from + CACHE_LINE));
        kmiCopyToFrameBuffer(to, (PKMDWORD)from, sizeof merge);
    } else {
        to = (KMDWORD)&merge[0];
        from = (KMDWORD)surface->pSurface;
        from += level4_offset[level] - 12;
        kmiCopyFromFrameBuffer((PKMDWORD)to, from, sizeof merge);
        in = texture;
        for (i = 0; i < 4; i++) {
            merge[12 + i] = *in++;
        }
        from = (KMDWORD)&merge[0];
        to = (KMDWORD)surface->pSurface;
        to += level4_offset[level] - 12;
        kmiPurgeCache((void *)from);
        kmiPurgeCache((void *)(from + CACHE_LINE));
        kmiCopyToFrameBuffer(to, (PKMDWORD)from, sizeof merge);
        if (level4_size[level] - 16 > 0) {
            to = (KMDWORD)surface->pSurface;
            to += level4_offset[level] + 4;
            from = (KMDWORD)texture + 4;
            kmiCopyToFrameBuffer(to, (PKMDWORD)from, level4_size[level] - 16);
        }
        /* @bug memory: at level 10, level4_offset[level + 1] reads past
         * the table into level4_size[0], which is 1 (0x8c662f0a, 0x8c662f46).
         * the last 16 bytes merged are then read from and written to 11 bytes
         * before the surface */
        to = (KMDWORD)&merge[0];
        from = (KMDWORD)surface->pSurface;
#ifdef GCC
        /* repairs the first read the tag names: the level ends at its offset
         * plus its size. level4_offset[level + 1] holds that end only below
         * level 10 */
        from += level4_offset[level] + level4_size[level] - 12;
#else
        from += level4_offset[level + 1] - 12;
#endif
        kmiCopyFromFrameBuffer((PKMDWORD)to, from, sizeof merge);
        in = texture + level4_size[level] - 12;
        for (i = 0; i < 12; i++) {
            merge[i] = *in++;
        }
        from = (KMDWORD)&merge[0];
        to = (KMDWORD)surface->pSurface;
#ifdef GCC
        /* repairs the second read the tag names, as above */
        to += level4_offset[level] + level4_size[level] - 12;
#else
        to += level4_offset[level + 1] - 12;
#endif
        kmiPurgeCache((void *)from);
        kmiPurgeCache((void *)(from + CACHE_LINE));
        kmiCopyToFrameBuffer(to, (PKMDWORD)from, sizeof merge);
    }
    return KMSTATUS_SUCCESS;
}

/* reload from texture the 8-bit level that is width texels on a side. the
 * levels of 2 texels or fewer lie in the surface's first 8 bytes, which go
 * through merge as in reload_palette4. a larger level is copied straight in */
static KMSTATUS reload_palette8(PKMSURFACEDESC surface, KMBYTE *texture,
                                int width)
{
    int level;
    KMDWORD length;
    /* @hack type: from and to KMDWORD, as in reload_palette4. SHC then loads
     * pSurface straight into R5 for kmiCopyFromFrameBuffer (MOV.L @(28,R12),R5
     * at 0x8c662fa2); as KMBYTE *, 32 of 81 words differ */
    KMDWORD from;
    KMDWORD to;
    KMBYTE merge[8];
    KMBYTE *in;
    int i;

    level = texture_level(width, width);
    if (width <= 2) {
        to = (KMDWORD)&merge[0];
        from = (KMDWORD)surface->pSurface;
        kmiCopyFromFrameBuffer((PKMDWORD)to, from, sizeof merge);
        if (width == 1) {
            in = texture;
            merge[3] = *in++;
        } else {
            in = texture;
            for (i = 0; i < 4; i++) {
                merge[4 + i] = *in++;
            }
        }
        from = (KMDWORD)&merge[0];
        to = (KMDWORD)surface->pSurface;
        kmiPurgeCache((void *)from);
        kmiPurgeCache((void *)(from + CACHE_LINE));
        kmiCopyToFrameBuffer(to, (PKMDWORD)from, sizeof merge);
    } else {
        length = level8_size[level];
        from = (KMDWORD)texture;
        to = (KMDWORD)surface->pSurface + level8_offset[level];
        kmiCopyToFrameBuffer(to, (PKMDWORD)from, length);
    }
    return KMSTATUS_SUCCESS;
}

/* reload from texture the 16-bit twiddled level that is width texels on a side.
 * the levels of 2 texels or fewer lie in the surface's first 16 bytes, which
 * go through merge as in reload_palette4. a larger level is copied straight
 * in */
static KMSTATUS reload_twiddled(PKMSURFACEDESC surface, KMBYTE *texture,
                                int width)
{
    int level;
    KMDWORD length;
    /* @hack type: from and to KMDWORD, as in reload_palette4. SHC then loads
     * pSurface straight into R5 for kmiCopyFromFrameBuffer (MOV.L @(28,R11),R5
     * at 0x8c66306e); as KMBYTE *, 34 of 98 words differ */
    KMDWORD from;
    KMDWORD to;
    KMBYTE merge[16];
    KMBYTE *in;
    int i;

    level = texture_level(width, width);
    if (width <= 2) {
        to = (KMDWORD)&merge[0];
        from = (KMDWORD)surface->pSurface;
        kmiCopyFromFrameBuffer((PKMDWORD)to, from, sizeof merge);
        if (width == 1) {
            in = texture;
            for (i = 0; i < 2; i++) {
                merge[6 + i] = *in++;
            }
        } else {
            in = texture;
            for (i = 0; i < 8; i++) {
                merge[8 + i] = *in++;
            }
        }
        from = (KMDWORD)&merge[0];
        to = (KMDWORD)surface->pSurface;
        kmiPurgeCache((void *)from);
        kmiPurgeCache((void *)(from + CACHE_LINE));
        kmiCopyToFrameBuffer(to, (PKMDWORD)from, sizeof merge);
    } else {
        length = level16_size[level];
        from = (KMDWORD)texture;
        to = (KMDWORD)surface->pSurface + level16_offset[level];
        kmiCopyToFrameBuffer(to, (PKMDWORD)from, length);
    }
    return KMSTATUS_SUCCESS;
}

/* reload from texture the indices of the VQ level that is width texels on a
 * side, small VQ included. a level of 1 texel has no index byte, and is left
 * as it is. the indices go through merge as in reload_palette4, in 16-byte
 * blocks counted from CODEBOOK_SIZE. the levels of 4 texels or fewer share the
 * first block. a larger level's indices start 10 bytes before the end of a
 * block and end 6 bytes into one. those between are copied straight in */
static KMSTATUS reload_vq(PKMSURFACEDESC surface, KMBYTE *texture, int width)
{
    int level;
    /* @hack type: from and to KMDWORD, as in reload_palette4. SHC then loads
     * pSurface straight into R5 for kmiCopyFromFrameBuffer (MOV.L @(28,R13),R5
     * at 0x8c663180); as KMBYTE *, 169 of 205 words differ */
    KMDWORD from;
    KMDWORD to;
    KMBYTE merge[16];
    KMBYTE *in;
    int i;

    level = texture_level(width, width);
    if (width != 1) {
        if (width <= 4) {
            to = (KMDWORD)&merge[0];
            from = (KMDWORD)surface->pSurface;
            from += CODEBOOK_SIZE;
            kmiCopyFromFrameBuffer((PKMDWORD)to, from, sizeof merge);
            in = texture;
            if (width == 2) {
                merge[1] = *in++;
            } else {
                for (i = 0; i < 4; i++) {
                    merge[2 + i] = *in++;
                }
            }
            from = (KMDWORD)&merge[0];
            to = (KMDWORD)surface->pSurface;
            to += CODEBOOK_SIZE;
            kmiPurgeCache((void *)from);
            kmiPurgeCache((void *)(from + CACHE_LINE));
            kmiCopyToFrameBuffer(to, (PKMDWORD)from, sizeof merge);
        } else {
            to = (KMDWORD)&merge[0];
            from = (KMDWORD)surface->pSurface;
            from += CODEBOOK_SIZE + vq_level_end[level - 1] - 6;
            kmiCopyFromFrameBuffer((PKMDWORD)to, from, sizeof merge);
            in = texture;
            for (i = 0; i < 10; i++) {
                merge[6 + i] = *in++;
            }
            from = (KMDWORD)&merge[0];
            to = (KMDWORD)surface->pSurface;
            to += CODEBOOK_SIZE + vq_level_end[level - 1] - 6;
            kmiPurgeCache((void *)from);
            kmiPurgeCache((void *)(from + CACHE_LINE));
            kmiCopyToFrameBuffer(to, (PKMDWORD)from, sizeof merge);
            if (vq_level_size[level] - 16 > 0) {
                to = (KMDWORD)surface->pSurface;
                to += CODEBOOK_SIZE + vq_level_end[level - 1] + 10;
                from = (KMDWORD)texture + 10;
                kmiCopyToFrameBuffer(to, (PKMDWORD)from,
                                     vq_level_size[level] - 16);
            }
            to = (KMDWORD)&merge[0];
            from = (KMDWORD)surface->pSurface;
            from += CODEBOOK_SIZE + vq_level_end[level] - 6;
            kmiCopyFromFrameBuffer((PKMDWORD)to, from, sizeof merge);
            in = texture + vq_level_size[level] - 6;
            for (i = 0; i < 6; i++) {
                merge[i] = *in++;
            }
            from = (KMDWORD)&merge[0];
            to = (KMDWORD)surface->pSurface;
            to += CODEBOOK_SIZE + vq_level_end[level] - 6;
            kmiPurgeCache((void *)from);
            kmiPurgeCache((void *)(from + CACHE_LINE));
            kmiCopyToFrameBuffer(to, (PKMDWORD)from, sizeof merge);
        }
    }
    return KMSTATUS_SUCCESS;
}

/* the 3-bit level nearest value, ties going up. a level below 4 stands for 32
 * times it, and the rest for 32 times it plus 31 */
static int nearest_3_bits(KMBYTE value)
{
    int band;

    if (value < 128) {
        band = 3;
    } else {
        band = 4;
    }
    if (value < 112) {
        band = (value + 16) >> 5;
    }
    if (value > 143) {
        band = (value - 15) >> 5;
    }
    return band;
}
