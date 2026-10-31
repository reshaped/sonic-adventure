/* SDK 1.20J Kamui's vertex buffers, and the system configuration that makes
 * them and the tile accelerator's two buffers. a vertex buffer holds the lists
 * in two banks, or in one under KM_LATENCYMODEL_2V */
#include <kamui.h>
#include <sg_xpt.h>

#include "loader/sdk/fballoc.h"

/* the tile accelerator's polygon input. kmFlushVertexBuffer sends a list there
 * by DMA, and the list a NOBUF model sends with no buffer takes it as its
 * base */
#define TA_INPUT 0x10000000UL

/* the video chip's registers, as the offsets kmiWriteRegister takes */
#define SOFT_RESET 0x008
#define TA_OL_BASE 0x124
#define TA_ISP_BASE 0x128
#define TA_OL_LIMIT 0x12c
#define TA_ISP_LIMIT 0x130
#define TA_LIST_INIT 0x144
#define TA_NEXT_OPB_INIT 0x164

/* a list's bit in lists_used and fActiveList, by its ListType */
#define LIST_BIT(list) (1 << (list))

/* a polygon header's parameter type in its control word, and the shift of its
 * ListType there */
#define PCW_POLYGON 0x80000000
#define PCW_LIST_SHIFT 24

/* the TSP word of the header kmiCopyDummyGlobalData opens a list with: no fog,
 * and a D adjust of KM_MIPMAP_D_ADJUST_1_00 */
#define HEADER_TSP                                                             \
    (((KMDWORD)KM_NOFOG << 22) | ((KMDWORD)KM_MIPMAP_D_ADJUST_1_00 << 8))

/* the words of kmiDummyOpaque, which kmRender can write into the opaque list */
#define DUMMY_OPAQUE_WORDS (sizeof(struct dummy_polygon) / sizeof(KMDWORD))
/* the words each list's room in a bank keeps past its limit.
 * kmCreateVertexBuffer and kmSetSystemConfiguration take them off each size,
 * and kmiGetVertexBasePointer gives them back past each list */
#define LIST_SPARE_WORDS 16
/* on a chip with KMI_CAP_DUMMY_OPAQUE, kmSetSystemConfiguration grows an
 * opaque share below this many words by as many. less LIST_SPARE_WORDS, the
 * list's limit then holds its header and kmiDummyOpaque */
#define DUMMY_OPAQUE_SHARE                                                     \
    (LIST_HEADER_WORDS + DUMMY_OPAQUE_WORDS + LIST_SPARE_WORDS)

/* a list's object pointer block size code: no blocks, or blocks of 8, 16 or
 * 32 words */
#define OPB_NONE 0
#define OPB_8 1
#define OPB_16 2
#define OPB_32 3

/* a list's size code as its field of the options word, and the code the
 * options word holds for a list */
#define OPB_FIELD(code, list)                                                  \
    (((code) << OPB_SHIFT(list)) & (3UL << OPB_SHIFT(list)))
#define OPB_CODE(options, list) (((options) >> OPB_SHIFT(list)) & 3)

struct kmi_system_config kmiSystemConfig;
KMCURRENTLISTSTATE kmiCurrentListState;
PKMDWORD *kmiVertexBuff;
PKMDWORD *kmiCurrentVertexBuff;
PKMDWORD *kmiDstVertexBuffer;
PKMDWORD *kmi_banks[2];
PKMDWORD kmi_list_bases[2][5];
PKMDWORD kmi_write_pointers[5];
PKMDWORD kmi_list_starts[5];
KMDWORD kmiCurrentListType;
/* the tile accelerator's two buffers, as the last kmCreateVertexBuffer or
 * kmCreateTABuffer allocated them, for kmDiscardVertexBuffer to free */
static KMDWORD ta_buffers[2];
/* how many header words each list starts with, by ListType: LIST_HEADER_WORDS
 * for a list kmiCopyDummyGlobalData opens, else 0. a list's write pointer
 * starts past them */
static KMDWORD header_words[5];
/* the KMI_CONFIG_ bit of each request recorded so far, or KMI_CONFIGURED once
 * the configuration is made */
static KMDWORD config_flags = 0;
/* 1 while the lists go straight to the tile accelerator, as kmCreateTABuffer
 * and KM_CONFIGFLAG_USEDIRECTMODE send them, else 0 */
static KMDWORD direct_mode = 0;

KMDWORD kmiChkActiveListType(KMDWORD opaque, KMDWORD opaque_modifier,
                             KMDWORD translucent, KMDWORD translucent_modifier,
                             KMDWORD punchthrough);
KMDWORD kmiGetPackOBPParam(KMDWORD opaque, KMDWORD opaque_modifier,
                           KMDWORD translucent, KMDWORD translucent_modifier,
                           KMDWORD punchthrough, KMDWORD direct);
void kmiGetNativeRegOBPSize(KMDWORD options);
void kmiCopyDummyGlobalData(KMDWORD bank_count);
KMSTATUS kmiGetVertexBasePointer(KMDWORD opaque, KMDWORD opaque_modifier,
                                 KMDWORD translucent,
                                 KMDWORD translucent_modifier,
                                 KMDWORD punchthrough, KMDWORD words,
                                 KMDWORD bank_count);
PKMDWORD *kmiGetDstVertexBuffer(void);

/* the address of the current list's write pointer, kmiCurrentVertexBuff's
 * entry for kmiCurrentListType */
PKMDWORD *kmiGetDstVertexBuffer(void)
{
    return &kmiCurrentVertexBuff[kmiCurrentListType];
}

