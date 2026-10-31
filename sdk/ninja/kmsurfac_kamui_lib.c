/* Kamui frame buffer activation and flips, and a frame's lists rendered to
 * the screen or into a texture */
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
/* @hack type: the count read as an unsigned int, which puts ta_start's offset,
 * MOV #72,R0, ahead of the load of write_slot's address in kmFlipFrameBuffer
 * (0x8c61de80, 0x8c61de82); read as a KMDWORD, the two swap and 2 of 267 words
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
/* @hack type: the entries at a cursor read and written through plain views.
 * SHC then reads the size, sets R7 and reads the start before it adds
 * END_OF_LIST in kmiReqRestOfDMA and kmFlipFrameBuffer, as the image does
 * (0x8c61ddc0 to 0x8c61ddc6, 0x8c61df72 to 0x8c61df78). in kmRenderTexture, it
 * reads each start before it loads the size cursor, and loads latency_model's
 * address before it stores the size (0x8c61f0c8 to 0x8c61f0e2). accessed
 * volatile, 44 of the three functions' 1098 words differ */
#define START_AT(cursor) (*(PKMDWORD *)(cursor))
#define SIZE_AT(cursor) (*(KMDWORD *)(cursor))

/* list_start's and list_size's rows as records, a member for each list in
 * ListType order */
struct start_rows {
    PKMDWORD opaque[SLOTS];
    PKMDWORD opaque_modifier[SLOTS];
    PKMDWORD translucent[SLOTS];
    PKMDWORD translucent_modifier[SLOTS];
    PKMDWORD punchthrough[SLOTS];
};
struct size_rows {
    KMDWORD opaque[SLOTS];
    KMDWORD opaque_modifier[SLOTS];
    KMDWORD translucent[SLOTS];
    KMDWORD translucent_modifier[SLOTS];
    KMDWORD punchthrough[SLOTS];
};
/* @hack type: the rows reached as members of a record. kmRender then loads
 * each row's address from a pool word of its own (0x8c61e3e2 to 0x8c61e432),
 * and kmRenderTexture steps R0 from one row to the next (0x8c61f09c to
 * 0x8c61f0bc). as list_start.row[list][slot] and list_size[list][slot],
 * kmRender adds each row's offset to list 0's address, kmRenderTexture runs 2
 * bytes long, and 1136 of the two functions' 2170 words differ */
#define STARTS (*(volatile struct start_rows *)&kmiDeviceExtension.list_start)
#define SIZES (*(volatile struct size_rows *)&kmiDeviceExtension.list_size)

/* a bank's list bases, by ListType */
struct list_bases {
    PKMDWORD opaque;
    PKMDWORD opaque_modifier;
    PKMDWORD translucent;
    PKMDWORD translucent_modifier;
    PKMDWORD punchthrough;
};
/* @hack type: kmiVertexBuff, the current bank's list bases, read as a record.
 * kmRender's reads of its members then take R0 for the base (0x8c61e440 to
 * 0x8c61e466). read as kmiVertexBuff[list], the base takes R1 and 230 of 1428
 * words differ */
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
static KMBYTE dat_8c67f388[4] = {0};
/* the vertex buffer bank, 0 or 1, that this file's calls give
 * kmiFlipVertexBuffer. kmRender switches it to the other bank before its call,
 * and the other calls keep it. kmSetFramebufferTexture takes it as a frame
 * buffer's index */
static KMDWORD vertex_bank = 0;
/* 1 from kmRenderDirect until the next kmRender or kmRenderTexture. while it
 * is 1, kmFlipFrameBuffer sends no lists, and gives the tile accelerator the
 * new write slot */
static KMINT32 render_direct = 0;

/* the end-of-list control word. kmRender writes it after each list it ends,
 * and kmFlipFrameBuffer sends it after a NOBUF model's list */
/* @hack type: list_end a word of its own, apart from the word after it.
 * kmRender then loads its address from the pool at each copy (0x8c61e4b2). as
 * an array of two with that word, as 1.20J's is, kmRender keeps the array's
 * address in R10, where the image keeps kmiIntStatisticData's (0x8c61e472);
 * 1292 of its 1428 words differ and it runs 68 bytes long */
