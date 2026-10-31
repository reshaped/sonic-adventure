/* SDK 1.20J Kamui frame buffer activation and flips, and a frame's lists
 * rendered to the screen or into a texture. its calls do what 1.30J's do, and
 * it has no kmResetPointer. its C is its own, with the hacks R10 needs and
 * alternate_vertex_bank, a function 1.30J's object doesn't hold */
#include <kamui.h>
#include <kmistat.h>
#include <machine.h>
#include <sg_xpt.h>

/* the store queues' address control registers, and the store queue area.
 * QACR_AREA moves an address's area, its bits 28 to 26, to bits 4 to 2, where
 * QACR0 and QACR1 take it. a queue holds SQ_BYTES bytes, written at
 * SQ_BASE | (address & SQ_ADDRESS). once QACR0 and QACR1 hold address's area,
 * prefetch of the address the bytes were written at sends them to address */
#define QACR0 (*(volatile KMDWORD *)0xff000038)
#define QACR1 (*(volatile KMDWORD *)0xff00003c)
#define QACR_AREA(address) ((address) >> 24)
#define SQ_ADDRESS 0x03ffffffUL
#define SQ_BASE 0xe0000000UL
#define SQ_BYTES 32

/* the CPU's timer 0 count, which counts down */
/* @hack type: the count read as an unsigned int, which has kmFlipFrameBuffer
 * load write_slot's address (0x8ce10020) before it reads the count; read as a
 * KMDWORD, the load follows the count's store to ta_start, and 4 of 256 words
 * differ */
#define TCNT0 (*(volatile KMUINT32 *)0xffd8000c)

/* the video chip's registers, as offsets into its register block */
#define SOFT_RESET 0x008
#define START_RENDER 0x014
#define PARAM_BASE 0x020
#define REGION_BASE 0x02c
#define FB_WRITE_CONTROL 0x048
#define FB_WRITE_STRIDE 0x04c
#define FB_READ_START1 0x050
#define FB_READ_START2 0x054
#define FB_WRITE_START1 0x060
#define FB_WRITE_START2 0x064
#define FB_CLIP_X 0x068
#define FB_CLIP_Y 0x06c
#define BACKGROUND_DEPTH 0x088
#define BACKGROUND_TAG 0x08c
#define TA_OL_BASE 0x124
#define TA_ISP_BASE 0x128
#define TA_OL_LIMIT 0x12c
#define TA_ISP_LIMIT 0x130
#define TA_LIST_INIT 0x144
#define TA_NEXT_OPB_INIT 0x164

/* SOFT_RESET's bits */
#define RESET_TA 1
#define RESET_RENDERER 2

/* FB_WRITE_CONTROL's pack mode field, and the mode's values for the three
 * formats a render into a texture writes */
#define FB_PACK 7
#define FB_PACK_RGB565 1
#define FB_PACK_ARGB4444 2
#define FB_PACK_ARGB1555 3

/* FB_WRITE_START1's bit that a render into a texture sets above the texture's
 * offset */
#define FB_WRITE_TEXTURE 0x01000000

/* vertex_buffer_model's bit that every NOBUF model sets */
#define NOBUF 0x20

/* the tile accelerator's polygon input, as a DMA target */
#define TA_INPUT 0x10000000UL
/* the size in bytes of the end-of-list parameter that follows each list */
#define END_OF_LIST 32

/* a list type in bits 24 to 27 of a parameter control word, and the mask of
 * those bits */
#define LIST_TYPE(type) ((type) << 24)
#define LIST_TYPE_MASK 0x0f000000UL

/* the bits a ListType's values take */
#define LIST_TYPE_BITS 7

/* lists_used's bit for a ListType */
#define LIST_BIT(type) (1 << (type))

#define LISTS 5 /* one for each ListType */
#define SLOTS 4

/* the vsync_count past which kmFlipFrameBuffer's second wait for flip_pending
 * gives up, and calls kmiFatalErrorEntry(KMI_FATAL_FLIP_TIMEOUT) */
#define FLIP_RETRY_TIMEOUT 20

/* a skip for kmiReqRestOfDMA that is no ListType, so that it sends every
 * list */
#define NO_LIST 0xff

/* byte cursors at a slot's entries in list_start's and list_size's rows.
 * START_CURSOR and SIZE_CURSOR point at list 0's entries, and START_AT and
 * SIZE_AT are the entries at a cursor. a step of a row's size moves a cursor
 * to the next list's entry. the step after list 4 takes a cursor past its
 * array. it stays inside the device record, since the cursors are formed from
 * DEVICE_BYTES, the record's bytes */
#define DEVICE_BYTES ((volatile unsigned char *)&kmiDeviceExtension)
#define START_CURSOR(slot)                                                     \
    (DEVICE_BYTES +                                                            \
     ((volatile unsigned char *)kmiDeviceExtension.list_start.row -            \
      DEVICE_BYTES) +                                                          \
     ((slot) * sizeof(PKMDWORD)))
#define SIZE_CURSOR(slot)                                                      \
    (DEVICE_BYTES +                                                            \
     ((volatile unsigned char *)kmiDeviceExtension.list_size - DEVICE_BYTES) + \
     ((slot) * sizeof(KMDWORD)))
#define START_AT(cursor) (*(PKMDWORD volatile *)(cursor))
#define SIZE_AT(cursor) (*(volatile KMDWORD *)(cursor))

/* a list's rows of list_start and list_size, each indexed by slot */
/* @hack type: each row converted to a pointer to volatile entries. SHC then
 * loads each row's address from a pool word of its own (0x8ce104a0 to
 * 0x8ce104f0 in kmRender). as list_start.row[list] + slot, SHC adds each row's
 * offset to list 0's address, kmRender is 16 bytes shorter and kmRenderTexture
 * 12 longer, and 985 of the two functions' 1,079 words differ */
#define START_ROW(list)                                                        \
    ((PKMDWORD volatile *)kmiDeviceExtension.list_start.row[list])
#define SIZE_ROW(list) ((volatile KMDWORD *)kmiDeviceExtension.list_size[list])

/* a bank's list bases, by ListType */
struct list_bases {
    PKMDWORD opaque;
    PKMDWORD opaque_modifier;
    PKMDWORD translucent;
    PKMDWORD translucent_modifier;
    PKMDWORD punchthrough;
};
/* @hack type: kmiVertexBuff, the current bank's list bases, read as a record.
 * a member read then takes R0 for the base in kmRender (0x8ce104fc to
 * 0x8ce1052c), and leaves R2 the base and R3 the value in kmRenderTexture
 * (0x8ce10a62 to 0x8ce10a86). read as kmiVertexBuff[list], 71 of the two
 * functions' 1,079 words differ */
#define LIST_BASES ((struct list_bases *)kmiVertexBuff)

/* the frame buffer write registers of a render into a texture */
struct render_target {
    KMDWORD control;
    KMDWORD stride;
    KMDWORD address;
    KMDWORD clip_x;
    KMDWORD clip_y;
    /* 4 bytes no code reads or writes, which bring kmiTextureRenderParam to
     * the 24 bytes of the object file's bss. their type is unknown */
    KMDWORD unk_14;
};

