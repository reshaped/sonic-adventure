/* Kamui's vertex buffers, and the system configuration that makes them and the
 * tile accelerator's two buffers. a vertex buffer holds the lists in two
 * banks, or in one under KM_LATENCYMODEL_2V */
#include <fballoc.h>
#include <kamui.h>
#include <sg_xpt.h>

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
static PKMDWORD ta_buffers[2];
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
void kmiCopyDummyGlobalData(KMDWORD bank_count);
void kmiGetNativeRegOBPSize(KMDWORD options);
KMSTATUS kmiGetVertexBasePointer(KMDWORD opaque, KMDWORD opaque_modifier,
                                 KMDWORD translucent,
                                 KMDWORD translucent_modifier,
                                 KMDWORD punchthrough, KMDWORD words,
                                 KMDWORD bank_count);

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
    KMSTATUS status = KMSTATUS_SUCCESS;
    KMDWORD sizes[4];
    /* @hack redundant: the table at 0x8c679708, which the image copies to the
     * frame (its address loaded at 0x8c65fcd2) and never reads; without it,
     * 358 of 368 words differ */
    KMDWORD opb_words[4] = {0, 8, 16, 32};
    KMDWORD i;
    KMDWORD bank_count = 2;
    KMDWORD lists;
    KMDWORD words;
    KMDWORD options;
    KMDWORD available;
    KMDWORD total;

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
         * straight from R6 (0x8c65ff80). as kmi_list_bases[0], SHC also copies
         * it to R3, unused, and 62 of 368 words differ */
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
    kmiCurrentVertexBuff[0] = kmiVertexBuff[0];
    kmiCurrentVertexBuff[1] = kmiVertexBuff[1];
    kmiCurrentVertexBuff[2] = kmiVertexBuff[2];
    kmiCurrentVertexBuff[3] = kmiVertexBuff[3];
    kmiCurrentVertexBuff[4] = kmiVertexBuff[4];
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
    KMDWORD bytes =
        (kmiCurrentVertexBuff[ListType] - kmi_list_starts[ListType]) * 4;

    while (kmiDMAtoTARequest(TA_INPUT, kmi_list_starts[ListType], bytes,
                             DMA_SOURCE_UP, &handle) == DMA_QUEUE_FULL) {
    }
    kmiCurrentVertexBuff[ListType] =
        kmi_list_starts[ListType] + header_words[ListType];
    /* @bug memory: for a list with nothing past its start, this store reads the
     * word an unset handle points at. bytes is 0 then, so kmiDMAtoTARequest
     * returns DMA_EMPTY without writing handle (0x8c663a52 to 0x8c663a6a), and
     * the retry loop ends. the reads of the unset handle and that word are at
     * 0x8c6601b6 and 0x8c6601ba */
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
    KMSTATUS status;

    kmiSystemConfig.buffer_mode = KMI_BUFFERS_TA;
    kmiSystemConfig.buffer = pBufferDesc;
    kmiSystemConfig.ta_buffer_size = TABuffer;
    if (kmiGetSysConfigFlag() == KMI_CONFIGURED) {
        direct_mode = 1;
        kmiDeviceExtension.ta_slot = kmiDeviceExtension.write_slot;
        kmiDeviceExtension.slot_state[kmiDeviceExtension.write_slot] =
            SLOT_IN_TA;
        /* @hack type: the 0 stored to ta_busy through a volatile long view.
         * SHC then loads it apart from the 0 kmiChkActiveListType takes
         * (0x8c660288, 0x8c6602aa). stored as the KMDWORD it is, one register
         * holds both, the function runs 2 bytes long and 132 of 162 words
         * differ */
        *(volatile long *)&kmiDeviceExtension.ta_busy = 0;
        kmiDeviceExtension.flip_pending = 1;
        lists[0] = pBufferDesc->fActiveList & LIST_BIT(KM_OPAQUE_POLYGON);
        lists[1] = pBufferDesc->fActiveList & LIST_BIT(KM_OPAQUE_MODIFIER);
        lists[2] = pBufferDesc->fActiveList & LIST_BIT(KM_TRANS_POLYGON);
        lists[3] = pBufferDesc->fActiveList & LIST_BIT(KM_TRANS_MODIFIER);
        kmiChkActiveListType(lists[0], lists[1], lists[2], lists[3], 0);
        options =
            kmiGetPackOBPParam(lists[0], lists[1], lists[2], lists[3], 0, 1);
        kmiGetNativeRegOBPSize(options);
        /* @hack spelling: the halving as x = x >> 1, which keeps &lists[0] to
         * &lists[3] in R12, R11, R10 and R9 (0x8c660294 to 0x8c6602a4). as
         * x >>= 1, the four take other registers and 19 of 162 words
         * differ */
        kmiSystemConfig.ta_buffer_size = kmiSystemConfig.ta_buffer_size >> 1;
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
    } else {
#ifdef GCC
        /* repairs the unset status the tag below names: a call that leaves
         * the configuration waiting returns KMSTATUS_SUCCESS, as nothing
         * failed */
        status = KMSTATUS_SUCCESS;
#endif
        /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
        if (kmiAddSysConfigFlag(KMI_CONFIG_VERTEX_BUFFER) == KMI_CONFIG_ALL) {
            kmiSetSysConfigFlag(KMI_CONFIGURED);
            status = kmiAllocSysConfig(kmiSystemConfig.buffer_mode);
        }
    }
    /* @bug uninitialized: a call that leaves the configuration waiting, its
     * flag word short of KMI_CONFIG_ALL, writes no status (BRA at 0x8c66026a
     * to 0x8c6603a0, BF at 0x8c6603ae). it then returns the caller's R12
     * (MOV R12,R0 at 0x8c6603be), which that path leaves as it was pushed at
     * 0x8c660244 */
    return status;
}