KMSTATUS kmCreateVertexBuffer(PKMVERTEXBUFFDESC pBufferDesc,
                              KMINT32 OpaquePolygonBuffer,
                              KMINT32 OpaqueModifierBuffer,
                              KMINT32 TransPolygonBuffer,
                              KMINT32 TransModifierBuffer)
{
    KMDWORD i;
    KMDWORD total;
    KMDWORD lists;
    KMDWORD words;
    KMDWORD options;
    KMDWORD bank_count = 2;
    KMDWORD available;
    KMSTATUS status = KMSTATUS_SUCCESS;
    KMDWORD sizes[4];
    /* @hack redundant: rest, set to &sizes[1] and never read. SHC then starts
     * the loop's pointer as MOV R4,R6 and ADD #4,R6 (0x8ce125ea), and stores
     * it at @R15 (0x8ce125ee); without it, 2 of 348 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    KMDWORD *rest;
    /* @hack redundant: the table at 0x8ce1ff8c, which the image copies to the
     * frame (its copy's JSR at 0x8ce12528) and never reads; without it, 345 of
     * 348 words differ */
    KMDWORD opb_words[4] = {0, 8, 16, 32};

    kmiSystemConfig.buffer_mode = KMI_BUFFERS_VERTEX;
    if (kmiDeviceExtension.capabilities & KMI_CAP_DUMMY_OPAQUE) {
        OpaquePolygonBuffer += DUMMY_OPAQUE_WORDS;
    }
    kmiSystemConfig.buffer = pBufferDesc;
    kmiSystemConfig.opaque = OpaquePolygonBuffer;
    kmiSystemConfig.opaque_modifier = OpaqueModifierBuffer;
    kmiSystemConfig.translucent = TransPolygonBuffer;
    kmiSystemConfig.translucent_modifier = TransModifierBuffer;
    if (kmiGetSysConfigFlag() == KMI_CONFIGURED) {
        lists =
            kmiChkActiveListType(OpaquePolygonBuffer, OpaqueModifierBuffer,
                                 TransPolygonBuffer, TransModifierBuffer, 0);
        pBufferDesc->fActiveList = kmiDeviceExtension.lists_used;
        direct_mode = 0;
        for (i = 0; i < 4; i++) {
            header_words[i] = 0;
        }
        options =
            kmiGetPackOBPParam(OpaquePolygonBuffer, OpaqueModifierBuffer,
                               TransPolygonBuffer, TransModifierBuffer, 0, 0);
        kmiGetNativeRegOBPSize(options);
        total = OpaquePolygonBuffer + OpaqueModifierBuffer +
                TransPolygonBuffer + TransModifierBuffer;
        words = total + (lists * 8) + 160;
        sizes[0] = OpaquePolygonBuffer;
        sizes[1] = OpaqueModifierBuffer;
        sizes[2] = TransPolygonBuffer;
        sizes[3] = TransModifierBuffer;
        if (sizes[0]) {
            sizes[0] -= LIST_SPARE_WORDS;
        }
        rest = &sizes[1];
        for (i = 1; i < 4; i++) {
            if (sizes[i]) {
                sizes[i] -= LIST_SPARE_WORDS;
            }
        }
        kmiSystemConfig.opaque = sizes[0];
        kmiSystemConfig.opaque_modifier = sizes[1];
        kmiSystemConfig.translucent = sizes[2];
        kmiSystemConfig.translucent_modifier = sizes[3];
        OpaquePolygonBuffer = sizes[0];
        OpaqueModifierBuffer = sizes[1];
        TransPolygonBuffer = sizes[2];
        TransModifierBuffer = sizes[3];
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V) {
            bank_count = 1;
        }
        status = kmiGetVertexBasePointer(
            OpaquePolygonBuffer, OpaqueModifierBuffer, TransPolygonBuffer,
            TransModifierBuffer, 0, words, bank_count);
        if (status) {
            return status;
        }
        kmiGetDisplayData(DISPLAY_BPP, kmiSystemConfig.strip_buffer,
                          kmiSystemConfig.request[0].height);
        if (total < 0x10000) {
            total = 0x10000;
        }
        kmiSystemConfig.ta_buffer_size =
            (total * 4) + kmiSystemConfig.region_opb_size;
        available = 0x200000 - kmiSystemConfig.frame_buffer_size;
        if (kmiSystemConfig.ta_buffer_size > available) {
            kmiSystemConfig.ta_buffer_size = available;
        }
        status = kmiTAallocEx(kmiSystemConfig.ta_buffer_size, &ta_buffers[0],
                              &ta_buffers[1]);
        if (status) {
            return status;
        }
        kmiDeviceExtension.ta_buffer_size = kmiSystemConfig.ta_buffer_size;
        kmiSetTAEnvironment(kmiDeviceExtension.ta_layout[0].region_array >> 7,
                            kmiDeviceExtension.ta_layout[0].object_list >> 7,
                            kmiDeviceExtension.ta_layout[0].isp_params >> 7,
                            kmiDeviceExtension.ta_layout[0].tsp_params >> 7,
                            options & ~kmiSystemConfig.strip_buffer, 0);
        kmi_write_pointers[0] = kmi_list_bases[0][0];
        kmi_write_pointers[1] = kmi_list_bases[0][1];
        kmi_write_pointers[2] = kmi_list_bases[0][2];
        kmi_write_pointers[3] = kmi_list_bases[0][3];
        for (i = 0; i < 4; i++) {
            kmi_list_starts[i] = kmi_write_pointers[i];
        }
        kmiCurrentListState.ListType = 0;
        kmiCurrentListState.GlobalParamSize = 0;
        kmiCurrentListState.Reserved0 = 0;
        kmiCurrentListState.VertexBank = 0;
        /* @hack spelling: bank 0's row as *kmi_list_bases, which SHC stores
         * straight from R5 (0x8ce1279a). as kmi_list_bases[0], SHC also copies
         * it to R3, unused, the function runs 2 bytes long and 76 of 348 words
         * differ */
        kmiVertexBuff = *kmi_list_bases;
        kmiCurrentVertexBuff = kmi_write_pointers;
        for (i = 0; i < bank_count; i++) {
            kmi_banks[i] = kmi_list_bases[i];
        }
        kmiCopyDummyGlobalData(bank_count);
        for (i = 0; i < 4; i++) {
            kmiCurrentVertexBuff[i] += header_words[i];
        }
        pBufferDesc->pCurrentPtr = kmi_write_pointers;
        pBufferDesc->pGlobalParam = &kmiGlobalWork.control;
        pBufferDesc->pCurrentListState = &kmiCurrentListState;
        pBufferDesc->ppBuffer = kmi_banks;
    } else if (kmiAddSysConfigFlag(KMI_CONFIG_VERTEX_BUFFER) ==
               KMI_CONFIG_ALL) {
        kmiSetSysConfigFlag(KMI_CONFIGURED);
        status = kmiAllocSysConfig(kmiSystemConfig.buffer_mode);
    }
    return status;
}

void kmiFlipVertexBuffer(KMDWORD bank)
{
    KMDWORD i;

    if (kmiDeviceExtension.latency_model != KM_LATENCYMODEL_2V) {
        kmiVertexBuff = kmi_list_bases[bank];
    } else {
        kmiVertexBuff = kmi_list_bases[0];
    }
    /* @hack redundant: the five copies in a loop of one pass. SHC then starts
     * both loops below from i's 0 in R14 (MOV R14,R6 at 0x8ce128aa, then
     * MOV R6,R4 at 0x8ce128d2). without it, each loads its own 0 and 2 of 75
     * words differ */
    do {
        kmiCurrentVertexBuff[0] = kmiVertexBuff[0];
        kmiCurrentVertexBuff[1] = kmiVertexBuff[1];
        kmiCurrentVertexBuff[2] = kmiVertexBuff[2];
        kmiCurrentVertexBuff[3] = kmiVertexBuff[3];
        kmiCurrentVertexBuff[4] = kmiVertexBuff[4];
    } while (0);
    for (i = 0; i < 5; i++) {
        kmi_list_starts[i] = kmiCurrentVertexBuff[i];
    }
    kmiCurrentListState.VertexBank = bank & 1;
    for (i = 0; i < 5; i++) {
        kmiCurrentVertexBuff[i] += header_words[i];
    }
}