/* 4 bytes no code reads; their type is unknown */
static KMBYTE dat_8ce203e0[4] = {0};
/* the vertex buffer bank, 0 or 1, that this file's calls give
 * kmiFlipVertexBuffer. kmRender switches it to the other bank, by
 * alternate_vertex_bank, before its call, and the other calls keep it.
 * kmSetFramebufferTexture takes it as a frame buffer's index */
static KMINT32 vertex_bank = 0;
/* 1 from kmRenderDirect until the next kmRender or kmRenderTexture. while it
 * is 1, kmFlipFrameBuffer sends no lists, and gives the tile accelerator the
 * new write slot */
static KMINT32 render_direct = 0;

/* the end-of-list control word, in list_end[0]. kmRender writes it after each
 * list it ends, and kmFlipFrameBuffer sends it after a NOBUF model's list. no
 * code reads list_end[1] */
static const KMDWORD list_end[2] = {0x00000000, 0x40000000};

/* the polygon kmiSetOpaqueDummy writes: an opaque triangle with one corner at
 * the origin and the others 2048 pixels along each axis. each vertex's union u
 * holds an fZ of 1.0e-10. the initializer leaves out the union's braces, which
 * SHC rejects there */
/* NOLINTBEGIN(clang-diagnostic-missing-braces) */
struct dummy_polygon kmiDummyOpaque = {
    {0x80040012,
     0x00800000,
     0x20800000,
     0x00000000,
     {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}},
    {
        {KM_VERTEXPARAM_NORMAL, 0.0F, 0.0F, 1.0e-10F, 1.0F, 1.0F, 0.0F, 0.0F},
        {KM_VERTEXPARAM_NORMAL, 2048.0F, 0.0F, 1.0e-10F, 1.0F, 0.0F, 1.0F,
         0.0F},
        {KM_VERTEXPARAM_ENDOFSTRIP, 0.0F, 2048.0F, 1.0e-10F, 1.0F, 0.0F, 0.0F,
         1.0F},
    },
};
/* NOLINTEND(clang-diagnostic-missing-braces) */

struct render_target kmiTextureRenderParam;

/* write kmiDummyOpaque at the opaque list's write pointer, through the store
 * queues, and move the pointer past it */
void kmiSetOpaqueDummy(void)
{
    KMDWORD address = (KMDWORD)kmiCurrentVertexBuff[KM_OPAQUE_POLYGON];
    /* @hack redundant: register on block, which gives it R14 and queue R13
     * (0x8ce0fd28, 0x8ce0fd3c); without it, the two swap and 9 of 43 words
     * differ */
    register unsigned char *block = (unsigned char *)&kmiDummyOpaque;
    KMDWORD queue;
    int n;

    QACR0 = QACR_AREA(address);
    QACR1 = QACR_AREA(address);
    queue = (address & SQ_ADDRESS) | SQ_BASE;
    for (n = 0; n < (int)(sizeof(kmiDummyOpaque) / SQ_BYTES); n++) {
        kmiMemCopy8((PKMDWORD)queue, (PKMDWORD)block, SQ_BYTES);
        prefetch((void *)queue);
        queue += SQ_BYTES;
        block += SQ_BYTES;
    }
    kmiCurrentVertexBuff[KM_OPAQUE_POLYGON] += sizeof(kmiDummyOpaque) / 4;
}

KMSTATUS kmActivateFrameBuffer(PKMSURFACEDESC pPrimarySurfaceDesc,
                               PKMSURFACEDESC pBackBufferSurfaceDesc,
                               KMBOOLEAN fStripBuffer, KMBOOLEAN fWaitVSync)
{
    KMSTATUS status = KMSTATUS_SUCCESS;

    if (pPrimarySurfaceDesc == NULL || pBackBufferSurfaceDesc == NULL) {
        return KMSTATUS_INVALID_SURFACE;
    }
    kmiSystemConfig.front = pPrimarySurfaceDesc;
    kmiSystemConfig.back = pBackBufferSurfaceDesc;
    kmiSystemConfig.wait_vsync = fWaitVSync;
    kmiSystemConfig.strip_buffer = fStripBuffer;
    kmiDeviceExtension.strip_buffer = fStripBuffer;
    if (kmiGetSysConfigFlag() == KMI_CONFIGURED) {
        if (pPrimarySurfaceDesc->fSurfaceFlags !=
            pBackBufferSurfaceDesc->fSurfaceFlags) {
            return KMSTATUS_INVALID_SURFACE_TYPE;
        }
        if (pPrimarySurfaceDesc->uSize.dwFrameBufferSize !=
            pBackBufferSurfaceDesc->uSize.dwFrameBufferSize) {
            return KMSTATUS_INVALID_SURFACE_SIZE;
        }
        kmiDeviceExtension.fb_start[0] = (KMDWORD)pPrimarySurfaceDesc->pSurface;
        kmiDeviceExtension.unk_334[0] = 0;
        kmiDeviceExtension.fb_start[1] =
            (KMDWORD)pBackBufferSurfaceDesc->pSurface;
        kmiDeviceExtension.unk_334[1] = 0;
        kmiWriteRegister(FB_READ_START1, kmiDeviceExtension.fb_read_start1);
        kmiWriteRegister(FB_READ_START2, kmiDeviceExtension.fb_read_start1);
        kmiWriteRegister(FB_WRITE_START1, kmiDeviceExtension.fb_start[0]);
        kmiWriteRegister(FB_WRITE_START2, kmiDeviceExtension.fb_start[1]);
        kmiDeviceExtension.fb_read_start1 = kmiDeviceExtension.fb_start[0];
        kmiDeviceExtension.fb_read_start2 = kmiDeviceExtension.fb_start[1];
        if (fStripBuffer == TRUE) {
            kmiDeviceExtension.field_offset[0] = 0;
            kmiDeviceExtension.field_offset[1] = 0;
            kmiWriteRegister(FB_READ_START1, kmiDeviceExtension.fb_read_start1);
            kmiWriteRegister(FB_READ_START2, kmiDeviceExtension.fb_read_start2);
        } else {
            kmiWriteRegister(FB_READ_START1, kmiDeviceExtension.fb_read_start1);
            kmiWriteRegister(FB_READ_START2, kmiDeviceExtension.fb_read_start1);
        }
        kmiDeviceExtension.display_changed = TRUE;
        kmiDeviceExtension.wait_vsync = fWaitVSync;
    } else if (kmiAddSysConfigFlag(KMI_CONFIG_ACTIVATE) == KMI_CONFIG_ALL) {
        kmiSetSysConfigFlag(KMI_CONFIGURED);
        status = kmiAllocSysConfig(kmiSystemConfig.buffer_mode);
    }
    return status;
}

/* send each list of the write slot whose size isn't 0 to the tile accelerator
 * by DMA, followed by its end-of-list parameter. list skip isn't sent. while
 * common_modifier is TRUE, the common modifier buffer goes ahead of each
 * modifier list, with that list's type set in the buffer's first word. a
 * request that finds the DMA queue full is made again until the queue takes
 * it */
