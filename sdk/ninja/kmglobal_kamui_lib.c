/* Kamui global render settings */
#include <kamui.h>
#include <sg_xpt.h>

/* the video chip's registers, as offsets into its register block */
#define BORDER_COLOR 0x040
#define FB_READ_CONTROL 0x044
#define FB_WRITE_CONTROL 0x048
#define FB_READ_SIZE 0x05c
#define CULL_VALUE 0x078
#define FOG_TABLE_COLOR 0x0b0
#define FOG_VERTEX_COLOR 0x0b4
#define FOG_DENSITY 0x0b8
#define FOG_CLAMP_MAX 0x0bc
#define FOG_CLAMP_MIN 0x0c0
#define GUN_POSITION 0x0c4
#define PALETTE_FORMAT 0x108
#define FOG_TABLE 0x200
#define PALETTE 0x1000

#define FOG_ENTRIES 128

/* FB_WRITE_CONTROL's alpha threshold, bits 16 to 23, and the largest
 * threshold, all 8 of its bits set */
#define ALPHA_THRESHOLD 0x00ff0000
#define THRESHOLD_MAX 0xff

/* the largest clip coordinate, in pixels */
#define CLIP_MAX 2047

/* a float's bits but its sign */
#define FLOAT_MAGNITUDE 0x7fffffff

/* regions_stale's bits for both banks */
#define BOTH_BANKS 3

/* clamp nThreshold from 0 to THRESHOLD_MAX and keep it in alpha_threshold.
 * fb_write_control then has ALPHA_THRESHOLD cleared and nThreshold ORed in
 * from bit 15, and goes to FB_WRITE_CONTROL */