KMSTATUS kmFlushVertexBuffer(KMINT32 ListType)
{
    KMDWORD *handle;
    KMDWORD bytes;

    bytes = (kmiCurrentVertexBuff[ListType] - kmi_list_starts[ListType]) * 4;
    /* @hack redundant: the retry loop in a loop of one pass, which keeps
     * ListType in R11 and DMA_QUEUE_FULL in R14 (0x8ce128fe, 0x8ce12924).
     * without it, 19 of 62 words differ */
    do {
        while (kmiDMAtoTARequest(TA_INPUT, kmi_list_starts[ListType], bytes,
                                 DMA_SOURCE_UP, &handle) == DMA_QUEUE_FULL) {
        }
    } while (0);
    kmiCurrentVertexBuff[ListType] =
        kmi_list_starts[ListType] + header_words[ListType];
    /* @bug memory: for a list with nothing past its start, this store reads the
     * word an unset handle points at. bytes is 0 then, so kmiDMAtoTARequest
     * returns DMA_EMPTY without writing handle (0x8ce17c98 to 0x8ce17cb0), and
     * the retry loop ends. the reads of the unset handle and that word are at
     * 0x8ce12952 and 0x8ce12956 */
    kmiDeviceExtension.flush_dma = *handle;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmQueryFinishFlushVertexBuffer(void)
{
    if (kmiQueryFinishDMA(kmiDeviceExtension.flush_dma) != TRUE) {
        return KMSTATUS_LAST_LIST_UNDER_DMA;
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmCreateTABuffer(PKMVERTEXBUFFDESC pBufferDesc, KMINT32 TABuffer)
{
    KMDWORD lists[4];
    KMDWORD options;
    KMDWORD available;
    KMDWORD *opaque;
    KMDWORD *opaque_modifier;
    KMDWORD *translucent;
    KMDWORD *translucent_modifier;
    KMSTATUS status;
    /* @hack spelling: pBufferDesc copied to desc for the one store, which keeps
     * buffer_mode's KMI_BUFFERS_TA in R11, and in R12 the 1 that direct_mode
     * and flip_pending take (0x8ce129c2, 0x8ce129e2). without it, the two
     * trade, the function runs 4 bytes long and 140 of 162 words differ */
    PKMVERTEXBUFFDESC desc = pBufferDesc;

    status = kmiSystemConfig.buffer_mode = KMI_BUFFERS_TA;
    kmiSystemConfig.buffer = desc;
    kmiSystemConfig.ta_buffer_size = TABuffer;
    if (kmiGetSysConfigFlag() == KMI_CONFIGURED) {
        direct_mode = 1;
        kmiDeviceExtension.ta_slot = kmiDeviceExtension.write_slot;
        /* @hack spelling: the slot's state set from status, still 2, as
         * SLOT_IN_TA. SHC then stores R11's 2 (0x8ce129f6). with SLOT_IN_TA,
         * SHC loads a 2 of its own, the function runs 4 bytes long and 127 of
         * 162 words differ */
        kmiDeviceExtension.slot_state[kmiDeviceExtension.write_slot] = status;
        kmiDeviceExtension.ta_busy = 0;
        kmiDeviceExtension.flip_pending = 1;
        /* @hack spelling: the lists' bits stored and read through pointers into
         * lists. SHC then keeps lists' address in R10, and the next three
         * elements' in R9, R8 and R11 (0x8ce12a06 to 0x8ce12a26). through
         * lists[0] to lists[3], the function runs 6 bytes short and 151 of
         * 162 words differ */
        opaque = lists;
        *opaque = pBufferDesc->fActiveList & LIST_BIT(KM_OPAQUE_POLYGON);
        opaque_modifier = opaque;
        translucent = opaque;
        opaque_modifier++;
        /* @hack spelling: the opaque modifier's bit taken with status, still 2,
         * as LIST_BIT(KM_OPAQUE_MODIFIER). SHC then shares buffer_mode's
         * KMI_BUFFERS_TA in R11 (0x8ce129c2). with
         * LIST_BIT(KM_OPAQUE_MODIFIER), the function runs 4 bytes long and 131
         * of 162 words differ */
        *opaque_modifier = status & pBufferDesc->fActiveList;
        /* @hack redundant: never-run stores of 1 and tests of lists and
         * pBufferDesc. SHC then keeps pBufferDesc in R13, status's 2 in R11,
         * and direct_mode's 1 in R12 (0x8ce129ba, 0x8ce129c2, 0x8ce129e2).
         * without them, 93 of 162 words differ */
        if (0) {
            kmiDeviceExtension.ta_busy = 1;
            kmiDeviceExtension.ta_busy = 1;
            /* NOLINTNEXTLINE(clang-diagnostic-pointer-bool-conversion) */
            if (lists) {
            }
            if (pBufferDesc) {
            }
            if (pBufferDesc) {
            }
            if (pBufferDesc) {
            }
            if (pBufferDesc) {
            }
        }
        translucent += 2;
        *translucent = pBufferDesc->fActiveList & LIST_BIT(KM_TRANS_POLYGON);
        translucent_modifier = opaque;
        translucent_modifier += 3;
        *translucent_modifier =
            pBufferDesc->fActiveList & LIST_BIT(KM_TRANS_MODIFIER);
        kmiChkActiveListType(*opaque, *opaque_modifier, *translucent,
                             *translucent_modifier, 0);
        /* @hack redundant: an empty never-run branch, which puts the pushed 0
         * and kmiGetDisplayData's address in R3 (0x8ce12a3a, 0x8ce12a54).
         * without it, R2 takes them and 7 of 162 words differ */
        if (0) {
        }
        options = kmiGetPackOBPParam(*opaque, *opaque_modifier, *translucent,
                                     *translucent_modifier, 0, 1);
        kmiGetNativeRegOBPSize(options);
        kmiSystemConfig.ta_buffer_size >>= 1;
        kmiGetDisplayData(DISPLAY_BPP, kmiSystemConfig.strip_buffer,
                          kmiSystemConfig.request[0].height);
        available = 0x200000 - kmiSystemConfig.frame_buffer_size;
        if (kmiSystemConfig.ta_buffer_size > available) {
            kmiSystemConfig.ta_buffer_size = available;
        }
        status = kmiTAallocEx(kmiSystemConfig.ta_buffer_size, &ta_buffers[0],
                              &ta_buffers[1]);
        if (status) {
            return status;
        }
        kmiDeviceExtension.ta_buffer_size = kmiSystemConfig.ta_buffer_size;
        kmiSetTAEnvironment(kmiDeviceExtension.ta_layout[0].region_array >> 7,
                            kmiDeviceExtension.ta_layout[0].object_list >> 7,
                            kmiDeviceExtension.ta_layout[0].isp_params >> 7,
                            kmiDeviceExtension.ta_layout[0].tsp_params >> 7,
                            options & ~kmiSystemConfig.strip_buffer, 0);
        kmiDeviceExtension.ta_busy = 1;
    } else if (kmiAddSysConfigFlag(KMI_CONFIG_VERTEX_BUFFER) ==
               KMI_CONFIG_ALL) {
        kmiSetSysConfigFlag(KMI_CONFIGURED);
        status = kmiAllocSysConfig(kmiSystemConfig.buffer_mode);
    }
    /* @bug logic: a call that leaves the configuration waiting, its flag word
     * short of KMI_CONFIG_ALL, returns buffer_mode's KMI_BUFFERS_TA, 2, though
     * nothing failed (BRA at 0x8ce129dc to 0x8ce12afc, BF at 0x8ce12b06). it
     * returns the 2 that status took at entry (MOV R11,R0 at 0x8ce12b16), and
     * to the caller, 2 is KMSTATUS_INVALID_VIDEO_MODE */
    return status;
}

KMSTATUS kmDiscardVertexBuffer(PKMVERTEXBUFFDESC pBufferDesc)
{
    int i;
    KMSTATUS status = kmiFBfreeEx(ta_buffers[0], ta_buffers[1]);

    if (status) {
        return status;
    }
    /* @hack redundant: the loop in a loop of one pass, which gives its stores
     * a 0 of their own in R6, apart from i's in R5 (0x8ce12b4c, 0x8ce12b4e).
     * without it, 3 of 26 words differ */
    do {
        for (i = 0; i < 5; i++) {
            header_words[i] = 0;
        }
    } while (0);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeLatencyModel(KMLATENCYMODEL LatencyModel,
                              KMVERTEXBUFMODEL VbufModel)
{
    kmiDeviceExtension.vsync_count = 0;
    while (kmiDeviceExtension.ta_busy) {
        RUN_WAIT_VSYNC();
        if (kmiDeviceExtension.vsync_count > 10) {
            break;
        }
    }
    kmiDeviceExtension.latency_model = LatencyModel;
    kmiDeviceExtension.vertex_buffer_model = VbufModel;
    kmiDeviceExtension.vertex_buffer_list = VbufModel & 15;
    kmiDeviceExtension.slot_state[0] = SLOT_FILLING;
    kmiDeviceExtension.slot_state[1] = SLOT_FREE;
    kmiDeviceExtension.slot_state[2] = SLOT_FREE;
    kmiDeviceExtension.slot_state[3] = SLOT_FREE;
    kmiDeviceExtension.write_slot = 0;
    kmiDeviceExtension.render_slot = 0;
    kmiDeviceExtension.ta_slot = 0;
    return KMSTATUS_SUCCESS;
}

KMDWORD kmGetCurrentVertexOffset(KMINT32 ListType)
{
    return kmiCurrentVertexBuff[ListType] - kmi_list_starts[ListType];
}

KMSTATUS kmChangeVertexOffset(KMINT32 ListType, KMDWORD VertexOffset)
{
    kmiCurrentVertexBuff[ListType] = kmi_list_starts[ListType] + VertexOffset;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeVertexPCW(KMINT32 ListType, KMDWORD VertexPCW, KMDWORD IncPtr)
{
    kmiDstVertexBuffer = &kmiCurrentVertexBuff[ListType];
    kmiMemCopy8(*kmiDstVertexBuffer, &VertexPCW, 4);
    *kmiDstVertexBuffer += IncPtr;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetSystemConfiguration(PKMSYSTEMCONFIGSTRUCT pSystemConfigStruct)
{
    KMFLOAT shares[5];
    KMFLOAT sum = 0;
    KMDWORD bank_count = 2;
    /* @hack redundant: clear's initializer, which both arms of its test
     * overwrite. it keeps pSystemConfigStruct in R14 and
     * KM_CONFIGFLAG_ENABLE_STRIPBUFFER in R13 (0x8ce12c86, 0x8ce12cc4).
     * without it, they take R13 and R11 and 462 of 657 words differ */
    KMDWORD clear = 0;
    KMDWORD strip_buffer;
    KMDWORD wait_vsync;
    KMDWORD available;
    KMDWORD i;
    KMDWORD total;
    KMDWORD largest_index;
    KMDWORD sizes[5];
    KMDWORD buffers[2];
    KMDWORD direct_lists[5];
    KMDWORD options;
    long largest;
    /* @hack spelling: nHeight copied to height for the one call, which keeps
     * available in R4 and the mask 31 in R5 (MOV.L @(40,R14),R4 at 0x8ce12d60,
     * MOV #31,R5 at 0x8ce12d68). without it, the two trade and 244 of 657
     * words differ */
    KMDWORD height;
    KMDWORD excess;
    KMDWORD *size;
    KMSTATUS status;

    kmiFBinit();
    kmiDiscardRenderPipeline();
    kmiSystemConfig.buffer_mode = KMI_BUFFERS_MADE;
    if (pSystemConfigStruct->flags & KM_CONFIGFLAG_ENABLE_CLEAR_FRAMEBUFFER) {
        clear = 1;
    } else {
        clear = 0;
    }
    if (pSystemConfigStruct->flags & KM_CONFIGFLAG_NOWAITVSYNC) {
        wait_vsync = 0;
    } else {
        wait_vsync = 1;
    }
    if (pSystemConfigStruct->flags & KM_CONFIGFLAG_ENABLE_STRIPBUFFER) {
        strip_buffer = 1;
    } else {
        strip_buffer = 0;
    }
    if (pSystemConfigStruct->flags & KM_CONFIGFLAG_NOWAIT_FINISH_TEXTUREDMA) {
        kmiDeviceExtension.frame_dma_wait = 0;
    } else {
        kmiDeviceExtension.frame_dma_wait = 1;
    }
    height = pSystemConfigStruct->nHeight;
    kmiGetDisplayData(DISPLAY_BPP, strip_buffer, height);
    if (!(pSystemConfigStruct->flags & KM_CONFIGFLAG_USEDIRECTMODE)) {
        direct_mode = 0;
        if (pSystemConfigStruct->flags & KM_CONFIGFLAG_ENABLE_2V_LATENCY) {
            kmiDeviceExtension.latency_model = KM_LATENCYMODEL_2V;
            kmiDeviceExtension.frame_dma_wait = 1;
        } else {
            kmiDeviceExtension.latency_model = KM_LATENCYMODEL_3V;
        }
        kmChangeLatencyModel(kmiDeviceExtension.latency_model,
                             pSystemConfigStruct->nVbufModel);
        available = pSystemConfigStruct->nVertexBufferSize;
        kmiDeviceExtension.vertex_base =
            (KMDWORD)pSystemConfigStruct->pVertexBuffer;
        if (kmiDeviceExtension.vertex_base & 31) {
            /* @hack type: the two reads of vertex_base through a cast to
             * KMDWORD *, which drops the record's volatile. SHC then loads -32
             * before it reloads the record's address and reads it (0x8ce12d72
             * to 0x8ce12d76). it also copies the address before the second read
             * (0x8ce12d80, 0x8ce12d82). without the casts, 10 of 657 words
             * differ */
            available -=
                32 - (*(KMDWORD *)&kmiDeviceExtension.vertex_base & 31);
            kmiDeviceExtension.vertex_base =
                (*(KMDWORD *)&kmiDeviceExtension.vertex_base + 31) & ~31UL;
        }
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V) {
            available >>= 1;
        }
        for (i = 0; i < 5; i++) {
            sizes[i] = 0;
        }
        for (i = 0; i < 5; i++) {
            sum += pSystemConfigStruct->fBufferSize[i];
        }
        if (sum > 100.0F) {
            for (i = 0; i < 5; i++) {
                shares[i] = pSystemConfigStruct->fBufferSize[i] * 100.0F / sum;
            }
        } else {
            for (i = 0; i < 5; i++) {
                shares[i] = pSystemConfigStruct->fBufferSize[i];
            }
        }
        for (i = 0; i < 5; i++) {
            sizes[i] = (KMDWORD)(shares[i] * available / 100.0F) & ~7UL;
        }
        if (kmiDeviceExtension.capabilities & KMI_CAP_DUMMY_OPAQUE) {
            if (sizes[0] < DUMMY_OPAQUE_SHARE) {
                sizes[0] += DUMMY_OPAQUE_SHARE;
            }
        }
        total = 0;
        largest = -1;
#ifdef GCC
        /* repairs largest_index where every size has its top bit set, so that
         * the signed test below finds none above -1: GCC then takes the
         * excess from the first list's size */
        largest_index = 0;
#endif
        for (i = 0; i < 5; i++) {
            /* @hack type: each size reached through size, which SHC keeps at
             * @R15 (0x8ce12e72), and compared signed by a read through a cast
             * to long * (CMP/GT at 0x8ce12e7a). as total += sizes[i] and
             * (long)sizes[i] > largest, the function runs 28 bytes long and
             * 555 of 657 words differ */
            size = &sizes[i];
            total += *size;
            if (*(long *)size > largest) {
                largest_index = i;
                largest = sizes[i];
            }
        }
        if (total > available) {
            excess = total - available;
            sizes[largest_index] -= excess;
            sizes[largest_index] &= ~31UL;
        }
        if (sizes[0]) {
            sizes[0] -= LIST_SPARE_WORDS;
        }
        for (i = 1; i < 5; i++) {
            if (sizes[i]) {
                sizes[i] -= LIST_SPARE_WORDS;
            }
        }
        kmiSystemConfig.buffer = pSystemConfigStruct->pBufferDesc;
        kmiSystemConfig.opaque = sizes[0];
        kmiSystemConfig.opaque_modifier = sizes[1];
        kmiSystemConfig.translucent = sizes[2];
        kmiSystemConfig.translucent_modifier = sizes[3];
        kmiSystemConfig.punchthrough = sizes[4];
        kmiChkActiveListType(sizes[0], sizes[1], sizes[2], sizes[3], sizes[4]);
        pSystemConfigStruct->pBufferDesc->fActiveList =
            kmiDeviceExtension.lists_used;
        for (i = 0; i < 5; i++) {
            header_words[i] = 0;
        }
        options = kmiGetPackOBPParam(sizes[0], sizes[1], sizes[2], sizes[3],
                                     sizes[4], 0);
        kmiGetNativeRegOBPSize(options);
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V) {
            bank_count = 1;
        }
        kmiGetVertexBasePointer(sizes[0], sizes[1], sizes[2], sizes[3],
                                sizes[4], 0, bank_count);
        kmi_write_pointers[0] = kmi_list_bases[0][0];
        kmi_write_pointers[1] = kmi_list_bases[0][1];
        kmi_write_pointers[2] = kmi_list_bases[0][2];
        kmi_write_pointers[3] = kmi_list_bases[0][3];
        kmi_write_pointers[4] = kmi_list_bases[0][4];
        for (i = 0; i < 5; i++) {
            kmi_list_starts[i] = kmi_write_pointers[i];
        }
        kmiCurrentListState.ListType = 0;
        kmiCurrentListState.GlobalParamSize = 0;
        kmiCurrentListState.Reserved0 = 0;
        kmiCurrentListState.VertexBank = 0;
        /* @hack spelling: bank 0's row as *kmi_list_bases, which SHC stores
         * straight from R5 (0x8ce12fde). as kmi_list_bases[0], SHC also copies
         * it to R3, unused, the function runs 4 bytes long and 261 of 657
         * words differ */
        kmiVertexBuff = *kmi_list_bases;
        kmiCurrentVertexBuff = kmi_write_pointers;
        for (i = 0; i < bank_count; i++) {
            kmi_banks[i] = kmi_list_bases[i];
        }
        kmiCopyDummyGlobalData(bank_count);
        for (i = 0; i < 5; i++) {
            kmiCurrentVertexBuff[i] += header_words[i];
        }
    } else {
        kmiDeviceExtension.frame_dma_wait = 1;
        direct_mode = 1;
        kmiDeviceExtension.ta_slot = kmiDeviceExtension.write_slot;
        kmiDeviceExtension.slot_state[kmiDeviceExtension.write_slot] =
            SLOT_IN_TA;
        kmiDeviceExtension.ta_busy = 0;
        kmiDeviceExtension.flip_pending = 1;
        direct_lists[0] = pSystemConfigStruct->pBufferDesc->fActiveList &
                          LIST_BIT(KM_OPAQUE_POLYGON);
        direct_lists[1] = pSystemConfigStruct->pBufferDesc->fActiveList &
                          LIST_BIT(KM_OPAQUE_MODIFIER);
        direct_lists[2] = pSystemConfigStruct->pBufferDesc->fActiveList &
                          LIST_BIT(KM_TRANS_POLYGON);
        direct_lists[3] = pSystemConfigStruct->pBufferDesc->fActiveList &
                          LIST_BIT(KM_TRANS_MODIFIER);
        direct_lists[4] = pSystemConfigStruct->pBufferDesc->fActiveList &
                          LIST_BIT(KM_PUNCHTHROUGH_POLYGON);
        kmiChkActiveListType(direct_lists[0], direct_lists[1], direct_lists[2],
                             direct_lists[3], direct_lists[4]);
        options = kmiGetPackOBPParam(direct_lists[0], direct_lists[1],
                                     direct_lists[2], direct_lists[3],
                                     direct_lists[4], 1);
        kmiGetNativeRegOBPSize(options);
    }
    kmiSystemConfig.texture_size = pSystemConfigStruct->nTextureMemorySize;
    if (kmiSystemConfig.texture_size & 7) {
        kmiSystemConfig.texture_size += 7;
        kmiSystemConfig.texture_size &= ~7UL;
    }
    kmiSystemConfig.ta_buffer_size = (kmiDeviceExtension.texture_memory_size -
                                      (kmiSystemConfig.texture_size * 4) -
                                      (kmiSystemConfig.frame_buffer_size * 2)) /
                                     2;
    if (kmiGetProfileInt("HARDWARE", "EnableTexture16MB", 0) == 1) {
        if (kmiSystemConfig.ta_buffer_size > 0x400000) {
            kmiSystemConfig.ta_buffer_size = 0x400000;
        }
    } else {
        if (kmiSystemConfig.ta_buffer_size > 0x200000) {
            kmiSystemConfig.ta_buffer_size = 0x200000;
        }
    }
    available = kmiSystemConfig.ta_buffer_size;
    if (available < kmiSystemConfig.region_opb_size) {
        return KMSTATUS_NOT_ENOUGH_MEMORY;
    }
    status = kmiTAallocEx(available, &buffers[0], &buffers[1]);
    if (status) {
        return status;
    }
    kmiDeviceExtension.ta_buffer_size = kmiSystemConfig.ta_buffer_size;
    kmiSetTAEnvironment(kmiDeviceExtension.ta_layout[0].region_array >> 7,
                        kmiDeviceExtension.ta_layout[0].object_list >> 7,
                        kmiDeviceExtension.ta_layout[0].isp_params >> 7,
                        kmiDeviceExtension.ta_layout[0].tsp_params >> 7,
                        options & ~strip_buffer, 0);
    if (pSystemConfigStruct->flags & KM_CONFIGFLAG_USEDIRECTMODE) {
        kmiDeviceExtension.ta_busy = 1;
    }
    pSystemConfigStruct->pBufferDesc->pCurrentPtr = kmi_write_pointers;
    pSystemConfigStruct->pBufferDesc->pGlobalParam = &kmiGlobalWork.control;
    pSystemConfigStruct->pBufferDesc->pCurrentListState = &kmiCurrentListState;
    pSystemConfigStruct->pBufferDesc->ppBuffer = kmi_banks;
    kmiSystemConfig.strip_buffer = strip_buffer;
    kmiDeviceExtension.strip_buffer = strip_buffer;
    kmiSystemConfig.request[0].surface =
        pSystemConfigStruct->ppSurfaceDescArray[0];
    kmiSystemConfig.request[0].width = pSystemConfigStruct->nWidth;
    kmiSystemConfig.request[0].height = pSystemConfigStruct->nHeight;
    kmiSystemConfig.request[0].clear = clear;
    kmiSystemConfig.request[1].surface =
        pSystemConfigStruct->ppSurfaceDescArray[1];
    kmiSystemConfig.request[1].width = pSystemConfigStruct->nWidth;
    kmiSystemConfig.request[1].height = pSystemConfigStruct->nHeight;
    kmiSystemConfig.request[1].clear = clear;
    kmiSystemConfig.front = pSystemConfigStruct->ppSurfaceDescArray[0];
    kmiSystemConfig.back = pSystemConfigStruct->ppSurfaceDescArray[1];
    kmiSystemConfig.wait_vsync = wait_vsync;
    /* @hack redundant: a never-run read of clear, which keeps the 1 that
     * frame_dma_wait takes in R9, and kmi_write_pointers' address in R8
     * (0x8ce12caa, 0x8ce12cfe). without it,
     * the two trade and 27 of 657 words differ */
    if (0) {
        if (clear) {
        }
    }
    kmiSetSysConfigFlag(KMI_CONFIGURED);
    status = kmiAllocSysConfig(kmiSystemConfig.buffer_mode);
    return status;
}

/* set kmiDeviceExtension.lists_used to the LIST_BIT of each list with a
 * nonzero size, and return how many lists that is. under KM_LATENCYMODEL_2V,
 * the list a NOBUF model sends with no buffer counts as in use even at size
 * 0 */
KMDWORD kmiChkActiveListType(KMDWORD opaque, KMDWORD opaque_modifier,
                             KMDWORD translucent, KMDWORD translucent_modifier,
                             KMDWORD punchthrough)
{
    KMDWORD unbuffered[5];
    KMDWORD count = 0;
    KMDWORD i;

    for (i = 0; i < 5; i++) {
        unbuffered[i] = 0;
    }
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V) {
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_OPAQUE) {
            unbuffered[KM_OPAQUE_POLYGON] = 1;
        }
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_OPAQUE_MODIFIER) {
            unbuffered[KM_OPAQUE_MODIFIER] = 1;
        }
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_TRANS) {
            unbuffered[KM_TRANS_POLYGON] = 1;
        }
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_TRANS_MODIFIER) {
            unbuffered[KM_TRANS_MODIFIER] = 1;
        }
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_PUNCH_THROUGH) {
            unbuffered[KM_PUNCHTHROUGH_POLYGON] = 1;
        }
    }
    opaque += unbuffered[KM_OPAQUE_POLYGON];
    opaque_modifier += unbuffered[KM_OPAQUE_MODIFIER];
    translucent += unbuffered[KM_TRANS_POLYGON];
    translucent_modifier += unbuffered[KM_TRANS_MODIFIER];
    punchthrough += unbuffered[KM_PUNCHTHROUGH_POLYGON];
    kmiDeviceExtension.lists_used = 0;
    if (opaque) {
        kmiDeviceExtension.lists_used |= LIST_BIT(KM_OPAQUE_POLYGON);
        count++;
    }
    if (opaque_modifier) {
        kmiDeviceExtension.lists_used |= LIST_BIT(KM_OPAQUE_MODIFIER);
        count++;
    }
    if (translucent) {
        kmiDeviceExtension.lists_used |= LIST_BIT(KM_TRANS_POLYGON);
        count++;
    }
    if (translucent_modifier) {
        kmiDeviceExtension.lists_used |= LIST_BIT(KM_TRANS_MODIFIER);
        count++;
    }
    if (punchthrough) {
        kmiDeviceExtension.lists_used |= LIST_BIT(KM_PUNCHTHROUGH_POLYGON);
        count++;
    }
    return count;
}