void kmiReqRestOfDMA(KMDWORD skip)
{
    KMDWORD *handle;
    KMINT32 slot = kmiDeviceExtension.write_slot;
    volatile unsigned char *start;
    volatile unsigned char *size;
    KMDWORD list;

    start = START_CURSOR(slot);
    size = SIZE_CURSOR(slot);
    for (list = 0; list < LISTS; list++) {
        if (SIZE_AT(size) != 0 && skip != list) {
            if (kmiDeviceExtension.common_modifier == TRUE &&
                (list == KM_OPAQUE_MODIFIER || list == KM_TRANS_MODIFIER)) {
                *kmiDeviceExtension.common_modifier_data =
                    (*kmiDeviceExtension.common_modifier_data &
                     ~LIST_TYPE_MASK) |
                    LIST_TYPE(list);
                while (kmiDMAtoTARequest(
                           TA_INPUT, kmiDeviceExtension.common_modifier_data,
                           kmiDeviceExtension.common_modifier_size,
                           DMA_SOURCE_UP, &handle) == DMA_QUEUE_FULL) {
                }
            }
            while (kmiDMAtoTARequest(TA_INPUT, START_AT(start),
                                     SIZE_AT(size) + END_OF_LIST, DMA_SOURCE_UP,
                                     &handle) == DMA_QUEUE_FULL) {
            }
        }
        size += sizeof kmiDeviceExtension.list_size[0];
        start += sizeof kmiDeviceExtension.list_start.row[0];
    }
}

KMSTATUS kmFlipFrameBuffer(void)
{
    KMDWORD *handle;
    volatile unsigned char *start;
    volatile unsigned char *size;
    KMINT32 slot;
    int timeout;
    int stalled;
    KMINT32 bank;

    if (render_direct == 0) {
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V) {
            kmiDeviceExtension.slot_state[kmiDeviceExtension.write_slot] =
                SLOT_READY;
            kmiDeviceExtension.flip_pending = 1;
        } else {
            kmiIntStatisticData.ta_start = TCNT0;
            slot = kmiDeviceExtension.write_slot;
            kmiDeviceExtension.ta_slot = slot;
            kmiDeviceExtension.ta_busy = 1;
            kmiDeviceExtension.lists_done = 0;
            kmiDeviceExtension.lists_sent = kmiDeviceExtension.lists_used;
            kmiDeviceExtension.flip_pending = 1;
            kmiIntStatisticData.ta_wait = 0;
            kmiDeviceExtension.slot_state[slot] = SLOT_IN_TA;
            if (kmiDeviceExtension.vertex_buffer_model & NOBUF) {
                QACR0 = QACR_AREA(TA_INPUT);
                QACR1 = QACR_AREA(TA_INPUT);
                kmiMemCopy4((void *)SQ_BASE, (void *)list_end, 4);
                prefetch((void *)SQ_BASE);
            } else {
                start = START_CURSOR(kmiDeviceExtension.write_slot);
                size = SIZE_CURSOR(kmiDeviceExtension.write_slot);
                size += kmiDeviceExtension.vertex_buffer_list *
                        sizeof kmiDeviceExtension.list_size[0];
                start += kmiDeviceExtension.vertex_buffer_list *
                         sizeof kmiDeviceExtension.list_start.row[0];
                while (kmiDMAtoTARequest(TA_INPUT, START_AT(start),
                                         SIZE_AT(size) + END_OF_LIST,
                                         DMA_SOURCE_UP,
                                         &handle) == DMA_QUEUE_FULL) {
                }
            }
            kmiReqRestOfDMA(kmiDeviceExtension.vertex_buffer_list);
        }
    }
    kmiDeviceExtension.vsync_count = 0;
    timeout = 0;
    while (kmiDeviceExtension.flip_pending != 0) {
        RUN_WAIT_VSYNC();
        if (kmiDeviceExtension.vsync_count >
            kmiDeviceExtension.vertex_timeout +
                kmiDeviceExtension.render_timeout + 1) {
            timeout = 1;
            break;
        }
    }
    if (timeout) {
        kmiDeviceExtension.vsync_count = kmiDeviceExtension.render_busy = 0;
        stalled = 0;
        while (kmiDeviceExtension.flip_pending != 0) {
            RUN_WAIT_VSYNC();
            if (kmiDeviceExtension.vsync_count > FLIP_RETRY_TIMEOUT) {
                stalled = 1;
                break;
            }
        }
        if (stalled) {
            kmiFatalErrorEntry(KMI_FATAL_FLIP_TIMEOUT);
        }
    }
    kmiDeviceExtension.write_slot++;
    kmiDeviceExtension.write_slot &= SLOTS - 1;
    kmiDeviceExtension.slot_state[kmiDeviceExtension.write_slot] = SLOT_FILLING;
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V ||
        render_direct == 1) {
        bank = kmiDeviceExtension.write_slot & 1;
        kmiWriteRegister(SOFT_RESET, RESET_TA);
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
        if (render_direct != 0) {
            kmiDeviceExtension.ta_slot = kmiDeviceExtension.write_slot;
            kmiDeviceExtension.slot_state[kmiDeviceExtension.write_slot] =
                SLOT_IN_TA;
            kmiDeviceExtension.ta_busy = 1;
            kmiDeviceExtension.slots_held = 1;
        }
    }
    /* @hack redundant: a never-run store of 0, which keeps the 0 the stores
     * share in R12 (MOV #0,R12 at 0x8ce0ffba) through the bank's setup; without
     * it, the 0 takes R14 and 38 of 256 words differ */
    if (0) {
        kmiDeviceExtension.lists_done = 0;
    }
    return KMSTATUS_SUCCESS;
}

static void alternate_vertex_bank(void)
{
    vertex_bank = (vertex_bank + 1) & 1;
}

/* render ta_slot's bank into the frame buffer target describes. it sets
 * render_busy and, under KMI_CAP_RESET_BEFORE_RENDER, resets the renderer. it
 * writes REGION_BASE and PARAM_BASE from the bank, and the frame buffer write
 * registers from target. the render starts once the tile accelerator is idle.
 *
 * if the tile accelerator is still busy when vsync_count passes vertex_timeout,
 * no render starts. the write control, stride and clip registers get back
 * kmiDeviceExtension's values for them. ta_busy and texture_render are cleared,
 * and kmiFlipVertexBuffer(vertex_bank) starts the lists over. FB_WRITE_START1
 * keeps target's address, and render_busy stays set */