KMSTATUS kmSetAlphaThreshold(KMINT32 nThreshold)
{
    KMDWORD control;

    if (nThreshold & 0x80000000) {
        nThreshold = 0;
    }
    if (nThreshold & 0x7fffff00) {
        nThreshold = THRESHOLD_MAX;
    }
    kmiDeviceExtension.alpha_threshold = nThreshold;
    control = kmiDeviceExtension.fb_write_control & ~ALPHA_THRESHOLD;
    /* @bug logic: the threshold is shifted by 15, not 16, into bits 15 to
     * 22 (MOV #15,R3 and SHLD R3,R4 at 0x8c62187e and 0x8c621882 in
     * 1ST_READ.BIN, 0x8ce0f75a and 0x8ce0f75c in LOADER.BIN). ALPHA_THRESHOLD
     * then gets nThreshold >> 1, and an odd nThreshold also sets bit 15 */
    control |= (nThreshold & THRESHOLD_MAX) << 15;
    kmiDeviceExtension.fb_write_control = control;
    kmiWriteRegister(FB_WRITE_CONTROL, control);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetCullingRegister(KMFLOAT fCullVal)
{
    union {
        KMFLOAT f;
        KMDWORD d;
    } bits;

    bits.f = fCullVal;
    /* @hack spelling: the bits read through the union's address, which SHC
     * copies to R5 before the load (MOV R15,R5 and MOV.L @R5,R5 at 0x8c62189e
     * and 0x8c6218a6 in 1ST_READ.BIN, 0x8ce0f77a and 0x8ce0f77e in LOADER.BIN);
     * as bits.d, 12 of 14 words differ in 1ST_READ.BIN and 10 of 14 in
     * LOADER.BIN, and the function is 2 bytes shorter in each */
    kmiWriteRegister(CULL_VALUE, (&bits)->d & FLOAT_MAGNITUDE);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetColorClampValue(KMPACKEDARGB MaxVal, KMPACKEDARGB MinVal)
{
    kmiWriteRegister(FOG_CLAMP_MAX, MaxVal.dwPacked);
    kmiWriteRegister(FOG_CLAMP_MIN, MinVal.dwPacked);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetFogTableColor(KMPACKEDARGB FogTableColor)
{
    kmiWriteRegister(FOG_TABLE_COLOR, FogTableColor.dwPacked);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetFogVertexColor(KMPACKEDARGB FogVertexColor)
{
    kmiWriteRegister(FOG_VERTEX_COLOR, FogVertexColor.dwPacked);
    return KMSTATUS_SUCCESS;
}

/* fill FOG_TABLE from pfFogTable's densities, each from 0 to 1, scaled by 255
 * to a byte and truncated. entry i holds density i in its high byte and density
 * i + 1 in its low byte. the last entry holds its own density in both */
/* NOLINTNEXTLINE(readability-non-const-parameter) */
KMSTATUS kmSetFogTable(PKMFLOAT pfFogTable)
{
    int i;
    KMDWORD next;
    KMDWORD density;
    KMDWORD entry;

    i = FOG_ENTRIES;
    density = (KMINT32)(pfFogTable[i - 1] * 255.0F) & 0xff;
    do {
        --i;
        next = density;
        density = (KMINT32)(pfFogTable[i] * 255.0F) & 0xff;
        /* @hack spelling: entry, a variable of its own, which lets SHC hold
         * next in R4 (MOV R13,R4 at 0x8c621960) and save one register fewer;
         * computed in the call, next takes R11, R8 is saved too, LOADER.BIN's
         * function changes size, and 46 of 52 words differ in 1ST_READ.BIN */
        entry = (next | (density << 8)) & 0xffff;
        kmiWriteRegister(FOG_TABLE + (i * 4), entry);
    } while (i != 0);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetFogDensity(KMDWORD fogDensity)
{
    kmiWriteRegister(FOG_DENSITY, fogDensity);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetPaletteMode(KMINT32 Palettemode)
{
    KMSTATUS status = KMSTATUS_SUCCESS;

    /* @hack spelling: an if with an empty body and the stores in its else,
     * which puts palette_mode's address in R2 (0x8c6219ec in 1ST_READ.BIN,
     * 0x8ce0f854 in LOADER.BIN); written as if (!(Palettemode & ~3)) with no
     * else, SHC puts it in R1, and 2 of 16 words differ in each image */
    if (Palettemode & ~3) {
    } else {
        kmiDeviceExtension.palette_mode = Palettemode;
        kmiWriteRegister(PALETTE_FORMAT, Palettemode);
    }
    return status;
}

KMSTATUS kmSetPaletteData(PKMPALETTEDATA pPaletteTable)
{
    int i;
    /* @hack spelling: the bound through a local, which keeps one call to a pass
     * (0x8c621a16 to 0x8c621a22); with KM_PALETTE_ENTRY in the test, SHC
     * unrolls the loop to two calls a pass in 1ST_READ.BIN, and 16 of 26 words
     * differ there and 3 of 26 in LOADER.BIN */
    int count = KM_PALETTE_ENTRY;

    for (i = 0; i < count; i++) {
        kmiWriteRegister(PALETTE + (i * 4), pPaletteTable->dwPaletteData[i]);
    }
    return KMSTATUS_SUCCESS;
}

/* write DataSize colors from entry PaletteEntry on. a range that runs past the
 * palette's end stops at entry 1022, one short of the last */
KMSTATUS kmSetPaletteBankData(KMINT32 PaletteEntry, KMINT32 DataSize,
                              PKMPALETTEDATA pPaletteTable)
{
    int i;
    KMDWORD reg;
    KMDWORD *color;
    int end;

    DataSize += PaletteEntry;
    end = DataSize > KM_PALETTE_ENTRY ? KM_PALETTE_ENTRY - 1 : DataSize;
    i = PaletteEntry;
    reg = PALETTE + (PaletteEntry * 4);
    color = &pPaletteTable->dwPaletteData[PaletteEntry];
    /* @hack spelling: an if around a do, not a while, which keeps
     * kmiWriteRegister's address loaded after the first test; as a while
     * loop, SHC loads it ahead (0x8c621a88), and 7 of 41 words differ in
     * 1ST_READ.BIN and 8 of 41 in LOADER.BIN */
    if (i < end) {
        do {
            kmiWriteRegister(reg, *color);
            color++;
            i++;
            reg += 4;
        } while (i < end);
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetBorderColor(KMPACKEDARGB BorderColor)
{
    kmiWriteRegister(BORDER_COLOR, BorderColor.dwPacked);
    return KMSTATUS_SUCCESS;
}

/* a color's four floats, each from 0 to 1, as the bytes of a word, alpha
 * highest. each byte is 255 times its float, truncated. none is masked to its
 * byte */
#define PACK_ARGB(a, r, g, b)                                                  \
    ((KMDWORD)(((KMINT32)((a) * 255.0F) << 24) |                               \
               ((KMINT32)((r) * 255.0F) << 16) |                               \
               ((KMINT32)((g) * 255.0F) << 8) | (KMINT32)((b) * 255.0F)))

KMSTATUS kmSetBackGroundRenderState(PKMVERTEXCONTEXT pVertexContext)
{
    kmiDeviceExtension.background_base_color = PACK_ARGB(
        pVertexContext->fFaceColorAlpha, pVertexContext->fFaceColorRed,
        pVertexContext->fFaceColorGreen, pVertexContext->fFaceColorBlue);
    kmiDeviceExtension.background_offset_color = PACK_ARGB(
        pVertexContext->fOffsetColorAlpha, pVertexContext->fOffsetColorRed,
        pVertexContext->fOffsetColorGreen, pVertexContext->fOffsetColorBlue);
    kmiDeviceExtension.background_isp = pVertexContext->ISPPARAMBUFFER;
    kmiDeviceExtension.background_tsp = pVertexContext->TSPPARAMBUFFER;
    kmiDeviceExtension.background_texture = pVertexContext->TexturePARAMBUFFER;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetBackGroundPlane(PVOID pVertex[3], KMVERTEXTYPE VertexType,
                              KMINT32 StructSize)
{
    KMSTATUS status;

    status = kmiBackGroundPlaneFuncs[VertexType](pVertex);
    kmiDeviceExtension.background_packet[0] &= 0xfff00000;
    return status;
}

/* turn the automatic sorting of translucent polygons on or off. presort holds
 * the opposite of fEnable. a change marks both banks' region arrays for
 * rebuilding. the status returned is undefined */
KMSTATUS kmSetAutoSortMode(KMBOOLEAN fEnable)
{
    KMSTATUS status;

    if (kmiDeviceExtension.presort == fEnable) {
        kmiDeviceExtension.regions_stale = BOTH_BANKS;
        if (fEnable == TRUE) {
            kmiDeviceExtension.presort = FALSE;
        } else {
            kmiDeviceExtension.presort = TRUE;
        }
    }
#ifdef GCC
    /* repairs the unset status the tag below names: GCC returns
     * KMSTATUS_SUCCESS, as nothing fails */
    status = KMSTATUS_SUCCESS;
#endif
    /* @bug uninitialized: no path sets status, and every path returns the
     * stack pointer, R15 (MOV R15,R0 in RTS's slot, at 0x8c621bc8 in
     * 1ST_READ.BIN and 0x8ce0fa46 in LOADER.BIN) */
    /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
    return status;
}

KMSTATUS kmSetGlobalClipping(KMINT32 nWidth, KMINT32 nHeight)
{
    kmiDeviceExtension.global_clip = TRUE;
    kmiDeviceExtension.global_clip_width = nWidth;
    kmiDeviceExtension.global_clip_height = nHeight;
    kmiDeviceExtension.regions_stale = BOTH_BANKS;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetPixelClipping(KMINT32 Xmin, KMINT32 Ymin, KMINT32 Xmax,
                            KMINT32 Ymax)
{
    KMDWORD clip;

    if (Xmin > Xmax) {
        return KMSTATUS_INVALID_PARAMETER;
    }
    if (Ymin > Ymax) {
        return KMSTATUS_INVALID_PARAMETER;
    }
    if (Xmax > CLIP_MAX) {
        Xmax = CLIP_MAX;
    }
    if (Ymax > CLIP_MAX) {
        Ymax = CLIP_MAX;
    }
    if (kmiDeviceExtension.bpp == KM_DSPBPP_RGB888) {
        Xmin = (Xmin + 1) & ~1;
        Ymin = (Ymin + 1) & ~1;
        Xmax &= ~1;
        Ymax &= ~1;
    }
    clip = ((Xmax & CLIP_MAX) << 16) | (Xmin & CLIP_MAX);
    kmiDeviceExtension.fb_clip_x = clip;
    clip = ((Ymax & CLIP_MAX) << 16) | (Ymin & CLIP_MAX);
    kmiDeviceExtension.fb_clip_y = clip;
    return KMSTATUS_SUCCESS;
}

/* skip lines at the top of the frame buffer: none for KM_LEVEL_ADJUST_NORMAL,
 * and for KM_LEVEL_ADJUST_HALF 16 on VGA or at 480 lines, otherwise 8. each
 * field's display read starts that many lines down. FB_READ_SIZE's line field
 * gets fb_lines less skip_lines or offset_y, whichever is greater. a difference
 * that wraps below 0 also becomes fb_lines. *pLine, if not null, gets
 * skip_lines */
KMSTATUS kmSetUserClipLevelAdjust(KMADJUSTTYPE Adjust, PKMINT32 pLine)
{
    KMDWORD offset;
    KMDWORD size;
    KMDWORD lines;

    if (kmiDeviceExtension.level_adjust == KM_LEVEL_ADJUST_HALF) {
        offset = kmiDeviceExtension.skip_lines *
                 kmiDeviceExtension.fb_write_stride * 8;
        kmiDeviceExtension.field_offset[0] =
            kmiDeviceExtension.field_offset[0] - offset;
        kmiDeviceExtension.field_offset[1] =
            kmiDeviceExtension.field_offset[1] - offset;
    }
    switch (Adjust) {
    case KM_LEVEL_ADJUST_NORMAL:
        kmiDeviceExtension.skip_lines = 0;
        break;
    case KM_LEVEL_ADJUST_HALF:
        /* @hack spelling: VGA_MODE compared with 1, which keeps the image's
         * compare of the 0 or 1 it computes (CMP/EQ #1,R0 at 0x8c621d32);
         * tested bare, LOADER.BIN's function changes size, and 51 of 148 words
         * differ in 1ST_READ.BIN */
        if (VGA_MODE == 1 ||
            (kmiDeviceExtension.display_mode & KM_DSPMODE_HEIGHT_480)) {
            kmiDeviceExtension.skip_lines = 16;
        } else {
            kmiDeviceExtension.skip_lines = 8;
        }
        offset = kmiDeviceExtension.skip_lines *
                 kmiDeviceExtension.fb_write_stride * 8;
        kmiDeviceExtension.field_offset[0] =
            kmiDeviceExtension.field_offset[0] + offset;
        kmiDeviceExtension.field_offset[1] =
            kmiDeviceExtension.field_offset[1] + offset;
        break;
    }
    size = kmiDeviceExtension.fb_read_size & ~(READ_LINES << READ_LINES_SHIFT);
    lines = kmiDeviceExtension.fb_lines;
    if (kmiDeviceExtension.skip_lines < kmiDeviceExtension.offset_y) {
        lines -= kmiDeviceExtension.offset_y;
    } else {
        lines -= kmiDeviceExtension.skip_lines;
    }
    if (lines + kmiDeviceExtension.offset_y > kmiDeviceExtension.fb_lines) {
        lines = kmiDeviceExtension.fb_lines - kmiDeviceExtension.skip_lines;
    }
    if (kmiDeviceExtension.fb_lines < lines) {
        kmiDeviceExtension.fb_lines = lines;
    }
    kmiDeviceExtension.fb_read_size =
        size | ((lines & READ_LINES) << READ_LINES_SHIFT);
    /* frame buffer reads are stopped while the read size changes */
    kmiWriteRegister(FB_READ_CONTROL, 0);
    kmiWriteRegister(FB_READ_SIZE, kmiDeviceExtension.fb_read_size);
    kmiWriteRegister(FB_READ_CONTROL, kmiDeviceExtension.fb_read_control);
    kmiDeviceExtension.level_adjust = Adjust;
    if (pLine != NULL) {
        *pLine = kmiDeviceExtension.skip_lines;
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetForwardClipping(KMFLOAT fDepth)
{
    return KMSTATUS_NOT_IMPLEMENTED;
}

KMSTATUS kmResetRenderer(void)
{
    kmiDiscardRenderPipeline();
    kmiResetBoard();
    return KMSTATUS_SUCCESS;
}

/* have the region arrays point both modifier lists of each entry at list
 * kmModifier, which is to be KM_OPAQUE_MODIFIER or KM_TRANS_MODIFIER.
 * kmiCreateRegionArray takes any other odd type as KM_TRANS_MODIFIER. an even
 * type turns this off and returns KMSTATUS_INVALID_LIST_TYPE. either way, both
 * banks' region arrays are marked for rebuilding */
KMSTATUS kmUseAnotherModifier(KMINT32 kmModifier)
{
    KMSTATUS status = KMSTATUS_INVALID_LIST_TYPE;

    if (kmModifier & 1) {
        kmiDeviceExtension.another_modifier = TRUE;
        kmiDeviceExtension.another_modifier_list = kmModifier;
        status = KMSTATUS_SUCCESS;
    } else {
        kmiDeviceExtension.another_modifier = FALSE;
    }
    kmiDeviceExtension.regions_stale = BOTH_BANKS;
    return status;
}

KMSTATUS kmSetWaitVsyncCount(KMINT32 nCount)
{
    kmiDeviceExtension.vsync_wait = nCount - 1;
    return KMSTATUS_SUCCESS;
}

/* the screen position at which the light gun last detected the beam */
KMSTATUS kmGetGunTriggerPos(PKMDWORD pHPos, PKMDWORD pVPos)
{
    KMDWORD position;

    position = kmiReadRegister(GUN_POSITION);
    *pHPos = position & 0x3ff;
    *pVPos = (position >> 16) & 0x3ff;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmQueryFinishLastTextureDMA(void)
{
    if (kmiDeviceExtension.frame_dma_wait == TRUE) {
        return KMSTATUS_SUCCESS;
    }
    if (kmiQueryFinishDMA(kmiDeviceExtension.frame_dma) == TRUE) {
        return KMSTATUS_SUCCESS;
    }
    return KMSTATUS_NOT_FINISH_DMA;
}