/* the options word kmiSetTAEnvironment takes for lists of these sizes:
 * ta_options' low byte, OPB_DOWNWARD, and each list's size code. direct is 1
 * when the lists go straight to the tile accelerator. on a chip with
 * KMI_CAP_TWO_ENTRIES, also set dummy_translucent to whether the opaque
 * modifier, translucent and punch-through lists all take blocks */
KMDWORD kmiGetPackOBPParam(KMDWORD opaque, KMDWORD opaque_modifier,
                           KMDWORD translucent, KMDWORD translucent_modifier,
                           KMDWORD punchthrough, KMDWORD direct)
{
    /* @hack redundant: the table at 0x8ce1ffbc, which the image copies to the
     * frame (its copy's JSR at 0x8ce13346) and never reads; without it, 135 of
     * 144 words differ */
    KMDWORD opb_words[4] = {0, 8, 16, 32};
    KMDWORD options = kmiDeviceExtension.ta_options & 0xff;
    KMDWORD opaque_opb;
    KMDWORD opaque_modifier_opb;
    KMDWORD translucent_opb;
    KMDWORD translucent_modifier_opb;
    KMDWORD punchthrough_opb;

    opaque_opb = opaque_modifier_opb = translucent_opb =
        translucent_modifier_opb = punchthrough_opb = OPB_NONE;
    if (opaque_modifier) {
        opaque_modifier_opb = OPB_8;
    }
    if (translucent_modifier) {
        translucent_modifier_opb = OPB_8;
    }
    if (punchthrough) {
        punchthrough_opb = OPB_16;
    }
    if (direct == 1) {
        if (opaque) {
            opaque_opb = OPB_16;
        }
        if (translucent) {
            translucent_opb = OPB_16;
        }
    } else {
        if (opaque) {
            opaque_opb = OPB_32;
        }
        if (translucent) {
            translucent_opb = OPB_32;
        }
        if (opaque && translucent) {
            if (opaque < translucent) {
                opaque_opb = OPB_16;
            } else {
                translucent_opb = OPB_16;
            }
        }
    }
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V) {
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_OPAQUE) {
            opaque_opb = OPB_32;
        }
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_OPAQUE_MODIFIER) {
            opaque_modifier_opb = OPB_16;
        }
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_TRANS) {
            translucent_opb = OPB_32;
        }
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_TRANS_MODIFIER) {
            translucent_modifier_opb = OPB_16;
        }
        if (kmiDeviceExtension.vertex_buffer_model ==
            KM_VERTEXBUFMODEL_NOBUF_PUNCH_THROUGH) {
            punchthrough_opb = OPB_32;
        }
    }
    options |= OPB_FIELD(opaque_opb, KM_OPAQUE_POLYGON) |
               OPB_FIELD(opaque_modifier_opb, KM_OPAQUE_MODIFIER) |
               OPB_FIELD(translucent_opb, KM_TRANS_POLYGON) |
               OPB_FIELD(translucent_modifier_opb, KM_TRANS_MODIFIER) |
               OPB_FIELD(punchthrough_opb, KM_PUNCHTHROUGH_POLYGON) |
               OPB_DOWNWARD;
    if (kmiDeviceExtension.capabilities & KMI_CAP_TWO_ENTRIES) {
        kmiDeviceExtension.dummy_translucent =
            opaque_modifier_opb && translucent_opb && punchthrough_opb;
    }
    return options;
}