void kmiStartRenderTextureEntry(struct render_target *target)
{
    KMINT32 bank;
    KMINT32 offset;
    int timeout;

    kmiDeviceExtension.render_busy = 1;
    bank = 1 & kmiDeviceExtension.ta_slot;
    if (kmiDeviceExtension.capabilities & KMI_CAP_RESET_BEFORE_RENDER) {
        kmiWriteRegister(SOFT_RESET, RESET_RENDERER);
        kmiWriteRegister(SOFT_RESET, 0);
    }
    /* @hack spelling: offset, a copy of bank, which keeps bank at @R15 and its
     * scaled offset, bank * 32, at @(4,R15) (0x8ce102d8, 0x8ce102fa); without
     * it, the two swap and 4 of 125 words differ */
    offset = bank;
    kmiWriteRegister(REGION_BASE,
                     kmiDeviceExtension.ta_bank[offset].region_base);
    kmiWriteRegister(PARAM_BASE, kmiDeviceExtension.ta_bank[offset].isp_base);
    kmiWriteRegister(FB_WRITE_START1, target->address);
    kmiWriteRegister(FB_WRITE_CONTROL, target->control);
    kmiWriteRegister(FB_WRITE_STRIDE, target->stride);
    kmiWriteRegister(FB_CLIP_X, target->clip_x);
    kmiWriteRegister(FB_CLIP_Y, target->clip_y);
    kmiDeviceExtension.vsync_count = 0;
    timeout = 0;
    while (kmiDeviceExtension.ta_busy != 0) {
        RUN_WAIT_VSYNC();
        /* @hack spelling: vertex_timeout read through its address. SHC then
         * loads vsync_count's and vertex_timeout's addresses (0x8ce1033e,
         * 0x8ce10340) before it reads either; read plainly, vertex_timeout is
         * read first and 2 of 125 words differ */
        if (kmiDeviceExtension.vsync_count >
            *(&kmiDeviceExtension.vertex_timeout)) {
            timeout = 1;
            break;
        }
    }
    if (timeout) {
        kmiWriteRegister(FB_WRITE_CONTROL, kmiDeviceExtension.fb_write_control);
        kmiWriteRegister(FB_WRITE_STRIDE, kmiDeviceExtension.fb_write_stride);
        kmiWriteRegister(FB_CLIP_X, kmiDeviceExtension.fb_clip_x);
        kmiWriteRegister(FB_CLIP_Y, kmiDeviceExtension.fb_clip_y);
        kmiDeviceExtension.ta_busy = 0;
        kmiDeviceExtension.texture_render = 0;
        kmiFlipVertexBuffer(vertex_bank);
        return;
    }
    kmiWriteRegister(BACKGROUND_TAG, kmiDeviceExtension.background_tag);
    kmiWriteRegister(BACKGROUND_DEPTH,
                     kmiDeviceExtension.background_depth & ~15);
    kmiWriteRegister(START_RENDER, 0xffffffff);
}

/* set kmiTextureRenderParam.control for a render into surface: the frame buffer
 * write control kmiDeviceExtension keeps, with the pack mode of surface's
 * format. returns KMSTATUS_SUCCESS. a twiddled surface, or one whose BitDepth
 * isn't KMI_BITDEPTH_16, returns KMSTATUS_INVALID_TEXTURE_TYPE and leaves
 * control as it was. a format other than ARGB1555, RGB565 and ARGB4444 has no
 * pack mode, so control gets surface's address ORed in instead */
static KMSTATUS set_texture_render_format(PKMSURFACEDESC surface)
{
    if ((surface->fSurfaceFlags & KM_SURFACEFLAGS_TWIDDLED) ||
        surface->BitDepth != KMI_BITDEPTH_16) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    /* @hack type: surface holds the pack mode, which the image keeps in R4,
     * the argument's register (MOV #3,R4 at 0x8ce10438, OR R4,R2 at
     * 0x8ce1044a). for any other format, the surface's address stays there and
     * is ORed in. with a KMDWORD pack set from surface, 10 of 36 words
     * differ */
    switch (surface->PixelFormat & TEX_FORMAT) {
    case KM_PIXELFORMAT_ARGB1555:
        surface = (PKMSURFACEDESC)FB_PACK_ARGB1555;
        break;
    case KM_PIXELFORMAT_RGB565:
        surface = (PKMSURFACEDESC)FB_PACK_RGB565;
        break;
    case KM_PIXELFORMAT_ARGB4444:
        surface = (PKMSURFACEDESC)FB_PACK_ARGB4444;
        break;
    }
    kmiTextureRenderParam.control =
        (kmiDeviceExtension.fb_write_control & ~FB_PACK) | (KMDWORD)surface;
    return KMSTATUS_SUCCESS;
}

void kmiInitRenderer(void)
{
}