KMSTATUS kmDiscardVertexBuffer(PKMVERTEXBUFFDESC pBufferDesc)
{
    KMSTATUS status;
    int i;

    status = kmiFBfreeEx(ta_buffers[0], ta_buffers[1]);
    if (status) {
        return status;
    }
    for (i = 0; i < 5; i++) {
        header_words[i] = 0;
    }
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
    KMDWORD clear;
    KMDWORD strip_buffer;
    KMDWORD wait_vsync;
    KMDWORD available;
    KMDWORD i;
    KMDWORD total;
    KMDWORD largest_index;
    KMDWORD sizes[5];
    PKMDWORD buffers[2];
    KMDWORD direct_lists[5];
    KMDWORD options;
    KMDWORD *opaque;
    KMDWORD *opaque_modifier;
    KMDWORD *translucent;
    KMDWORD *translucent_modifier;
    KMDWORD *punchthrough;
    long largest;
    KMDWORD excess;
    KMDWORD *size;
    KMDWORD *out;
    KMSTATUS status;

    /* @hack redundant: a never-run read of nNumOfFrameBuffer, which keeps
     * pSystemConfigStruct in R13 and the loops' bound of 5 in R11 (0x8c660538,
     * 0x8c660658). without it, the two trade and 114 of 738 words differ */
    if (0) {
        if (pSystemConfigStruct->nNumOfFrameBuffer) {
        }
    }
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
    kmiGetDisplayData(DISPLAY_BPP, strip_buffer, pSystemConfigStruct->nHeight);
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
        /* @hack type: three reads of vertex_base through a non-volatile
         * KMDWORD view, which SHC schedules among their neighbours (0x8c66061a,
         * 0x8c660628, 0x8c660638). read volatile, they load in source order
         * and 13 of 738 words differ */
        if (*(KMDWORD *)&kmiDeviceExtension.vertex_base & 31) {
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
            /* @hack spelling: the step as i = i + 1, which keeps the direct
             * arm's &direct_lists[0] at @(8,R15) and translucent at @(4,R15)
             * (0x8c660a38, 0x8c660a52). with i++, the two trade and 6 of 738
             * words differ */
            for (i = 0; i < 5; i = i + 1) {
                shares[i] = pSystemConfigStruct->fBufferSize[i] * 100.0F / sum;
            }
        } else {
            for (i = 0; i < 5; i++) {
                shares[i] = pSystemConfigStruct->fBufferSize[i];
            }
        }
        /* @hack spelling: the step as i = i + 1, which keeps the direct arm's
         * list addresses in their registers and frame slots from 0x8c660a36.
         * with i++, they take others and 29 of 738 words differ */
        for (i = 0; i < 5; i = i + 1) {
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
             * @R15 (0x8c6607a2), and compared signed through a long view of
             * it (CMP/GT at 0x8c6607a8). as total += sizes[i] and
             * (long)sizes[i] > largest, the function runs 60 bytes short, the
             * PSG 32, and 517 of 738 words differ */
            size = &sizes[i];
            total += *size;
            if (*(long *)size > largest) {
                largest_index = i;
                largest = sizes[i];
            }
        }
        if (total > available) {
            /* @hack spelling: the excess through a local, which SHC computes
             * in R5, a copy of total (MOV R6,R5 and SUB R3,R5 at 0x8c660844,
             * 0x8c660846). folded into the subtraction, it is computed in
             * total's R6, and 109 of 738 words differ */
            excess = total - available;
            sizes[largest_index] -= excess;
            sizes[largest_index] &= ~31UL;
        }
        if (sizes[0]) {
            sizes[0] -= LIST_SPARE_WORDS;
        }
        /* @hack spelling: the sizes read through size and written through out,
         * two cursors that start in R5 and R4 (0x8c660862 to 0x8c660866). with
         * sizes[i], SHC forms &sizes[1] in R7 for both, the reads and writes
         * trade registers, and 85 of 738 words differ */
        for (i = 1, out = size = &sizes[1]; i < 5; i++, out++, size++) {
            if (*size) {
                *out -= LIST_SPARE_WORDS;
            }
        }
        kmiSystemConfig.buffer = pSystemConfigStruct->pBufferDesc;
        kmiSystemConfig.opaque = sizes[0];
        kmiSystemConfig.opaque_modifier = sizes[1];
        kmiSystemConfig.translucent = sizes[2];
        kmiSystemConfig.translucent_modifier = sizes[3];
        kmiSystemConfig.punchthrough = sizes[4];
        /* @hack spelling: the sizes passed to kmiChkActiveListType through
         * pointers, set after the stores above in this order. sizes[0] to
         * sizes[4]'s addresses then take the image's frame slots, @(24,R15),
         * @(4,R15), @R15, @(20,R15) and
         * @(28,R15) (0x8c6608ba to 0x8c6608de). with the pointers set in
         * field order, 10 of 738 words differ, and passed as sizes[0] to
         * sizes[4], 59 */
        opaque_modifier = &sizes[1];
        translucent = &sizes[2];
        punchthrough = &sizes[4];
        opaque = &sizes[0];
        translucent_modifier = &sizes[3];
        kmiChkActiveListType(*opaque, *opaque_modifier, *translucent,
                             *translucent_modifier, *punchthrough);
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
         * straight from R5 (0x8c6609c0). as kmi_list_bases[0], SHC also copies
         * it to R3, unused, and 161 of 738 words differ */
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
        /* @hack spelling: direct_lists[2] stored and passed through
         * translucent, which keeps its address at @(4,R15) (0x8c660a52).
         * through direct_lists[2], it trades slots with direct_lists[3]'s, and
         * 6 of 738 words differ */
        translucent = &direct_lists[2];
        *translucent = pSystemConfigStruct->pBufferDesc->fActiveList &
                       LIST_BIT(KM_TRANS_POLYGON);
        direct_lists[3] = pSystemConfigStruct->pBufferDesc->fActiveList &
                          LIST_BIT(KM_TRANS_MODIFIER);
        /* @hack redundant: the store repeated after a comma, which puts
         * direct_lists[4]'s address in R14 (0x8c660a50, 0x8c660a54). until
         * then R14 holds bank_count's first value, 2 (MOV #2,R14 at
         * 0x8c660536). stored once, the 2 takes R11, R14 takes
         * direct_lists[1]'s address, and 89 of 738 words differ */
        direct_lists[4] =
            (direct_lists[4] = pSystemConfigStruct->pBufferDesc->fActiveList &
                               LIST_BIT(KM_PUNCHTHROUGH_POLYGON),
             direct_lists[4]);
        kmiChkActiveListType(direct_lists[0], direct_lists[1], *translucent,
                             direct_lists[3], direct_lists[4]);
        options =
            kmiGetPackOBPParam(direct_lists[0], direct_lists[1], *translucent,
                               direct_lists[3], direct_lists[4], 1);
        kmiGetNativeRegOBPSize(options);
    }
    kmiSystemConfig.texture_size = pSystemConfigStruct->nTextureMemorySize;
    if (kmiSystemConfig.texture_size & 7) {
        /* @hack spelling: the rounding in two statements. SHC then computes
         * ta_buffer_size below in R3 and loads the profile's two names in
         * their order (0x8c660ac0 to 0x8c660adc). as one, x = (x + 7) & ~7UL,
         * it computes it in R2, loads the names in the other order, and 14 of
         * 738 words differ */
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
    /* @hack redundant: a never-run read of clear, which keeps clear at
     * @(16,R15) (0x8c660558, 0x8c66055a). without it, clear moves to
     * @(28,R15), size and out trade R5 and R4, and 49 of 738 words
     * differ */
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
    KMDWORD i;
    KMDWORD count = 0;

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
    /* @hack redundant: the table at 0x8c679738, which the image copies to the
     * frame (its address loaded at 0x8c660d6e) and never reads; without it,
     * 143 of 147 words differ */
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
    struct list_header empty;
    /* @hack spelling: the header's address set through destination, which
     * keeps it in R14 and bank_count in R13 (0x8c660ef8, 0x8c660efc). with
     * header set to &empty, the two trade and 21 of 212 words differ */
    struct list_header *header;
    struct list_header *destination = &empty;
    KMDWORD i;

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
         * loads TA_INPUT, the mask and kmi_list_bases' address after the bank
         * loop's guard (0x8c6611e2 to 0x8c6611e6). without it, they load ahead
         * of the guard and 11 of 269 words differ */
        do {
            if (base[bank] & 31) {
                base[bank] = (base[bank] + 31) & ~31UL;
            }
            /* @hack spelling: list 0's base as *kmi_list_bases[bank], which SHC
             * stores through the row's address kept at @R15 (0x8c661216,
             * 0x8c66122a, 0x8c661244). as kmi_list_bases[bank][0], the stores
             * index through R0 and 35 of 269 words differ */
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
    KMSTATUS status;
    KMDWORD i;
    KMDWORD count = 2;
    KMDWORD bank;

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
        /* @hack redundant: the call in a loop of one pass, which loads
         * kmCreateFrameBufferSurface's address after the loop's guard
         * (0x8c661486). without it, the load moves ahead of the guard and 5 of
         * 164 words differ */
        do {
            status = kmCreateFrameBufferSurface(
                kmiSystemConfig.request[i].surface,
                kmiSystemConfig.request[i].width,
                kmiSystemConfig.request[i].height, kmiSystemConfig.strip_buffer,
                kmiSystemConfig.request[i].clear);
            if (status) {
                return status;
            }
        } while (0);
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