/* open each list lists_used marks with a polygon header of its ListType, in
 * each of bank_count banks, and set its header_words to LIST_HEADER_WORDS.
 * under KM_LATENCYMODEL_2V, the list a NOBUF model sends with no buffer gets no
 * header */
void kmiCopyDummyGlobalData(KMDWORD bank_count)
{
    static const struct list_header initial = {
        PCW_POLYGON | (KM_OPAQUE_POLYGON << PCW_LIST_SHIFT),
        0,
        HEADER_TSP,
        0,
        {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
    KMDWORD i;
    struct list_header empty;
    /* @hack spelling: the header's address set through destination, which
     * keeps it in R14 and bank_count in R13 (0x8ce1348a, 0x8ce1349c). with
     * header set to &empty, the two trade and 24 of 194 words differ */
    struct list_header *header;
    struct list_header *destination = &empty;

    header = destination;
    empty = initial;
    if (kmiDeviceExtension.lists_used & LIST_BIT(KM_OPAQUE_POLYGON)) {
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V ||
            kmiDeviceExtension.vertex_buffer_model !=
                KM_VERTEXBUFMODEL_NOBUF_OPAQUE) {
            for (i = 0; i < bank_count; i++) {
                kmiMemCopy4(kmi_list_bases[i][KM_OPAQUE_POLYGON], header,
                            sizeof(struct list_header));
            }
            header_words[KM_OPAQUE_POLYGON] = LIST_HEADER_WORDS;
        }
    }
    if (kmiDeviceExtension.lists_used & LIST_BIT(KM_OPAQUE_MODIFIER)) {
        header->control = PCW_POLYGON | (KM_OPAQUE_MODIFIER << PCW_LIST_SHIFT);
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V ||
            kmiDeviceExtension.vertex_buffer_model !=
                KM_VERTEXBUFMODEL_NOBUF_OPAQUE_MODIFIER) {
            for (i = 0; i < bank_count; i++) {
                kmiMemCopy4(kmi_list_bases[i][KM_OPAQUE_MODIFIER], header,
                            sizeof(struct list_header));
            }
            header_words[KM_OPAQUE_MODIFIER] = LIST_HEADER_WORDS;
        }
    }
    if (kmiDeviceExtension.lists_used & LIST_BIT(KM_TRANS_POLYGON)) {
        header->control = PCW_POLYGON | (KM_TRANS_POLYGON << PCW_LIST_SHIFT);
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V ||
            kmiDeviceExtension.vertex_buffer_model !=
                KM_VERTEXBUFMODEL_NOBUF_TRANS) {
            for (i = 0; i < bank_count; i++) {
                kmiMemCopy4(kmi_list_bases[i][KM_TRANS_POLYGON], header,
                            sizeof(struct list_header));
            }
            header_words[KM_TRANS_POLYGON] = LIST_HEADER_WORDS;
        }
    }
    if (kmiDeviceExtension.lists_used & LIST_BIT(KM_TRANS_MODIFIER)) {
        header->control = PCW_POLYGON | (KM_TRANS_MODIFIER << PCW_LIST_SHIFT);
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V ||
            kmiDeviceExtension.vertex_buffer_model !=
                KM_VERTEXBUFMODEL_NOBUF_TRANS_MODIFIER) {
            for (i = 0; i < bank_count; i++) {
                kmiMemCopy4(kmi_list_bases[i][KM_TRANS_MODIFIER], header,
                            sizeof(struct list_header));
            }
            header_words[KM_TRANS_MODIFIER] = LIST_HEADER_WORDS;
        }
    }
    if (kmiDeviceExtension.lists_used & LIST_BIT(KM_PUNCHTHROUGH_POLYGON)) {
        header->control =
            PCW_POLYGON | (KM_PUNCHTHROUGH_POLYGON << PCW_LIST_SHIFT);
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V ||
            kmiDeviceExtension.vertex_buffer_model !=
                KM_VERTEXBUFMODEL_NOBUF_PUNCH_THROUGH) {
            for (i = 0; i < bank_count; i++) {
                kmiMemCopy4(kmi_list_bases[i][KM_PUNCHTHROUGH_POLYGON], header,
                            sizeof(struct list_header));
            }
            header_words[KM_PUNCHTHROUGH_POLYGON] = LIST_HEADER_WORDS;
        }
    }
}