KMSTATUS kmRender(void)
{
    PKMDWORD volatile *start[LISTS];
    volatile KMDWORD *size[LISTS];
    /* the polygon header kmRender writes at the start of each list it cuts
     * back. under a NOBUF model, kmRender also sends it to the tile
     * accelerator when vertex_buffer_list's list is empty. kmRender sets the
     * list's type in its control word */
    struct list_header header = {
        0x80000000,
        0x00000000,
        0x00800400,
        0x00000000,
        {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
    };
    KMDWORD *control;
    /* @hack spelling: the opaque and translucent lists' starts stored through
     * opaque and translucent, pointers to start[0] and start[2]. SHC then keeps
     * &start[0] at @(8,R15) (0x8ce104a8), and &start[1], &start[3] and
     * &start[4] in R6, R5 and R7 (0x8ce104a2 to 0x8ce104c2). stored through
     * start, those take other registers and frame slots, and 33 of 628 words
     * differ */
    PKMDWORD volatile **opaque;
    PKMDWORD volatile **translucent;
    /* @hack redundant: register on slot, which keeps it, scaled by 4, in R13
     * through the rows (0x8ce104a6 to 0x8ce104f8); without it, kmRender is 20
     * bytes shorter and 571 of 628 words differ */
    register KMINT32 slot;
    KMDWORD *queue;
    int i;
    int j;

    if ((kmiDeviceExtension.capabilities & KMI_CAP_DUMMY_OPAQUE) &&
        kmiDeviceExtension.opaque_opb && kmiDeviceExtension.translucent_opb &&
        kmiDeviceExtension.presort) {
        kmiSetOpaqueDummy();
    }
    slot = kmiDeviceExtension.write_slot;
    start[0] = START_ROW(0) + slot;
    start[1] = START_ROW(1) + slot;
    start[2] = START_ROW(2) + slot;
    start[3] = START_ROW(3) + slot;
    start[4] = START_ROW(4) + slot;
    size[0] = SIZE_ROW(0) + slot;
    size[1] = SIZE_ROW(1) + slot;
    size[2] = SIZE_ROW(2) + slot;
    size[3] = SIZE_ROW(3) + slot;
    size[4] = SIZE_ROW(4) + slot;
    opaque = &start[KM_OPAQUE_POLYGON];
    **opaque = LIST_BASES->opaque;
    *start[KM_OPAQUE_MODIFIER] = LIST_BASES->opaque_modifier;
    translucent = &start[KM_TRANS_POLYGON];
    **translucent = LIST_BASES->translucent;
    /* @hack type: the translucent modifier and punch-through lists' starts
     * stored through casts to PKMDWORD *, which drop the entries' volatile and
     * let the image set R8 to R11 and R13 ahead of them (0x8ce1051a to
     * 0x8ce1052e); without the casts, 11 of 628 words differ */
    *(PKMDWORD *)start[KM_TRANS_MODIFIER] = LIST_BASES->translucent_modifier;
    *(PKMDWORD *)start[KM_PUNCHTHROUGH_POLYGON] = LIST_BASES->punchthrough;
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V) {
        for (i = 0; i < LISTS; i++) {
            if (LIST_BIT(i) & kmiDeviceExtension.lists_used) {
                /* @hack type: the size stored and the start read through
                 * casts to KMDWORD * and PKMDWORD *, which drop their volatile
                 * and let the image load the division routine's address
                 * between the read and its subtraction (0x8ce10566 to
                 * 0x8ce1056a) and list_end's ahead of the store (0x8ce10570 to
                 * 0x8ce10574); without the casts, 5 of 628 words differ */
                *(KMDWORD *)size[i] =
                    (kmiCurrentVertexBuff[i] - *(PKMDWORD *)start[i]) * 4;
                kmiMemCopy4(kmiCurrentVertexBuff[i], (void *)list_end, 4);
                kmiCurrentVertexBuff[i] += END_OF_LIST / 4;
                /* @hack redundant: a never-run use of size's address, which
                 * gives &start[1] R6 and &start[4] R7 (0x8ce104a2, 0x8ce104b2);
                 * without it, those two swap, as do two values' frame slots,
                 * and 12 of 628 words differ */
                if (0) {
                    header.control = (KMDWORD)size;
                }
            }
        }
        for (i = 0; i < LISTS; i++) {
            if (!(LIST_BIT(i) & kmiDeviceExtension.lists_used)) {
                *size[i] = 0;
            }
        }
        /* @hack redundant: a never-run use of i, which keeps the NOBUF branch's
         * j * 4 at @R15 and &start[j] at @(4,R15) (0x8ce107c4, 0x8ce107ca);
         * without it, the two swap frame slots and 4 of 628 words differ */
        if (0) {
            header.control = (KMDWORD)i;
        }
        for (i = 0, control = &header.control; i < LISTS; i++) {
            if (LIST_BIT(i) & kmiDeviceExtension.lists_used) {
                /* @hack type: the size read through a cast to KMDWORD *,
                 * which drops its volatile. the image reads it ahead of
                 * list_max[i] (0x8ce105d4); without the cast, 2 of 628 words
                 * differ */
                if (*(KMDWORD *)size[i] > kmiIntStatisticData.list_max[i]) {
                    kmiIntStatisticData.list_max[i] = *size[i];
                }
                if (*size[i] > kmiDeviceExtension.list_limit[i]) {
                    for (j = 0; j < LISTS; j++) {
                        if (LIST_BIT(j) & kmiDeviceExtension.lists_used) {
                            *control =
                                (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                            /* @hack redundant: a never-run use of the mask,
                             * which keeps it in R8 (0x8ce10524, AND R8,R2 at
                             * 0x8ce10616); without it, list_end's address
                             * takes R8, kmRender is 12 bytes longer and 329 of
                             * 628 words differ */
                            if (0) {
                                header.control = ~LIST_TYPE_MASK;
                            }
                            kmiMemCopy4(*start[j], &header, sizeof(header));
                            kmiMemCopy4(*start[j] + (sizeof(header) / 4),
                                        (void *)list_end, 4);
                            *size[j] = sizeof(header);
                        }
                    }
                    kmiFatalErrorEntry(KMI_FATAL_LIST_LIMIT);
                    break;
                }
            }
        }
    } else if (kmiDeviceExtension.vertex_buffer_model & NOBUF) {
        /* clang-format off */
#ifdef GCC
        /* GCC, like clang, rejects SHC's cast of control used as an
         * lvalue */
        for (i = 0, control = &header.control; i < LISTS; i++) {
#else
#ifndef __clang__
        /* @hack type: control set through a cast used as an lvalue, an SHC
         * extension. the cast's volatile keeps SHC from moving control's store
         * into the BRA's delay slot: the image stores i, then control, and
         * leaves the slot empty (0x8ce106f4 to 0x8ce10700). set plainly after
         * i, control's store fills the slot and 289 of 628 words differ; set
         * before i, 34. clang rejects the cast, and takes the plain form */
        for (i = 0, (KMDWORD *volatile)control = &header.control; i < LISTS;
             i++) {
#else
        for (i = 0, control = &header.control; i < LISTS; i++) {
#endif
#endif
            /* clang-format on */
            if ((KMDWORD)i != kmiDeviceExtension.vertex_buffer_list) {
                if (LIST_BIT(i) & kmiDeviceExtension.lists_used) {
                    /* @hack type: the size stored through a KMDWORD * cast,
                     * which drops its volatile and lets the image load
                     * list_end's address ahead of it (0x8ce10740 to
                     * 0x8ce10744); without the cast, 3 of 628 words differ */
                    *(KMDWORD *)size[i] =
                        (kmiCurrentVertexBuff[i] - *start[i]) * 4;
                    kmiMemCopy4(kmiCurrentVertexBuff[i], (void *)list_end, 4);
                    kmiCurrentVertexBuff[i] += END_OF_LIST / 4;
                    if (*size[i] > kmiIntStatisticData.list_max[i]) {
                        kmiIntStatisticData.list_max[i] = *size[i];
                    }
                    /* @hack type: the size read through a cast to KMDWORD *,
                     * which drops its volatile. the image reads it ahead of
                     * list_limit[i] (0x8ce10788); without the cast, 2 of 628
                     * words differ */
                    if (*(KMDWORD *)size[i] >
                        kmiDeviceExtension.list_limit[i]) {
                        for (j = 0; j < LISTS; j++) {
                            if ((KMDWORD)j !=
                                    kmiDeviceExtension.vertex_buffer_list &&
                                (LIST_BIT(j) & kmiDeviceExtension.lists_used)) {
                                *control =
                                    (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                                /* @hack redundant: a never-run block with a
                                 * local of its own, which keeps opaque and the
                                 * first loop's i at @(8,R15) and the NOBUF
                                 * branch's j * 4 at @R15 (0x8ce104a8,
                                 * 0x8ce10540, 0x8ce107c4); without it, 25 of
                                 * 628 words differ */
                                if (0) {
                                    KMDWORD word = header.control;
                                    header.control = word << 1;
                                }
                                kmiMemCopy4(*start[j], &header, sizeof(header));
                                kmiMemCopy4(*start[j] + (sizeof(header) / 4),
                                            (void *)list_end, 4);
                                *size[j] = sizeof(header);
                                /* @hack redundant: a second such block, for
                                 * the same frame slots and the else branch's i
                                 * at @(4,R15) (0x8ce10890); without it, 21 of
                                 * 628 words differ */
                                if (0) {
                                    KMDWORD word = header.control;
                                    header.control = word << 1;
                                }
                            } else {
                                *size[j] = 0;
                            }
                        }
                        kmiFatalErrorEntry(KMI_FATAL_LIST_LIMIT);
                        break;
                    }
                } else {
                    *size[i] = 0;
                }
            }
        }
        if (kmiCurrentVertexBuff[kmiDeviceExtension.vertex_buffer_list] ==
            *start[kmiDeviceExtension.vertex_buffer_list]) {
            QACR0 = QACR_AREA(TA_INPUT);
            QACR1 = QACR_AREA(TA_INPUT);
            control = &header.control;
            *control = (*control & ~LIST_TYPE_MASK) |
                       LIST_TYPE(kmiDeviceExtension.vertex_buffer_list &
                                 LIST_TYPE_BITS);
            queue = (KMDWORD *)(((KMDWORD)kmiCurrentVertexBuff
                                     [kmiDeviceExtension.vertex_buffer_list] &
                                 SQ_ADDRESS) |
                                SQ_BASE);
            kmiMemCopy4(queue, &header, sizeof(header));
            prefetch(queue);
        }
    } else {
        kmQueryFinishFlushVertexBuffer();
        for (i = 0, control = &header.control; i < LISTS; i++) {
            /* @hack type: the size stored through a cast to KMDWORD *, which
             * drops its volatile and lets the image load list_end's address
             * ahead of it (0x8ce108b4 to 0x8ce108b8); without the cast, 3 of
             * 628 words differ */
            *(KMDWORD *)size[i] = (kmiCurrentVertexBuff[i] - *start[i]) * 4;
            kmiMemCopy4(kmiCurrentVertexBuff[i], (void *)list_end, 4);
            kmiCurrentVertexBuff[i] += END_OF_LIST / 4;
            if (!(LIST_BIT(i) & kmiDeviceExtension.lists_used)) {
                *size[i] = 0;
            }
            if (LIST_BIT(i) & kmiDeviceExtension.lists_used) {
                /* @hack type: the size read twice through a KMDWORD * cast,
                 * which drops its volatile, the first ahead of list_max[i]'s
                 * read (0x8ce108f8) and the second after list_max's address
                 * (0x8ce1090a); without the casts, 4 of 628 words differ */
                if (*(KMDWORD *)size[i] > kmiIntStatisticData.list_max[i]) {
                    kmiIntStatisticData.list_max[i] = *(KMDWORD *)size[i];
                }
                if (*size[i] > kmiDeviceExtension.list_limit[i]) {
                    for (j = 0; j < LISTS; j++) {
                        if (LIST_BIT(j) & kmiDeviceExtension.lists_used) {
                            *control =
                                (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                            /* @hack redundant: a never-run use of control,
                             * which gives &start[1] R6 and &start[4] R7
                             * (0x8ce104a2, 0x8ce104b2); without it, the two
                             * swap and 8 of 628 words differ */
                            if (0) {
                                header.control = (KMDWORD)control;
                            }
                            /* @hack redundant: a never-run block with a local
                             * of its own, for the same registers; without it, 8
                             * of 628 words differ */
                            if (0) {
                                KMDWORD word = header.control;
                                header.control = word << 1;
                            }
                            kmiMemCopy4(*start[j], &header, sizeof(header));
                            kmiMemCopy4(*start[j] + (sizeof(header) / 4),
                                        (void *)list_end, 4);
                            *size[j] = sizeof(header);
                        } else {
                            *size[j] = 0;
                        }
                    }
                    kmiFatalErrorEntry(KMI_FATAL_LIST_LIMIT);
                    break;
                }
            }
        }
    }
    render_direct = 0;
    alternate_vertex_bank();
    kmiFlipVertexBuffer(vertex_bank);
    return KMSTATUS_SUCCESS;
}

/* have each kmFlipFrameBuffer until the next kmRender or kmRenderTexture send
 * no lists, and give the tile accelerator the new write slot. kmRenderDirect
 * sets flip_pending and render_direct, and clears slots_held. the result is
 * render_direct's address, not a status */
KMSTATUS kmRenderDirect(void)
{
    kmiDeviceExtension.flip_pending = 1;
    kmiDeviceExtension.slots_held = 0;
    render_direct = 1;
    return (KMSTATUS)&render_direct;
}

/* the end-of-list control word, as list_end, for kmRenderTexture.
 * kmRenderTexture writes it after each list it ends, and under
 * KM_LATENCYMODEL_2V sends it after vertex_buffer_list's list */
static const KMDWORD texture_list_end = 0x00000000;

KMSTATUS kmRenderTexture(PKMSURFACEDESC pTextureSurface)
{
    /* @hack redundant: 32 bytes of frame, sp+24 to sp+55, that no
     * instruction reads or writes, of a type unknown. sp+0 to sp+23, below
     * them, hold the values the code keeps in the frame, and the 32 bytes
     * above them hold the saved registers (0x8ce10a06 to 0x8ce10a14). the
     * image's ADD #-56,R15 reserves them with the 24 bytes below (0x8ce10a16);
     * without them, 2 of 451 words differ */
    KMBYTE unk_18[32];
    KMDWORD stack;
    KMDWORD saved_clip_y;
    KMDWORD saved_clip_x;
    KMDWORD saved_tag;
    KMDWORD *queue;
    /* @hack redundant: register on slot, which gives it R14 after status
     * (0x8ce10a48); without it, the function is 46 bytes shorter and 433 of
     * 451 words differ */
    register KMINT32 slot;
    KMINT32 bank;
    volatile unsigned char *start;
    volatile unsigned char *size;
    KMUINT32 list;
    KMSTATUS status;
    int timeout;

    status = set_texture_render_format(pTextureSurface);
    if (status != KMSTATUS_SUCCESS) {
        kmiFlipVertexBuffer(vertex_bank);
        return status;
    }
    kmiTextureRenderParam.stride = pTextureSurface->u0.Width << 4 >> 6;
    kmiTextureRenderParam.address =
        (KMDWORD)pTextureSurface->pSurface | FB_WRITE_TEXTURE;
    slot = kmiDeviceExtension.write_slot;
    bank = slot & 1;
    start = START_CURSOR(slot);
    size = SIZE_CURSOR(slot);
    START_ROW(KM_OPAQUE_POLYGON)[slot] = LIST_BASES->opaque;
    START_ROW(KM_OPAQUE_MODIFIER)[slot] = LIST_BASES->opaque_modifier;
    START_ROW(KM_TRANS_POLYGON)[slot] = LIST_BASES->translucent;
    START_ROW(KM_TRANS_MODIFIER)[slot] = LIST_BASES->translucent_modifier;
    START_ROW(KM_PUNCHTHROUGH_POLYGON)[slot] = LIST_BASES->punchthrough;
    for (list = 0; list < LISTS; list++) {
        /* @hack type: the size stored through a cast of size, a cursor of
         * volatile bytes, to KMDWORD *, which drops the volatile and lets the
         * image load latency_model's address ahead of it (0x8ce10aa8,
         * 0x8ce10aaa); as SIZE_AT(size), which keeps it, 2 of 451 words
         * differ */
        *(KMDWORD *)size = (kmiCurrentVertexBuff[list] - START_AT(start)) * 4;
        if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V ||
            list != kmiDeviceExtension.vertex_buffer_list) {
            if (LIST_BIT(list) & kmiDeviceExtension.lists_used) {
                kmiMemCopy4(kmiCurrentVertexBuff[list],
                            (void *)&texture_list_end, 4);
                kmiCurrentVertexBuff[list] += END_OF_LIST / 4;
                if (SIZE_AT(size) > kmiDeviceExtension.list_limit[list]) {
                    kmiFatalErrorEntry(KMI_FATAL_LIST_LIMIT);
                }
            }
        }
        size += sizeof kmiDeviceExtension.list_size[0];
        start += sizeof kmiDeviceExtension.list_start.row[0];
    }
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V) {
        kmiDeviceExtension.vsync_count = 0;
        timeout = 0;
        while (kmiDeviceExtension.ta_busy != 0) {
            RUN_WAIT_VSYNC();
            if (kmiDeviceExtension.vsync_count >
                kmiDeviceExtension.vertex_timeout) {
                timeout = 1;
                break;
            }
        }
        if (timeout) {
            kmiDeviceExtension.texture_render = 0;
            kmiDeviceExtension.ta_busy = 0;
            kmiFlipVertexBuffer(vertex_bank);
            return (KMSTATUS)-1;
        }
    }
    saved_tag = kmiDeviceExtension.background_tag;
    kmiDeviceExtension.texture_render = 1;
    kmiDeviceExtension.ta_busy = 1;
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V) {
        kmiWriteRegister(SOFT_RESET, RESET_TA);
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
    }
    kmiDeviceExtension.vsync_count = 0;
    timeout = 0;
    while (kmiDeviceExtension.render_busy != 0) {
        RUN_WAIT_VSYNC();
        if (kmiDeviceExtension.vsync_count >
            kmiDeviceExtension.render_timeout) {
            timeout = 1;
            break;
        }
    }
    if (timeout) {
        kmiDeviceExtension.texture_render = 0;
        kmiFlipVertexBuffer(vertex_bank);
        return (KMSTATUS)-2;
    }
    kmiDeviceExtension.ta_slot = slot;
    kmiDeviceExtension.lists_sent = kmiDeviceExtension.lists_used;
    kmiDeviceExtension.lists_done = 0;
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V) {
        kmiReqRestOfDMA(NO_LIST);
    } else {
        QACR0 = QACR_AREA(TA_INPUT);
        QACR1 = QACR_AREA(TA_INPUT);
        queue = (KMDWORD *)(((KMDWORD)kmiCurrentVertexBuff
                                 [kmiDeviceExtension.vertex_buffer_list] &
                             SQ_ADDRESS) |
                            SQ_BASE);
        kmiMemCopy4(queue, (void *)&texture_list_end, 4);
        prefetch(queue);
        kmiReqRestOfDMA(kmiDeviceExtension.vertex_buffer_list);
    }
    saved_clip_x = kmiDeviceExtension.fb_clip_x;
    saved_clip_y = kmiDeviceExtension.fb_clip_y;
    kmSetPixelClipping(
        0, 0,
        pTextureSurface->u0.Width > kmiDeviceExtension.display_width
            ? kmiDeviceExtension.display_width
            : pTextureSurface->u0.Width,
        pTextureSurface->u1.Height > kmiDeviceExtension.display_height
            ? kmiDeviceExtension.display_height
            : pTextureSurface->u1.Height);
    kmiTextureRenderParam.clip_x = kmiDeviceExtension.fb_clip_x;
    kmiTextureRenderParam.clip_y = kmiDeviceExtension.fb_clip_y;
    kmiDeviceExtension.fb_clip_x = saved_clip_x;
    kmiDeviceExtension.fb_clip_y = saved_clip_y;
    kmiStartRenderTextureEntry(&kmiTextureRenderParam);
    kmiDeviceExtension.background_tag = saved_tag;
    kmiFlipVertexBuffer(vertex_bank);
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V) {
        kmiDeviceExtension.vsync_count = 0;
        timeout = 0;
        while (kmiDeviceExtension.render_busy != 0) {
            RUN_WAIT_VSYNC();
            /* @hack spelling: render_timeout read through its address. SHC then
             * loads vsync_count's and render_timeout's addresses (0x8ce10d9e,
             * 0x8ce10da0) before it reads either; read plainly, render_timeout
             * is read first and 2 of 451 words differ */
            if (kmiDeviceExtension.vsync_count >
                *(&kmiDeviceExtension.render_timeout)) {
                timeout = 1;
                break;
            }
        }
        if (timeout) {
            kmiDeviceExtension.texture_render = 0;
            return (KMSTATUS)-3;
        }
        kmiWriteRegister(SOFT_RESET, RESET_TA);
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
    }
    render_direct = 0;
    /* @hack implicit: stack, which no assignment reaches, is R15, the stack
     * pointer, which the image stores (MOV.L R15,@R3 at 0x8ce10e74); set
     * explicitly, to &saved_tag, the function is 4 bytes longer and 50 of 451
     * words differ */
#ifdef GCC
    /* sets stack to the stack pointer, the value stack_pointer is declared
     * to hold and the image stores */
    stack = (KMDWORD)__builtin_stack_address();
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
    kmiDeviceExtension.stack_pointer = stack;
    return KMSTATUS_SUCCESS;
}

