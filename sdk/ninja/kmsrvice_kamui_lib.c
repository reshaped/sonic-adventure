/* Kamui's service calls: the vertex buffer's base, the internal metrics, the
 * timeouts, the YUV converter and the common modifier */
#include <kamui.h>
#include <kmistat.h>
#include <sg_xpt.h>

/* the video chip's registers, as offsets into its register block */
#define TA_YUV_BASE 0x148
#define TA_YUV_CONTROL 0x14c

/* TA_YUV_CONTROL's fields. the last macroblock column is in bits 0 to 5, and
 * the last row in bits 8 to 13. bit 16 selects a run of textures rather than
 * one, and bit 24 a 4:2:2 source rather than 4:2:0 */
#define YUV_LAST 0x3f
#define YUV_ROW_SHIFT 8
#define YUV_TEXTURES 0x00010000
#define YUV_422 0x01000000

/* the last column and row fields of a run across by down macroblocks */
#define YUV_SIZE(across, down)                                                 \
    ((((down) - 1) & YUV_LAST) << YUV_ROW_SHIFT | (((across) - 1) & YUV_LAST))

#define SLOTS 4
#define LISTS 5 /* a slot's lists, one for each ListType */

KMSTATUS kmfSetVertexBufferBaseAddress(KMDWORD dwBaseAddress,
                                       KMDWORD dwVertexBufferSize)
{
    kmiDeviceExtension.vertex_base = dwBaseAddress;
    kmiDeviceExtension.vertex_size = dwVertexBufferSize;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmfGetInternalSystemMetrics(PKMISYSTEMMETRICS pSystemMetrics)
{
    int i;

    for (i = 0; i < 2; i++) {
        pSystemMetrics->NativeParams[1 - i].NativeITPBase =
            kmiDeviceExtension.ta_bank[i].isp_base;
        pSystemMetrics->NativeParams[1 - i].NativeOBJBase =
            kmiDeviceExtension.ta_bank[i].ol_base;
        pSystemMetrics->NativeParams[1 - i].NativeRegBase =
            kmiDeviceExtension.ta_bank[i].region_base;
        pSystemMetrics->NativeParams[1 - i].NativeITPLimit =
            kmiDeviceExtension.ta_bank[i].isp_limit;
        pSystemMetrics->NativeParams[1 - i].NativeOBJLimit =
            kmiDeviceExtension.ta_bank[i].ol_limit;
        pSystemMetrics->NativeParams[1 - i].NativeLastITPCurrent =
            kmiIntStatisticData.isp_current[i];
        pSystemMetrics->FBParams.FBLineStride[1 - i] =
            kmiDeviceExtension.field_offset[i];
    }
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V) {
        pSystemMetrics->FBParams.FBStartAddr[0] =
            kmiDeviceExtension.fb_start[kmiDeviceExtension.write_slot & 1];
        pSystemMetrics->FBParams.FBStartAddr[1] =
            kmiDeviceExtension
                .fb_start[1 - (kmiDeviceExtension.write_slot & 1)];
    } else {
        pSystemMetrics->FBParams.FBStartAddr[1] =
            kmiDeviceExtension.fb_start[kmiDeviceExtension.write_slot & 1];
        pSystemMetrics->FBParams.FBStartAddr[0] =
            kmiDeviceExtension
                .fb_start[1 - (kmiDeviceExtension.write_slot & 1)];
    }
    pSystemMetrics->FBParams.XClip = kmiDeviceExtension.fb_clip_x;
    pSystemMetrics->FBParams.YClip = kmiDeviceExtension.fb_clip_y;
    pSystemMetrics->RenderTimeOutCount = kmiIntStatisticData.render_timeouts;
    pSystemMetrics->VertexTimeOutCount = kmiIntStatisticData.ta_timeouts;
    pSystemMetrics->OBJOverflowCount = kmiIntStatisticData.ol_overflows;
    pSystemMetrics->ITPOverflowCount = kmiIntStatisticData.isp_overflows;
    for (i = 0; i < LISTS; i++) {
        /* @hack type: list i's start in slot 0 read as
         * list_start.word[i * SLOTS], through the union's twenty words, which
         * hold four slots for each list. SHC then loads it with @(R0,R6) from
         * the record's 0x378 (0x8c622156); as list_start.row[i][0], and slot
         * 1's as row[i][1], 40 of 229 words differ and the function is 2 bytes
         * longer */
        pSystemMetrics->DMADataAddress[0][i] =
            (KMDWORD)kmiDeviceExtension.list_start.word[i * SLOTS];
        /* @hack type: the start in slot 1 read through
         * &list_start.word[1], cast to its own type, which SHC addresses with
         * ADD #4,R0 and @(R0,R6) (0x8c62215a, 0x8c622160); uncast, as
         * list_start.word[i * SLOTS + 1], 36 of 229 words differ and the
         * function is 2 bytes longer */
        /* NOLINTBEGIN(readability-redundant-casting) */
        pSystemMetrics->DMADataAddress[1][i] =
            (KMDWORD)((PKMDWORD volatile *)&kmiDeviceExtension.list_start
                          .word[1])[i * SLOTS];
        /* NOLINTEND(readability-redundant-casting) */
        pSystemMetrics->InternalVertexBufSize[i] =
            kmiDeviceExtension.list_limit[i];
        pSystemMetrics->MaxVertexBufSize[i] = kmiIntStatisticData.list_max[i];
    }
    pSystemMetrics->LastTATime = kmiIntStatisticData.ta_time;
    pSystemMetrics->LastRenderTime = kmiIntStatisticData.render_time;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmfSetTimeOutCount(KMDWORD nVertexTimeOut, KMDWORD nRenderTimeOut)
{
    KMDWORD revision;

    kmiDeviceExtension.vertex_timeout = nVertexTimeOut;
    kmiDeviceExtension.render_timeout = nRenderTimeOut;
    if (nRenderTimeOut == 0) {
        kmiDeviceExtension.capabilities |= KMI_CAP_RESET_BEFORE_RENDER;
    } else {
        /* @hack spelling: the switch on revision, a copy of BUS_REVISION,
         * which SHC keeps in R5 (MOV R0,R5 at 0x8c622228); switched on
         * BUS_REVISION itself, 32 of 93 words differ and the function is 2
         * bytes shorter */
        revision = BUS_REVISION;
        switch (revision) {
        case 4:
        case 5:
        case 6:
        case 8:
            kmiDeviceExtension.capabilities |= KMI_CAP_RESET_BEFORE_RENDER;
            break;
        /* @hack redundant: case labels for revisions 0, 1, 2 and 9, which share
         * the default's code. SHC still compares the revision with each
         * (CMP/EQ #0,R0 at 0x8c622224 to CMP/EQ #9,R0 at 0x8c622242); without
         * them, count unmeasured: the function changes size */
        case 0:
        case 1:
        case 2:
        case 9:
        default:
            kmiDeviceExtension.capabilities &= ~KMI_CAP_RESET_BEFORE_RENDER;
            break;
        }
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmfInitializeYUVConverter(PKMSURFACEDESC pSurface, KMDWORD TextureType,
                                   KMDWORD ConvertMode, KMDWORD xMBSize,
                                   KMDWORD yMBSize)
{
    KMDWORD form;
    KMDWORD control;

    form = 0;
    if (TextureType == KM_TEXTURE_YUV422) {
        form = YUV_422;
    }
    /* @hack spelling: YUV_SIZE written in each branch, which SHC computes once,
     * above the test (0x8c622274 to 0x8c622288); computed into a local before
     * the test, 13 of 36 words differ */
    if (ConvertMode == KMF_YUVCONVERT_ONE_TEXTURE) {
        control = YUV_SIZE(xMBSize, yMBSize) | form;
    } else {
        control = YUV_SIZE(xMBSize, yMBSize) | form | YUV_TEXTURES;
    }
    kmiWriteRegister(TA_YUV_CONTROL, control);
    kmiWriteRegister(TA_YUV_BASE, (KMDWORD)pSurface->pSurface);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmfSetCommonModifier(PKMDWORD pCommonBuffer, KMDWORD BufferSize)
{
    /* @hack redundant: an else after the branch that returns, which puts
     * common_modifier's address in R3 and TRUE in R2 (0x8c6222ea,
     * 0x8c6222ec); without it, the two swap registers and 7 of 45 words
     * differ */
    if (BufferSize == 0) {
        kmiDeviceExtension.common_modifier = FALSE;
        return KMSTATUS_SUCCESS;
    } else {
        kmiDeviceExtension.common_modifier = TRUE;
    }
    if (((KMDWORD)pCommonBuffer & 0x1f) != 0 || (BufferSize & 0x1f) != 0) {
        kmiDeviceExtension.common_modifier = FALSE;
        return KMSTATUS_INVALID_ADDRESS;
    }
    kmiDeviceExtension.common_modifier_data = pCommonBuffer;
    kmiDeviceExtension.common_modifier_size = BufferSize;
    return KMSTATUS_SUCCESS;
}