/* size kmiSystemConfig.region_opb_size for options as kmiGetPackOBPParam packs
 * them. each tile takes a 24-byte region array entry, and for each list an
 * object pointer block of the size its code gives. a chip with
 * KMI_CAP_TWO_ENTRIES takes every tile twice, or four times while
 * dummy_translucent is set */
void kmiGetNativeRegOBPSize(KMDWORD options)
{
    KMDWORD opaque_opb;
    KMDWORD opaque_modifier_opb;
    KMDWORD translucent_opb;
    KMDWORD translucent_modifier_opb;
    KMDWORD punchthrough_opb;
    KMDWORD tiles;
    KMDWORD opb_words[4] = {0, 8, 16, 32};

    opaque_opb = OPB_CODE(options, KM_OPAQUE_POLYGON);
    opaque_modifier_opb = OPB_CODE(options, KM_OPAQUE_MODIFIER);
    translucent_opb = OPB_CODE(options, KM_TRANS_POLYGON);
    translucent_modifier_opb = OPB_CODE(options, KM_TRANS_MODIFIER);
    punchthrough_opb = OPB_CODE(options, KM_PUNCHTHROUGH_POLYGON);
    tiles = kmiDeviceExtension.tiles_x * kmiDeviceExtension.tiles_y;
    if (kmiDeviceExtension.capabilities & KMI_CAP_TWO_ENTRIES) {
        if (kmiDeviceExtension.dummy_translucent) {
            tiles *= 4;
        } else {
            tiles *= 2;
        }
    }
    kmiSystemConfig.region_opb_size =
        (((opb_words[opaque_opb] + opb_words[opaque_modifier_opb] +
           opb_words[translucent_opb] + opb_words[translucent_modifier_opb] +
           opb_words[punchthrough_opb]) *
          4) +
         24) *
        tiles;
}