/* once the tile accelerator and then the renderer are idle, render ta_slot's
 * bank into surface as kmRenderTexture renders into one. after the render,
 * reset the tile accelerator and set it up for the write slot's bank. then give
 * it that slot, and set slots_held. returns KMSTATUS_SUCCESS. for a surface
 * set_texture_render_format refuses, it returns that status, after
 * kmiFlipVertexBuffer(vertex_bank).
 *
 * each of its three waits gives up once vsync_count passes a timeout field,
 * and returns that field's address in place of a status. the wait for the
 * tile accelerator returns &vertex_timeout. the wait for an earlier render
 * returns &render_timeout and clears render_busy. the wait for this render
 * returns &render_timeout and clears texture_render */
KMSTATUS kmRenderTextureDirect(PKMSURFACEDESC surface)
{
    KMDWORD saved_clip_y;
    KMDWORD saved_clip_x;
    /* @hack redundant: register on slot, which gives it R14 after status
     * (0x8ce10eca); without it, 57 of 224 words differ */
    register KMINT32 slot;
    KMSTATUS status;
    int timeout;

    status = set_texture_render_format(surface);
    if (status != KMSTATUS_SUCCESS) {
        kmiFlipVertexBuffer(vertex_bank);
        return status;
    }
    kmiTextureRenderParam.stride = surface->u0.Width << 4 >> 6;
    kmiTextureRenderParam.address =
        (KMDWORD)surface->pSurface | FB_WRITE_TEXTURE;
    slot = kmiDeviceExtension.write_slot;
    kmiDeviceExtension.vsync_count = 0;
    timeout = 0;
    while (kmiDeviceExtension.ta_busy != 0) {
        RUN_WAIT_VSYNC();
        /* @hack spelling: vertex_timeout read through its address. SHC then
         * loads vsync_count's and vertex_timeout's addresses (0x8ce10ede,
         * 0x8ce10ee0) before it reads either; read plainly, 2 of 224 words
         * differ */
        if (kmiDeviceExtension.vsync_count >
            *(&kmiDeviceExtension.vertex_timeout)) {
            timeout = 1;
            break;
        }
    }
    if (timeout) {
        /* @hack implicit: R0 still holds &vertex_timeout, loaded for the
         * wait's compare (0x8ce10ede); returned explicitly, 7 of 224 words
         * differ, and 7 of the 451 of kmRenderTexture, whose pool it shares */
#ifdef GCC
        /* repairs the return the tag names: GCC's is -1, kmRenderTexture's
         * when its wait for the tile accelerator gives up */
        return (KMSTATUS)-1;
#else
        return; /* NOLINT(clang-diagnostic-return-mismatch) */
#endif
    }
    kmiDeviceExtension.vsync_count = 0;
    timeout = 0;
    while (kmiDeviceExtension.render_busy != 0) {
        RUN_WAIT_VSYNC();
        if (kmiDeviceExtension.vsync_count >
            kmiDeviceExtension.render_timeout) {
            timeout = 1;
            break;
        }
    }
    if (timeout) {
        kmiDeviceExtension.render_busy = 0;
        /* @hack implicit: R0 still holds &render_timeout, loaded for the
         * wait's compare (0x8ce10f68); returned explicitly, the function is 4
         * bytes longer and 150 of 224 words differ */
#ifdef GCC
        /* repairs the return the tag names: GCC's is -2, kmRenderTexture's
         * when its wait for an earlier render gives up */
        return (KMSTATUS)-2;
#else
        return; /* NOLINT(clang-diagnostic-return-mismatch) */
#endif
    }
    kmiDeviceExtension.texture_render = 1;
    saved_clip_x = kmiDeviceExtension.fb_clip_x;
    saved_clip_y = kmiDeviceExtension.fb_clip_y;
    kmSetPixelClipping(0, 0,
                       surface->u0.Width > kmiDeviceExtension.display_width
                           ? kmiDeviceExtension.display_width
                           : surface->u0.Width,
                       surface->u1.Height > kmiDeviceExtension.display_height
                           ? kmiDeviceExtension.display_height
                           : surface->u1.Height);
    kmiTextureRenderParam.clip_x = kmiDeviceExtension.fb_clip_x;
    kmiTextureRenderParam.clip_y = kmiDeviceExtension.fb_clip_y;
    kmiDeviceExtension.fb_clip_x = saved_clip_x;
    kmiDeviceExtension.fb_clip_y = saved_clip_y;
    kmiStartRenderTextureEntry(&kmiTextureRenderParam);
    kmiDeviceExtension.vsync_count = 0;
    timeout = 0;
    while (kmiDeviceExtension.render_busy != 0) {
        RUN_WAIT_VSYNC();
        /* @hack spelling: render_timeout read through its address. SHC then
         * loads vsync_count's and render_timeout's addresses (0x8ce10ffa,
         * 0x8ce10ffc) before it reads either; read plainly, 2 of 224 words
         * differ */
        if (kmiDeviceExtension.vsync_count >
            *(&kmiDeviceExtension.render_timeout)) {
            timeout = 1;
            break;
        }
    }
    if (timeout) {
        kmiDeviceExtension.texture_render = 0;
        /* @hack implicit: R0 still holds &render_timeout, loaded for the
         * wait's compare (0x8ce10ffa); returned explicitly, the function is 4
         * bytes longer and 99 of 224 words differ */
#ifdef GCC
        /* repairs the return the tag names: GCC's is -3, kmRenderTexture's
         * when its wait for its own render gives up */
        return (KMSTATUS)-3;
#else
        return; /* NOLINT(clang-diagnostic-return-mismatch) */
#endif
    }
    slot &= 1;
    kmiWriteRegister(SOFT_RESET, RESET_TA);
    /* @hack redundant: a never-run store of 1, which keeps the 1 in R13 from
     * texture_render's store (0x8ce10f8a) through the reset to the busy flags'
     * stores (0x8ce110b0, 0x8ce110b4); without it, the function is 8 bytes
     * longer and 112 of 224 words differ */
    if (0) {
        kmiDeviceExtension.slots_held = 1;
    }
    kmiWriteRegister(SOFT_RESET, 0);
    kmiWriteRegister(TA_OL_BASE, kmiDeviceExtension.ta_bank[slot].ol_base);
    kmiWriteRegister(TA_OL_LIMIT, kmiDeviceExtension.ta_bank[slot].ol_limit);
    kmiWriteRegister(TA_ISP_BASE, kmiDeviceExtension.ta_bank[slot].isp_base);
    kmiWriteRegister(TA_ISP_LIMIT, kmiDeviceExtension.ta_bank[slot].isp_limit);
    kmiWriteRegister(TA_NEXT_OPB_INIT,
                     kmiDeviceExtension.ta_bank[slot].next_opb_init);
    kmiWriteRegister(TA_LIST_INIT, 0x80000000);
    kmiReadRegister(TA_LIST_INIT);
    kmiDeviceExtension.lists_done = 0;
    kmiDeviceExtension.lists_sent = kmiDeviceExtension.lists_used;
    kmiDeviceExtension.ta_slot = kmiDeviceExtension.write_slot;
    kmiDeviceExtension.ta_busy = 1;
    kmiDeviceExtension.slots_held = 1;
    kmiDeviceExtension.slot_state[kmiDeviceExtension.write_slot] = SLOT_IN_TA;
    return KMSTATUS_SUCCESS;
}