static const KMDWORD list_end = 0x00000000;
/* a word no code reads; its use is unknown */
static const KMDWORD dat_8c678cac = 0x40000000;
/* the polygon header kmRender writes at the start of each list it cuts back.
 * under a NOBUF model, kmRender also sends it to the tile accelerator when
 * vertex_buffer_list's list is empty. kmRender sets the list's type in a
 * copy's control word */
static const struct list_header empty_header = {
    0x80000000,
    0x00000000,
    0x00800400,
    0x00000000,
    {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
};

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
    unsigned char *block = (unsigned char *)&kmiDummyOpaque;
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
    PKMSURFACEDESC front;
    PKMSURFACEDESC back;
    KMBOOLEAN strip;
    KMDWORD zero;
    KMSTATUS status;

    back = pBackBufferSurfaceDesc;
    strip = fStripBuffer;
    front = pPrimarySurfaceDesc;
    status = zero = 0;
    /* @hack redundant: a never-run copy of the front surface, which gives front
     * R14 and back R13 (0x8c61db96, 0x8c61db9e); without it, the two swap and
     * 12 of 142 words differ */
    if (0) {
        front = pPrimarySurfaceDesc;
    }
    if (front == NULL || back == NULL) {
        return KMSTATUS_INVALID_SURFACE;
    }
    kmiSystemConfig.front = front;
    kmiSystemConfig.back = back;
    kmiSystemConfig.wait_vsync = fWaitVSync;
    kmiSystemConfig.strip_buffer = strip;
    kmiDeviceExtension.strip_buffer = strip;
    if (kmiGetSysConfigFlag() == KMI_CONFIGURED) {
        if (front->fSurfaceFlags != back->fSurfaceFlags) {
            return KMSTATUS_INVALID_SURFACE_TYPE;
        }
        if (front->uSize.dwFrameBufferSize != back->uSize.dwFrameBufferSize) {
            return KMSTATUS_INVALID_SURFACE_SIZE;
        }
        kmiDeviceExtension.fb_start[0] = (KMDWORD)front->pSurface;
        kmiDeviceExtension.unk_334[0] = 0;
        kmiDeviceExtension.fb_start[1] = (KMDWORD)back->pSurface;
        kmiDeviceExtension.unk_334[1] = 0;
        kmiWriteRegister(FB_READ_START1, kmiDeviceExtension.fb_read_start1);
        kmiWriteRegister(FB_READ_START2, kmiDeviceExtension.fb_read_start1);
        kmiWriteRegister(FB_WRITE_START1, kmiDeviceExtension.fb_start[0]);
        kmiWriteRegister(FB_WRITE_START2, kmiDeviceExtension.fb_start[1]);
        kmiDeviceExtension.fb_read_start1 = kmiDeviceExtension.fb_start[0];
        kmiDeviceExtension.fb_read_start2 = kmiDeviceExtension.fb_start[1];
        if (strip == TRUE) {
            /* @hack type: the field offsets stored as KMDWORDs, zero's type,
             * which keeps zero in R10 and status, set from it, in R9
             * (0x8c61db98, 0x8c61dba4); stored as the fields' KMINT32, the two
             * swap and 6 of 142 words differ */
            *(volatile KMDWORD *)&kmiDeviceExtension.field_offset[0] = zero;
            *(volatile KMDWORD *)&kmiDeviceExtension.field_offset[1] = zero;
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
    KMDWORD slot = kmiDeviceExtension.write_slot;
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
    KMDWORD slot;
    int timeout;
    KMDWORD bank;

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
                kmiMemCopy4((void *)SQ_BASE, (void *)&list_end, 4);
                prefetch((void *)SQ_BASE);
                kmiReqRestOfDMA(kmiDeviceExtension.vertex_buffer_list);
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
                kmiReqRestOfDMA(kmiDeviceExtension.vertex_buffer_list);
            }
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
        timeout = 0;
        /* @hack spelling: a test, then a do-while, which puts timeout's 0,
         * MOV R13,R12, in the delay slot of the test's BT/S (0x8c61dfd2,
         * 0x8c61dfd4); as a while loop, it comes ahead of flip_pending's read
         * and 4 of 267 words differ */
        if (kmiDeviceExtension.flip_pending != 0) {
            do {
                RUN_WAIT_VSYNC();
                if (kmiDeviceExtension.vsync_count > FLIP_RETRY_TIMEOUT) {
                    timeout = 1;
                    break;
                }
            } while (kmiDeviceExtension.flip_pending != 0);
        }
        if (timeout) {
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
    return KMSTATUS_SUCCESS;
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
    bank = kmiDeviceExtension.ta_slot & 1;
    if (kmiDeviceExtension.capabilities & KMI_CAP_RESET_BEFORE_RENDER) {
        kmiWriteRegister(SOFT_RESET, RESET_RENDERER);
        kmiWriteRegister(SOFT_RESET, 0);
    }
    /* @hack spelling: offset, a copy of bank, which keeps bank at @R15 and its
     * scaled offset, bank * 32, at @(4,R15) (0x8c61e19c, 0x8c61e1bc); without
     * it, the two swap and 4 of 131 words differ */
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
         * loads vsync_count's and vertex_timeout's addresses (0x8c61e206,
         * 0x8c61e208) before it reads either; read plainly, vertex_timeout is
         * read first and 2 of 131 words differ */
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
     * the argument's register (MOV #3,R4 at 0x8c61e314, OR R4,R2 at
     * 0x8c61e350). for any other format, the surface's address stays there and
     * is ORed in. with a KMDWORD pack set from surface, 14 of 36 words differ,
     * and a pool word */
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
    PKMDWORD volatile **opaque;
    PKMDWORD volatile **row;
    volatile KMDWORD *size[LISTS];
    struct list_header header;
    KMDWORD *control;
    KMDWORD slot;
    KMDWORD *queue;
    int i;
    int j;

    /* @hack redundant: a never-run use of start's address. SHC then forms that
     * address, R15 + 68, afresh where kmRender uses it (0x8c61e3e6,
     * 0x8c61e49e); without it, kmRender is 32 bytes shorter and 1332 of 1428
     * words differ */
    if (0) {
        i = (int)&start;
    }
    header = empty_header;
    if ((kmiDeviceExtension.capabilities & KMI_CAP_DUMMY_OPAQUE) &&
        kmiDeviceExtension.opaque_opb && kmiDeviceExtension.translucent_opb &&
        kmiDeviceExtension.presort) {
        kmiSetOpaqueDummy();
    }
    slot = kmiDeviceExtension.write_slot;
    start[KM_OPAQUE_POLYGON] = &STARTS.opaque[slot];
    /* @hack redundant: start[1] stored a second time through a comma, and
     * start[3] a second and a third. SHC then keeps &start[1], &start[2] and
     * &start[3] in R6, R7 and R4 (0x8c61e3ea to 0x8c61e3fe). without the extra
     * stores, 33 of 1428 words differ. without start[1]'s alone, 26 do; without
     * start[3]'s alone, 30; and with only one of start[3]'s, 8 */
    start[KM_OPAQUE_MODIFIER] =
        (start[KM_OPAQUE_MODIFIER] = &STARTS.opaque_modifier[slot],
         start[KM_OPAQUE_MODIFIER]);
    start[KM_TRANS_POLYGON] = &STARTS.translucent[slot];
    start[KM_TRANS_MODIFIER] =
        (start[KM_TRANS_MODIFIER] =
             (start[KM_TRANS_MODIFIER] = &STARTS.translucent_modifier[slot],
              start[KM_TRANS_MODIFIER]),
         start[KM_TRANS_MODIFIER]);
    start[KM_PUNCHTHROUGH_POLYGON] = &STARTS.punchthrough[slot];
    /* @hack spelling: the opaque list's start stored through opaque, a pointer
     * to start[0], which a comma sets after size[0]. SHC then keeps &start[0]
     * at
     * @R15, with no copy written for opaque, and &start[4] at @(4,R15)
     * (0x8c61e3f0, 0x8c61e418, 0x8c61e43e, 0x8c61e464); stored through
     * start[0], 41 of 1428 words differ */
    opaque = (size[KM_OPAQUE_POLYGON] = &SIZES.opaque[slot],
              &start[KM_OPAQUE_POLYGON]);
    size[KM_OPAQUE_MODIFIER] = &SIZES.opaque_modifier[slot];
    size[KM_TRANS_POLYGON] = &SIZES.translucent[slot];
    size[KM_TRANS_MODIFIER] = &SIZES.translucent_modifier[slot];
    size[KM_PUNCHTHROUGH_POLYGON] = &SIZES.punchthrough[slot];
    **opaque = LIST_BASES->opaque;
    *start[KM_OPAQUE_MODIFIER] = LIST_BASES->opaque_modifier;
    *start[KM_TRANS_POLYGON] = LIST_BASES->translucent;
    *start[KM_TRANS_MODIFIER] = LIST_BASES->translucent_modifier;
    /* @hack type: the punch-through list's start stored through a plain view,
     * which lets MOV #0,R11 and kmiMemCopy4's pool load come ahead of the store
     * (0x8c61e462, 0x8c61e46a, 0x8c61e46c); stored volatile, 5 of 1428 words
     * differ */
    *(PKMDWORD *)start[KM_PUNCHTHROUGH_POLYGON] = LIST_BASES->punchthrough;
    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_3V) {
        for (i = 0; i < LISTS; i++) {
            if (LIST_BIT(i) & kmiDeviceExtension.lists_used) {
                /* @hack type: the size stored through a plain view, which lets
                 * the image load list_end's address ahead of the store
                 * (0x8c61e4b2, 0x8c61e4b8); stored volatile, 30 of 1428 words
                 * differ */
                *(KMDWORD *)size[i] = (kmiCurrentVertexBuff[i] - *start[i]) * 4;
                kmiMemCopy4(kmiCurrentVertexBuff[i], (void *)&list_end, 4);
                kmiCurrentVertexBuff[i] += END_OF_LIST / 4;
            }
        }
        for (i = 0; i < LISTS; i++) {
            if (!(LIST_BIT(i) & kmiDeviceExtension.lists_used)) {
                *size[i] = 0;
            }
        }
        for (i = 0, control = &header.control; i < LISTS; i++) {
            if (LIST_BIT(i) & kmiDeviceExtension.lists_used) {
                /* @hack type: the size read through plain views. the image
                 * then loads size[i] ahead of list_max[i]'s read, of
                 * list_max[i]'s address and of list_limit[i]'s read
                 * (0x8c61e73c to 0x8c61e764); read volatile, 18 of 1428 words
                 * differ */
                if (*(KMDWORD *)size[i] > kmiIntStatisticData.list_max[i]) {
                    kmiIntStatisticData.list_max[i] = *(KMDWORD *)size[i];
                }
                if (*(KMDWORD *)size[i] > kmiDeviceExtension.list_limit[i]) {
                    /* @hack redundant: a never-run call. it settles the
                     * registers and frame slots of the entry pointers in start
                     * and size (0x8c61e3f6 to 0x8c61e412), and the frame slots
                     * the loops that cut the lists back give j * 4 and
                     * &start[j] (0x8c61e794 on); without it, 261 of 1428 words
                     * differ, and a pool word */
                    if (0) {
                        kmQueryFinishFlushVertexBuffer();
                    }
                    for (j = 0; j < LISTS; j++) {
                        if (LIST_BIT(j) & kmiDeviceExtension.lists_used) {
                            *control =
                                (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                            kmiMemCopy4(*start[j], &header, sizeof(header));
                            kmiMemCopy4(*start[j] + (sizeof(header) / 4),
                                        (void *)&list_end, 4);
                            *size[j] = sizeof(header);
                        }
                    }
                    kmiFatalErrorEntry(KMI_FATAL_LIST_LIMIT);
                    break;
                }
            }
        }
    } else if (kmiDeviceExtension.vertex_buffer_model & NOBUF) {
        for (i = 0, control = &header.control; i < LISTS; i++) {
            if ((KMDWORD)i != kmiDeviceExtension.vertex_buffer_list) {
                if (LIST_BIT(i) & kmiDeviceExtension.lists_used) {
                    /* @hack type: the size stored through a plain view, which
                     * lets the image load list_end's address ahead of the store
                     * (0x8c61e9aa, 0x8c61e9b0); stored volatile, 6 of 1428
                     * words differ */
                    *(KMDWORD *)size[i] =
                        (kmiCurrentVertexBuff[i] - *start[i]) * 4;
                    kmiMemCopy4(kmiCurrentVertexBuff[i], (void *)&list_end, 4);
                    kmiCurrentVertexBuff[i] += END_OF_LIST / 4;
                    /* @hack type: the size read through plain views. the image
                     * reads it ahead of list_max[i] (0x8c61e9d0, 0x8c61e9d4),
                     * and loads size[i] ahead of list_limit[i] (0x8c61e9f4,
                     * 0x8c61e9f6); read volatile, 5 of 1428 words differ */
                    if (*(KMDWORD *)size[i] > kmiIntStatisticData.list_max[i]) {
                        kmiIntStatisticData.list_max[i] = *size[i];
                    }
                    if (*(KMDWORD *)size[i] >
                        kmiDeviceExtension.list_limit[i]) {
                        /* @hack spelling: the five passes of the loop that cuts
                         * the lists back, spelled as SHC's unroller lays them
                         * out: four bodies in a loop of one pass, whose test is
                         * j < 1, then a loop for the fifth. spelled so, the
                         * first and third bodies can reach the start through
                         * row, which their views need. as one loop of five
                         * passes, 12 of 1428 words differ */
                        for (j = 0; j < 1;) {
                            if ((KMDWORD)j !=
                                    kmiDeviceExtension.vertex_buffer_list &&
                                (LIST_BIT(j) & kmiDeviceExtension.lists_used)) {
                                *control =
                                    (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                                /* @hack spelling: the start reached through
                                 * row, which a comma sets to &start[j] after
                                 * the first copy. SHC then keeps j * 4 at @R15
                                 * and &start[j] at @(8,R15) (0x8c61ea32,
                                 * 0x8c61ea38, 0x8c61ea44, 0x8c61ea52); without
                                 * it, 12 of 1428 words differ */
                                row = (kmiMemCopy4(*start[j], &header,
                                                   sizeof(header)),
                                       &start[j]);
                                kmiMemCopy4(**row + (sizeof(header) / 4),
                                            (void *)&list_end, 4);
                                *size[j] = sizeof(header);
                            } else {
                                *size[j] = 0;
                            }
                            j++;
                            if ((KMDWORD)j !=
                                    kmiDeviceExtension.vertex_buffer_list &&
                                (LIST_BIT(j) & kmiDeviceExtension.lists_used)) {
                                *control =
                                    (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                                kmiMemCopy4(*start[j], &header, sizeof(header));
                                kmiMemCopy4(*start[j] + (sizeof(header) / 4),
                                            (void *)&list_end, 4);
                                *size[j] = sizeof(header);
                            } else {
                                *size[j] = 0;
                            }
                            j++;
                            if ((KMDWORD)j !=
                                    kmiDeviceExtension.vertex_buffer_list &&
                                (LIST_BIT(j) & kmiDeviceExtension.lists_used)) {
                                *control =
                                    (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                                /* @hack spelling: the start reached through
                                 * row, as in the first body, for j * 4 at @R15
                                 * and &start[j] at @(8,R15) (0x8c61eafa,
                                 * 0x8c61eb00, 0x8c61eb0c, 0x8c61eb1a); without
                                 * it, 4 of 1428 words differ */
                                row = (kmiMemCopy4(*start[j], &header,
                                                   sizeof(header)),
                                       &start[j]);
                                kmiMemCopy4(**row + (sizeof(header) / 4),
                                            (void *)&list_end, 4);
                                *size[j] = sizeof(header);
                            } else {
                                *size[j] = 0;
                            }
                            j++;
                            if ((KMDWORD)j !=
                                    kmiDeviceExtension.vertex_buffer_list &&
                                (LIST_BIT(j) & kmiDeviceExtension.lists_used)) {
                                *control =
                                    (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                                kmiMemCopy4(*start[j], &header, sizeof(header));
                                kmiMemCopy4(*start[j] + (sizeof(header) / 4),
                                            (void *)&list_end, 4);
                                *size[j] = sizeof(header);
                            } else {
                                *size[j] = 0;
                            }
                            j++;
                        }
                        for (; j < LISTS; j++) {
                            if ((KMDWORD)j !=
                                    kmiDeviceExtension.vertex_buffer_list &&
                                (LIST_BIT(j) & kmiDeviceExtension.lists_used)) {
                                *control =
                                    (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                                kmiMemCopy4(*start[j], &header, sizeof(header));
                                kmiMemCopy4(*start[j] + (sizeof(header) / 4),
                                            (void *)&list_end, 4);
                                *size[j] = sizeof(header);
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
            /* @hack type: the size stored through a plain view, which lets the
             * image load list_end's address ahead of the store (0x8c61ed2c,
             * 0x8c61ed32); stored volatile, 6 of 1428 words differ */
            *(KMDWORD *)size[i] = (kmiCurrentVertexBuff[i] - *start[i]) * 4;
            kmiMemCopy4(kmiCurrentVertexBuff[i], (void *)&list_end, 4);
            kmiCurrentVertexBuff[i] += END_OF_LIST / 4;
            if (!(LIST_BIT(i) & kmiDeviceExtension.lists_used)) {
                *size[i] = 0;
            }
            if (LIST_BIT(i) & kmiDeviceExtension.lists_used) {
                /* @hack type: the size read through a plain view. the image
                 * reads it ahead of list_max[i] (0x8c61ed80, 0x8c61ed82); read
                 * volatile, 7 of 1428 words differ */
                if (*(KMDWORD *)size[i] > kmiIntStatisticData.list_max[i]) {
                    kmiIntStatisticData.list_max[i] = *size[i];
                }
                if (*size[i] > kmiDeviceExtension.list_limit[i]) {
                    for (j = 0; j < LISTS; j++) {
                        if (LIST_BIT(j) & kmiDeviceExtension.lists_used) {
                            *control =
                                (*control & ~LIST_TYPE_MASK) | LIST_TYPE(j);
                            kmiMemCopy4(*start[j], &header, sizeof(header));
                            kmiMemCopy4(*start[j] + (sizeof(header) / 4),
                                        (void *)&list_end, 4);
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
    vertex_bank = (vertex_bank + 1) & 1;
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
    KMDWORD saved_clip_y;
    KMDWORD saved_clip_x;
    KMDWORD saved_tag;
    KMDWORD *queue;
    /* @hack redundant: register on slot, which gives it R14 (0x8c61f07a);
     * without it, the function is 218 bytes shorter and 722 of 742 words
     * differ */
    register KMDWORD slot;
    KMDWORD bank;
    volatile unsigned char *start;
    volatile unsigned char *size;
    KMUINT32 list;
    KMSTATUS status;
    int timeout;
    /* @hack redundant: 32 bytes of frame, sp+24 to sp+55, that no
     * instruction reads or writes, of a type unknown. sp+0 to sp+23, below
     * them, hold the values the code keeps in the frame, and the 32 bytes
     * above them hold the saved registers (0x8c61f00c to 0x8c61f01a). the
     * image's ADD #-56,R15 reserves them with the 24 bytes below (0x8c61f01c);
     * without them, 2 of 742 words differ */
    KMBYTE unk_18[32];
    KMDWORD stack;

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
    STARTS.opaque[slot] = kmiVertexBuff[KM_OPAQUE_POLYGON];
    STARTS.opaque_modifier[slot] = kmiVertexBuff[KM_OPAQUE_MODIFIER];
    STARTS.translucent[slot] = kmiVertexBuff[KM_TRANS_POLYGON];
    STARTS.translucent_modifier[slot] = kmiVertexBuff[KM_TRANS_MODIFIER];
    STARTS.punchthrough[slot] = kmiVertexBuff[KM_PUNCHTHROUGH_POLYGON];
    for (list = 0; list < LISTS; list++) {
        SIZE_AT(size) = (kmiCurrentVertexBuff[list] - START_AT(start)) * 4;
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
            /* @hack spelling: vertex_timeout read through its address. SHC then
             * loads vsync_count's and vertex_timeout's addresses (0x8c61f3e4,
             * 0x8c61f3e6) before it reads either; read plainly, vertex_timeout
             * is read first and 2 of 742 words differ */
            if (kmiDeviceExtension.vsync_count >
                *(&kmiDeviceExtension.vertex_timeout)) {
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
        /* @hack spelling: render_timeout read through its address. SHC then
         * loads vsync_count's and render_timeout's addresses (0x8c61f4fe,
         * 0x8c61f500) before it reads either; read plainly, render_timeout is
         * read first and 2 of 742 words differ */
        if (kmiDeviceExtension.vsync_count >
            *(&kmiDeviceExtension.render_timeout)) {
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
             * loads vsync_count's and render_timeout's addresses (0x8c61f6d4,
             * 0x8c61f6d6) before it reads either; read plainly, render_timeout
             * is read first and 2 of 742 words differ */
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
     * pointer, which the image stores (MOV.L R15,@R3 at 0x8c61f7a0); set
     * explicitly, to &saved_tag, the function is 4 bytes longer and 50 of 742
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
    /* @hack redundant: register on slot, which gives it R14 (0x8c61f83a);
     * without it, 54 of 237 words differ */
    register KMDWORD slot;
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
         * loads vsync_count's and vertex_timeout's addresses (0x8c61f856,
         * 0x8c61f858) before it reads either; read plainly, vertex_timeout is
         * read first and 2 of 237 words differ */
        if (kmiDeviceExtension.vsync_count >
            *(&kmiDeviceExtension.vertex_timeout)) {
            timeout = 1;
            break;
        }
    }
    if (timeout) {
        /* @hack implicit: R0 still holds &vertex_timeout, loaded for the
         * wait's compare (0x8c61f856); returned explicitly, 187 of 237 words
         * differ and kmSetFramebufferTexture starts 20 bytes early */
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
        /* @hack spelling: render_timeout read through its address. SHC then
         * loads vsync_count's and render_timeout's addresses (0x8c61f88c,
         * 0x8c61f88e) before it reads either; read plainly, render_timeout is
         * read first and 2 of 237 words differ */
        if (kmiDeviceExtension.vsync_count >
            *(&kmiDeviceExtension.render_timeout)) {
            timeout = 1;
            break;
        }
    }
    if (timeout) {
        kmiDeviceExtension.render_busy = 0;
        /* @hack implicit: R0 still holds &render_timeout, loaded for the
         * wait's compare (0x8c61f88c); returned explicitly, 72 of 237 words
         * differ, and a pool word */
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
    /* @hack spelling: timeout set from vsync_count's store, which copies the 0
     * R12 holds (MOV R12,R11 at 0x8c61f91a); set apart, as for the two waits
     * before it, MOV #0,R11 takes its place and 1 of 237 words differ */
    timeout = kmiDeviceExtension.vsync_count = 0;
    while (kmiDeviceExtension.render_busy != 0) {
        RUN_WAIT_VSYNC();
        /* @hack spelling: render_timeout read through its address. SHC then
         * loads vsync_count's and render_timeout's addresses (0x8c61f924,
         * 0x8c61f926) before it reads either; read plainly, render_timeout is
         * read first and 2 of 237 words differ */
        if (kmiDeviceExtension.vsync_count >
            *(&kmiDeviceExtension.render_timeout)) {
            timeout = 1;
            break;
        }
    }
    if (timeout) {
        kmiDeviceExtension.texture_render = 0;
        /* @hack implicit: R0 still holds &render_timeout, loaded for the
         * wait's compare (0x8c61f924); returned explicitly, 78 of 237 words
         * differ, and 18 pool words */
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
    /* @hack spelling: slots_held set from ta_busy's store, which keeps
     * saved_clip_y at @R15 and the surface's height at @(8,R15) (0x8c61f8bc,
     * 0x8c61f8c2); set apart, the two swap and 4 of 237 words differ */
    kmiDeviceExtension.slots_held = kmiDeviceExtension.ta_busy = 1;
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
     * tests the image makes (CMP/EQ #4,R0 and CMP/EQ #5,R0 at 0x8c61fabe and
     * 0x8c61fac2); without them, 41 of 70 words differ and kmResetPointer
     * starts 32 bytes early */
    case KM_DSPBPP_RGB888:
    case KM_DSPBPP_ARGB8888:
    default:
        return KMSTATUS_INVALID_TEXTURE_TYPE;
        /* @hack redundant: a break after the return, which SHC deletes, but
         * which leaves R2 for the type's 2 (MOV #2,R2 at 0x8c61fb18); without
         * it, R2 and R3 swap and 7 of 70 words differ */
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

void kmResetPointer(void)
{
    KMDWORD bank;

    if (kmiDeviceExtension.latency_model == KM_LATENCYMODEL_2V) {
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
    }
    kmiFlipVertexBuffer(vertex_bank);
}