/* lay the lists out in bank_count banks, and set kmi_list_bases and each
 * list's list_limit, its size in bytes. the sizes count words. under
 * KM_LATENCYMODEL_2V, the list a NOBUF model sends with no buffer gets
 * TA_INPUT as its base. any other list of size 0 gets a null base. each other
 * list takes its size plus LIST_SPARE_WORDS, rounded up to a multiple of 8
 * words, in ListType order. bank 0 starts at vertex_base rounded up to 32
 * bytes, and bank 1 where bank 0's lists end. the parameter words is not read,
 * and the result is always KMSTATUS_SUCCESS */
KMSTATUS kmiGetVertexBasePointer(KMDWORD opaque, KMDWORD opaque_modifier,
                                 KMDWORD translucent,
                                 KMDWORD translucent_modifier,
                                 KMDWORD punchthrough, KMDWORD words,
                                 KMDWORD bank_count)
{
    long base[2];
    long used = 0;
    KMDWORD bank;

    kmiDeviceExtension.list_limit[KM_OPAQUE_POLYGON] = opaque * 4;
    kmiDeviceExtension.list_limit[KM_OPAQUE_MODIFIER] = opaque_modifier * 4;
    kmiDeviceExtension.list_limit[KM_TRANS_POLYGON] = translucent * 4;
    kmiDeviceExtension.list_limit[KM_TRANS_MODIFIER] = translucent_modifier * 4;
    kmiDeviceExtension.list_limit[KM_PUNCHTHROUGH_POLYGON] = punchthrough * 4;
    base[0] = kmiDeviceExtension.vertex_base;
    for (bank = 0; bank < bank_count; bank++) {
        /* @hack redundant: the bank's layout in a loop of one pass. SHC then
         * enters the bank loop at its test, at its foot (BRA at 0x8ce1374c).
         * without it, SHC tests at the head too, the function runs 16 bytes
         * long and 247 of 273 words differ */
        do {
            if (base[bank] & 31) {
                base[bank] = (base[bank] + 31) & ~31UL;
            }
            /* @hack spelling: list 0's base as *kmi_list_bases[bank], which SHC
             * stores through the row's address kept at @R15 (0x8ce1377c,
             * 0x8ce137ac). as kmi_list_bases[bank][0], the stores index
             * through R0, the function runs 4 bytes short and 200 of 273 words
             * differ */
            if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V &&
                kmiDeviceExtension.vertex_buffer_model ==
                    KM_VERTEXBUFMODEL_NOBUF_OPAQUE) {
                *kmi_list_bases[bank] = (PKMDWORD)TA_INPUT;
            } else if (opaque > 0) {
                *kmi_list_bases[bank] = (PKMDWORD)base[bank] + used;
                used += opaque;
                used += LIST_SPARE_WORDS + 7;
                used &= ~7UL;
            } else {
                *kmi_list_bases[bank] = 0;
            }
            if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V &&
                kmiDeviceExtension.vertex_buffer_model ==
                    KM_VERTEXBUFMODEL_NOBUF_OPAQUE_MODIFIER) {
                kmi_list_bases[bank][KM_OPAQUE_MODIFIER] = (PKMDWORD)TA_INPUT;
            } else if (opaque_modifier > 0) {
                kmi_list_bases[bank][KM_OPAQUE_MODIFIER] =
                    (PKMDWORD)base[bank] + used;
                used += opaque_modifier;
                used += LIST_SPARE_WORDS + 7;
                used &= ~7UL;
            } else {
                kmi_list_bases[bank][KM_OPAQUE_MODIFIER] = 0;
            }
            if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V &&
                kmiDeviceExtension.vertex_buffer_model ==
                    KM_VERTEXBUFMODEL_NOBUF_TRANS) {
                kmi_list_bases[bank][KM_TRANS_POLYGON] = (PKMDWORD)TA_INPUT;
            } else if (translucent > 0) {
                kmi_list_bases[bank][KM_TRANS_POLYGON] =
                    (PKMDWORD)base[bank] + used;
                used += translucent;
                used += LIST_SPARE_WORDS + 7;
                used &= ~7UL;
            } else {
                kmi_list_bases[bank][KM_TRANS_POLYGON] = 0;
            }
            if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V &&
                kmiDeviceExtension.vertex_buffer_model ==
                    KM_VERTEXBUFMODEL_NOBUF_TRANS_MODIFIER) {
                kmi_list_bases[bank][KM_TRANS_MODIFIER] = (PKMDWORD)TA_INPUT;
            } else if (translucent_modifier > 0) {
                kmi_list_bases[bank][KM_TRANS_MODIFIER] =
                    (PKMDWORD)base[bank] + used;
                used += translucent_modifier;
                used += LIST_SPARE_WORDS + 7;
                used &= ~7UL;
            } else {
                kmi_list_bases[bank][KM_TRANS_MODIFIER] = 0;
            }
            if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V &&
                kmiDeviceExtension.vertex_buffer_model ==
                    KM_VERTEXBUFMODEL_NOBUF_PUNCH_THROUGH) {
                kmi_list_bases[bank][KM_PUNCHTHROUGH_POLYGON] =
                    (PKMDWORD)TA_INPUT;
            } else if (punchthrough > 0) {
                kmi_list_bases[bank][KM_PUNCHTHROUGH_POLYGON] =
                    (PKMDWORD)base[bank] + used;
                used += punchthrough;
                used += LIST_SPARE_WORDS + 7;
                used &= ~7UL;
            } else {
                kmi_list_bases[bank][KM_PUNCHTHROUGH_POLYGON] = 0;
            }
            /* bank 1 starts where bank 0's lists end, used words past
             * bank 0's start. base counts bytes */
            base[1] = base[0] + (used * 4);
            used = 0;
        } while (0);
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiAllocSysConfig(KMDWORD buffer_mode)
{
    KMDWORD count = 2;
    KMDWORD bank;
    KMDWORD i;
    KMSTATUS status;

    if (buffer_mode == KMI_BUFFERS_VERTEX) {
        status = kmCreateVertexBuffer(
            kmiSystemConfig.buffer, kmiSystemConfig.opaque,
            kmiSystemConfig.opaque_modifier, kmiSystemConfig.translucent,
            kmiSystemConfig.translucent_modifier);
        if (status) {
            return status;
        }
    }
    if (buffer_mode == KMI_BUFFERS_TA) {
        status = kmCreateTABuffer(kmiSystemConfig.buffer,
                                  kmiSystemConfig.ta_buffer_size);
        if (status) {
            return status;
        }
    }
    if (kmiSystemConfig.request[0].surface ==
        kmiSystemConfig.request[1].surface) {
        count = 1;
    }
    for (i = 0; i < count; i++) {
        status = kmCreateFrameBufferSurface(
            kmiSystemConfig.request[i].surface,
            kmiSystemConfig.request[i].width, kmiSystemConfig.request[i].height,
            kmiSystemConfig.strip_buffer, kmiSystemConfig.request[i].clear);
        /* @hack spelling: the test inverted, with a continue that skips the
         * return. SHC then enters the loop at its test, at its foot (BRA at
         * 0x8ce139a0). as if (status) return status, the section runs 24 bytes
         * long and 106 of 163 words differ */
        if (!status) {
            continue;
        }
        return status;
    }
    status = kmActivateFrameBuffer(kmiSystemConfig.front, kmiSystemConfig.back,
                                   kmiSystemConfig.strip_buffer,
                                   kmiSystemConfig.wait_vsync);
    if (status) {
        return status;
    }
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V ||
        direct_mode == 1) {
        kmiDeviceExtension.ta_slot = kmiDeviceExtension.write_slot;
        kmiDeviceExtension.slot_state[kmiDeviceExtension.write_slot] =
            SLOT_FILLING;
        kmiDeviceExtension.flip_pending = 1;
        bank = kmiDeviceExtension.write_slot & 1;
        kmiWriteRegister(SOFT_RESET, 1);
        kmiWriteRegister(SOFT_RESET, 0);
        kmiWriteRegister(TA_OL_BASE, kmiDeviceExtension.ta_bank[bank].ol_base);
        kmiWriteRegister(TA_OL_LIMIT,
                         kmiDeviceExtension.ta_bank[bank].ol_limit);
        kmiWriteRegister(TA_ISP_BASE,
                         kmiDeviceExtension.ta_bank[bank].isp_base);
        kmiWriteRegister(TA_ISP_LIMIT,
                         kmiDeviceExtension.ta_bank[bank].isp_limit);
        kmiWriteRegister(TA_NEXT_OPB_INIT,
                         kmiDeviceExtension.ta_bank[bank].next_opb_init);
        kmiWriteRegister(TA_LIST_INIT, 0x80000000);
        kmiReadRegister(TA_LIST_INIT);
        kmiDeviceExtension.lists_done = 0;
        kmiDeviceExtension.lists_sent = kmiDeviceExtension.lists_used;
    }
    return status;
}