/* describe the frame buffer fb_start[vertex_bank] in surface as a stride
 * texture of 1024 by 1024 texels with a BitDepth of KMI_BITDEPTH_16. its format
 * is RGB565 for an RGB565 display, and ARGB1555 for an RGB555 or ARGB1555 one.
 * dwTextureSize is 2 bytes for each pixel of the display mode, and pSurface is
 * fb_start[vertex_bank] shifted left by 3. the result is KMSTATUS_SUCCESS, or
 * KMSTATUS_INVALID_TEXTURE_TYPE for another display format, with surface
 * unwritten */
KMSTATUS kmSetFramebufferTexture(PKMSURFACEDESC surface)
{
    KMINT32 width;
    KMINT32 height;

    if (kmiDeviceExtension.video_mode == KM_VGA) {
        width = 640;
        height = 480;
    } else {
        if (kmiDeviceExtension.display_mode & KM_DSPMODE_WIDTH_640) {
            width = 640;
        } else {
            width = 320;
        }
        if (kmiDeviceExtension.display_mode & KM_DSPMODE_HEIGHT_480) {
            height = 480;
        } else {
            height = 240;
        }
    }
    switch (kmiDeviceExtension.bpp) {
    case KM_DSPBPP_ARGB1555:
        surface->BitDepth = KMI_BITDEPTH_16;
        surface->PixelFormat = KM_PIXELFORMAT_ARGB1555;
        break;
    case KM_DSPBPP_RGB565:
        surface->BitDepth = KMI_BITDEPTH_16;
        surface->PixelFormat = KM_PIXELFORMAT_RGB565;
        break;
    case KM_DSPBPP_RGB555:
        surface->BitDepth = KMI_BITDEPTH_16;
        surface->PixelFormat = KM_PIXELFORMAT_ARGB1555;
        break;
    /* @hack redundant: case labels for two formats the default covers, whose
     * tests the image makes (CMP/EQ #4,R0 and CMP/EQ #5,R0 at 0x8ce11116 and
     * 0x8ce1111a); without them, the function is 4 bytes shorter and 49 of
     * 72 words differ, and 23 of the 224 of kmRenderTextureDirect, whose pool
     * it shares */
    case KM_DSPBPP_RGB888:
    case KM_DSPBPP_ARGB8888:
    default:
        return KMSTATUS_INVALID_TEXTURE_TYPE;
        /* @hack redundant: a break after the return, which SHC deletes, but
         * which leaves R2 for the type's 2 (MOV #2,R2 at 0x8ce1119a); without
         * it, R2 and R3 swap and 13 of 72 words differ */
        break;
    }
    surface->SurfaceType = KM_SURFACETYPE_TEXTURE;
    surface->u0.USize = 1024;
    surface->u1.VSize = 1024;
    surface->uSize.dwTextureSize = width * height * 2;
    surface->fSurfaceFlags = KM_SURFACEFLAGS_STRIDE;
    surface->pSurface =
        (PKMDWORD)(kmiDeviceExtension.fb_start[vertex_bank] << 3);
    return KMSTATUS_SUCCESS;
}