KMDWORD kmiAddSysConfigFlag(KMDWORD flag)
{
    config_flags |= flag;
    return config_flags;
}

KMDWORD kmiSetSysConfigFlag(KMDWORD flag)
{
    config_flags = flag;
    return config_flags;
}

KMDWORD kmiGetSysConfigFlag(void)
{
    return config_flags;
}

void kmiDiscardRenderPipeline(void)
{
    kmiDeviceExtension.slot_state[0] = SLOT_FILLING;
    kmiDeviceExtension.slot_state[1] = SLOT_FREE;
    kmiDeviceExtension.slot_state[2] = SLOT_FREE;
    kmiDeviceExtension.slot_state[3] = SLOT_FREE;
    kmiDeviceExtension.write_slot = 0;
    kmiDeviceExtension.render_slot = 0;
    kmiDeviceExtension.ta_slot = 0;
    kmiDeviceExtension.vsync_count = 0;
    while (kmiDeviceExtension.render_busy) {
        RUN_WAIT_VSYNC();
        if (kmiDeviceExtension.vsync_count > 10) {
            break;
        }
    }
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V) {
        kmiDeviceExtension.vsync_count = 0;
        while (kmiDeviceExtension.ta_busy) {
            RUN_WAIT_VSYNC();
            if (kmiDeviceExtension.vsync_count > 10) {
                break;
            }
        }
    }
    kmiDeviceExtension.ta_busy = 0;
    kmiDeviceExtension.render_busy = 0;
}
