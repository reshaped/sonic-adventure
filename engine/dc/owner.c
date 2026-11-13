/* the Dreamcast's submission owner. it owns the frame's lists, its flips and
 * the video chip's interrupts in place of Kamui's frame code. make dc links
 * it into 1ST_READ.BIN. it defines the six calls engine/mk/owner.mk's
 * OWNER_SURFACE names, and kmiInitInterrupt, which installs this file's
 * handler. make dc compiles Kamui's kmsurfac and interupt without their
 * frame code. the game's draws write the opaque list straight to the tile
 * accelerator, through the store queues. the others go to the vertex buffer
 * in RAM.
 *
 * Kamui sets the chip, the display and the banks up and allocates the vertex
 * buffer. its kmiFlipVertexBuffer starts each list over. it keeps the
 * textures and runs the DMA queue, which texture loads share with this
 * file's lists. the owner keeps Kamui's frame state in kmiDeviceExtension and
 * kmiIntStatisticData, which present.c, overlay.c and Kamui's services read.
 *
 * the owner runs only the configuration the game sets, Kamui's 2V latency
 * under a NOBUF model. njInitVertexBuffer gives it for a negative opaque
 * size. the owner refuses kmRenderDirect, the texture renders and any other
 * configuration with KMSTATUS_NOT_IMPLEMENTED, and sends nothing. the game
 * asks for none of them. a flip runs the wait callback while it waits, and
 * polygon.c builds the collision index in that callback. the owner sets
 * texture_render and slots_held only to 0, so its handler doesn't read them.
 *
 * at 60 fps the owner runs the frame owner.h describes, and at 30 Kamui's,
 * step for step. ENGINE.md#dreamcast gives the seam it replaces */
#include <kamui.h>
#include <kmistat.h>
#include <machine.h>

#include "owner.h"

/* a long word of the machine's, by its address. engine/tests/host/machine.h
 * gives owner_slots.c's own, for a host that maps nothing at these
 * addresses */
#ifndef WORD
#define WORD(address) (*(volatile KMDWORD *)(address))
#endif

/* the store queues' address control registers and area, as kmsurfac's */
#define QACR0 WORD(0xff000038)
#define QACR1 WORD(0xff00003c)
#define QACR_AREA(address) ((address) >> 24)
#define SQ_ADDRESS 0x03ffffffUL
#define SQ_BASE 0xe0000000UL

/* the CPU's timer 0 count, which counts down */
#define TCNT0 WORD(0xffd8000c)
/* DMA channel 2's control register, and its enable and transfer end bits */
#define CHCR2 WORD(0xffa0002c)
#define CHCR_ENABLE 0x1
#define CHCR_TRANSFER_END 0x2

/* the system bus's interrupt status, normal and error, each with its bits.
 * writing 1 to a bit clears it */
#define INT_STATUS WORD(0xa05f6900)
#define INT_RENDER_END 0x07 /* a render's end bits: video, ISP and TSP */
#define INT_RENDER_END_TSP 0x04
#define INT_VBLANK_IN 0x08
#define INT_HBLANK_IN 0x20
#define INT_YUV_END 0x40
#define INT_OPAQUE_END 0x80
#define INT_OPAQUE_MODIFIER_END 0x100
#define INT_TRANS_END 0x200
#define INT_TRANS_MODIFIER_END 0x400
#define INT_DMA2_END 0x80000
#define INT_PUNCHTHROUGH_END 0x200000
#define INT_STATUS_ERROR WORD(0xa05f6908)
#define INT_STRIP_OVERRUN 0x02
#define INT_ISP_OVERFLOW 0x04
#define INT_OL_OVERFLOW 0x08

/* each list's end bit in INT_STATUS, by ListType */
static const KMDWORD list_ends[] = {
    INT_OPAQUE_END,         INT_OPAQUE_MODIFIER_END, INT_TRANS_END,
    INT_TRANS_MODIFIER_END, INT_PUNCHTHROUGH_END,
};

/* the video chip's registers, by address for the handler */
#define SOFT_RESET WORD(0xa05f8008)
#define START_RENDER WORD(0xa05f8014)
#define PARAM_BASE WORD(0xa05f8020)
#define REGION_BASE WORD(0xa05f802c)
#define FB_READ_CONTROL WORD(0xa05f8044)
#define FB_WRITE_CONTROL WORD(0xa05f8048)
#define FB_WRITE_STRIDE WORD(0xa05f804c)
#define FB_READ_START1 WORD(0xa05f8050)
#define FB_READ_START2 WORD(0xa05f8054)
#define FB_WRITE_START WORD(0xa05f8060)
#define FB_CLIP_X WORD(0xa05f8068)
#define FB_CLIP_Y WORD(0xa05f806c)
#define BACKGROUND_DEPTH WORD(0xa05f8088)
#define BACKGROUND_TAG WORD(0xa05f808c)
#define VIDEO_CONTROL WORD(0xa05f80e8)
#define SCALER_CONTROL WORD(0xa05f80f4)
#define SCALER_INTERLACE 0x20000
#define SCALER_FIELD 0x40000
#define SYNC_STATUS WORD(0xa05f810c)
#define SYNC_FIELD 0x400
#define SYNC_FIELD_SHIFT 10
#define TA_ISP_CURRENT WORD(0xa05f8138)
/* and by offset, for kmiWriteRegister in open_bank */
#define SOFT_RESET_REG 0x008
#define TA_OL_BASE_REG 0x124
#define TA_ISP_BASE_REG 0x128
#define TA_OL_LIMIT_REG 0x12c
#define TA_ISP_LIMIT_REG 0x130
#define TA_LIST_INIT_REG 0x144
#define TA_NEXT_OPB_INIT_REG 0x164
#define RESET_TA 1
#define RESET_RENDERER 2
#define RESET_SDRAM 4
#define FB_READ_ON 1
#define LIST_INIT 0x80000000UL

/* texture memory, uncached */
#define TEXTURE_MEMORY 0xa5000000UL
/* the tile accelerator's polygon input, as a DMA and store queue target */
#define TA_INPUT 0x10000000UL
/* the end-of-list parameter's bytes, which follow each list */
#define END_OF_LIST 32

#define LISTS 5
#define SLOTS 4
#define LIST_BIT(type) (1UL << (type))
#define LIST_TYPE(type) ((KMDWORD)(type) << 24)
#define LIST_TYPE_MASK 0x0f000000UL
#define LIST_TYPE_BITS 7
/* vertex_buffer_model's bit that a NOBUF model sets */
#define NOBUF 0x20

/* a flip's second wait gives up past this vsync_count, as Kamui's */
#define FLIP_RETRY_TIMEOUT 20

/* the background packet's words, and BACKGROUND_TAG's fields */
#define PACKET_WORDS 48
#define TAG_SKIP 0xff000000UL
#define TAG_OFFSET(bytes) ((bytes) << 1 & 0x00fffff8UL)

/* an overflow's growth, in bytes. the ISP parameters grow up by ISP_STEP and
 * the object lists down by OL_STEP. the limits stay LIMIT_GAP apart, and
 * ol_limit stays more than OL_FLOOR above isp_base */
#define ISP_STEP 0x2000
#define OL_STEP 0x800
#define OL_FLOOR 0x140
#define OL_OVERFLOW_LIMIT 10
#define LIMIT_GAP (PACKET_WORDS * 4)

/* BY_COLUMNS where ta_options holds OPTION_BY_COLUMNS, and else BY_ROWS. the
 * complement gives that only because BY_COLUMNS is 0, and BY_ROWS and
 * OPTION_BY_COLUMNS are both 1 */
#define REGION_ORDER (~kmiDeviceExtension.ta_options & OPTION_BY_COLUMNS)

#define RUN(callback) (1UL << (callback))

#define DEVICE kmiDeviceExtension
#define STATS kmiIntStatisticData

/* the end-of-list control word */
static const KMDWORD list_end = 0;
/* an empty list's header, whose type each use sets */
static const struct list_header empty_header = {
    0x80000000,
    0x00000000,
    0x00800400,
    0x00000000,
    {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};

/* the bank kmRender gives kmiFlipVertexBuffer, switched by each render */
static KMDWORD vertex_bank;
/* the field the last vblank began */
static KMDWORD field;
static KMDWORD field_toggle = 1;
/* set where time_out_slots finds a slot under way. count_down then lets
 * through the next vblank it counts */
static KMDWORD slot_busy;
/* 1 from a render's start until its first strip overrun, which queues no
 * callback */
static KMINT32 render_started;
/* TRUE while the 60 fps frame runs. owner_sixty starts it, and vblank_in
 * ends it */
static int sixty;
/* what owner_sixty last asked */
static int sixty_asked;
/* at 60 fps, TRUE from a flip that waits for a vblank to that vblank */
static int vblank_awaited;
/* each slot's background plane as its lists' end left it. the tag locates
 * the plane's packet in the slot's bank. the device holds one plane: the last
 * end's tag and the depth the game last set. at 60 fps both can change
 * before a slot renders */
static struct plane {
    KMDWORD tag;
    KMDWORD depth;
} planes[SLOTS];

/* kmsurfac's, which kamui.h doesn't declare: write kmiDummyOpaque at the
 * opaque list's write pointer */
void kmiSetOpaqueDummy(void);

/* TRUE where the configuration is the one the owner runs */
static int owned(void)
{
    return DEVICE.latency_model == KM_LATENCYMODEL_2V &&
           (DEVICE.vertex_buffer_model & NOBUF) != 0;
}

/* reset the tile accelerator and open bank's lists, whose ends the handler
 * counts from here */
static void open_bank(KMDWORD bank)
{
    kmiWriteRegister(SOFT_RESET_REG, RESET_TA);
    kmiWriteRegister(SOFT_RESET_REG, 0);
    kmiWriteRegister(TA_OL_BASE_REG, DEVICE.ta_bank[bank].ol_base);
    kmiWriteRegister(TA_OL_LIMIT_REG, DEVICE.ta_bank[bank].ol_limit);
    kmiWriteRegister(TA_ISP_BASE_REG, DEVICE.ta_bank[bank].isp_base);
    kmiWriteRegister(TA_ISP_LIMIT_REG, DEVICE.ta_bank[bank].isp_limit);
    kmiWriteRegister(TA_NEXT_OPB_INIT_REG, DEVICE.ta_bank[bank].next_opb_init);
    kmiWriteRegister(TA_LIST_INIT_REG, LIST_INIT);
    kmiReadRegister(TA_LIST_INIT_REG);
    DEVICE.lists_done = 0;
    DEVICE.lists_sent = DEVICE.lists_used;
}

/* cut slot's buffered lists in use back to an empty header and its end */
static void cut_back(KMDWORD slot, KMDWORD direct)
{
    struct list_header header = empty_header;
    KMDWORD list;

    for (list = 0; list < LISTS; list++) {
        if (list != direct && (DEVICE.lists_used & LIST_BIT(list))) {
            header.control =
                (header.control & ~LIST_TYPE_MASK) | LIST_TYPE(list);
            kmiMemCopy4(DEVICE.list_start.row[list][slot], &header,
                        sizeof(header));
            kmiMemCopy4(DEVICE.list_start.row[list][slot] + LIST_HEADER_WORDS,
                        (void *)&list_end, 4);
            DEVICE.list_size[list][slot] = sizeof(header);
        } else {
            DEVICE.list_size[list][slot] = 0;
        }
    }
}

/* end the write slot's buffered lists and size them. an empty direct list is
 * sent a header, so that the tile accelerator has a list to end */
KMSTATUS kmRender(void)
{
    KMDWORD slot = DEVICE.write_slot;
    KMDWORD direct = DEVICE.vertex_buffer_list;
    struct list_header header = empty_header;
    KMDWORD *queue;
    KMDWORD list;
    KMDWORD size;

    if (!owned()) {
        return KMSTATUS_NOT_IMPLEMENTED;
    }
    if ((DEVICE.capabilities & KMI_CAP_DUMMY_OPAQUE) && DEVICE.opaque_opb &&
        DEVICE.translucent_opb && DEVICE.presort) {
        kmiSetOpaqueDummy();
    }
    for (list = 0; list < LISTS; list++) {
        DEVICE.list_start.row[list][slot] = kmiVertexBuff[list];
    }
    for (list = 0; list < LISTS; list++) {
        if (list == direct) {
            continue;
        }
        if (!(DEVICE.lists_used & LIST_BIT(list))) {
            DEVICE.list_size[list][slot] = 0;
            continue;
        }
        size =
            (kmiCurrentVertexBuff[list] - DEVICE.list_start.row[list][slot]) *
            4;
        DEVICE.list_size[list][slot] = size;
        kmiMemCopy4(kmiCurrentVertexBuff[list], (void *)&list_end, 4);
        kmiCurrentVertexBuff[list] += END_OF_LIST / 4;
        if (size > STATS.list_max[list]) {
            STATS.list_max[list] = size;
        }
        if (size > DEVICE.list_limit[list]) {
            cut_back(slot, direct);
            kmiFatalErrorEntry(KMI_FATAL_LIST_LIMIT);
            break;
        }
    }
    if (kmiCurrentVertexBuff[direct] == DEVICE.list_start.row[direct][slot]) {
        QACR0 = QACR_AREA(TA_INPUT);
        QACR1 = QACR_AREA(TA_INPUT);
        header.control = (header.control & ~LIST_TYPE_MASK) |
                         LIST_TYPE(direct & LIST_TYPE_BITS);
        queue =
            (KMDWORD *)(((KMDWORD)kmiCurrentVertexBuff[direct] & SQ_ADDRESS) |
                        SQ_BASE);
        kmiMemCopy4(queue, &header, sizeof(header));
        prefetch(queue);
    }
    vertex_bank = (vertex_bank + 1) & 1;
    kmiFlipVertexBuffer(vertex_bank);
    return KMSTATUS_SUCCESS;
}

/* send slot's buffered lists, each with its end, by DMA. while
 * common_modifier is set, its words go ahead of each modifier list */
static void send_buffered(KMDWORD slot, KMDWORD direct)
{
    KMDWORD *handle;
    KMDWORD list;

    for (list = 0; list < LISTS; list++) {
        if (list == direct || DEVICE.list_size[list][slot] == 0) {
            continue;
        }
        if (DEVICE.common_modifier == 1 &&
            (list == KM_OPAQUE_MODIFIER || list == KM_TRANS_MODIFIER)) {
            *DEVICE.common_modifier_data =
                (*DEVICE.common_modifier_data & ~LIST_TYPE_MASK) |
                LIST_TYPE(list);
            while (kmiDMAtoTARequest(TA_INPUT, DEVICE.common_modifier_data,
                                     DEVICE.common_modifier_size, DMA_SOURCE_UP,
                                     &handle) == DMA_QUEUE_FULL) {
            }
        }
        while (kmiDMAtoTARequest(TA_INPUT, DEVICE.list_start.row[list][slot],
                                 DEVICE.list_size[list][slot] + END_OF_LIST,
                                 DMA_SOURCE_UP, &handle) == DMA_QUEUE_FULL) {
        }
    }
}

/* wait, running the wait callback, until the handler clears flip_pending and
 * the flip goes on. past the vertex and render timeouts it waits
 * FLIP_RETRY_TIMEOUT vsyncs more, and then stops on KMI_FATAL_FLIP_TIMEOUT.
 * it stays out of line, so that a profile tells the wait from the sending */
static void flip_wait(void) __attribute__((noinline));
static void flip_wait(void)
{
    DEVICE.vsync_count = 0;
    while (DEVICE.flip_pending != 0) {
        RUN_WAIT_VSYNC();
        if (DEVICE.vsync_count >
            DEVICE.vertex_timeout + DEVICE.render_timeout + 1) {
            /* Kamui's test follows the callback, so a flip that ends there
             * late clears render_busy too */
            DEVICE.vsync_count = DEVICE.render_busy = 0;
            while (DEVICE.flip_pending != 0) {
                RUN_WAIT_VSYNC();
                if (DEVICE.vsync_count > FLIP_RETRY_TIMEOUT) {
                    kmiFatalErrorEntry(KMI_FATAL_FLIP_TIMEOUT);
                    return;
                }
            }
            return;
        }
    }
}

/* hand the write slot to the tile accelerator. once the flip goes on, open
 * the next slot's bank. at 60 fps, a flip with go_on set waits for no
 * vblank */
static KMSTATUS flip(int go_on)
{
    KMDWORD slot = DEVICE.write_slot;

    if (!owned()) {
        return KMSTATUS_NOT_IMPLEMENTED;
    }
    vblank_awaited = !go_on;
    STATS.ta_start = TCNT0;
    DEVICE.ta_slot = slot;
    DEVICE.ta_busy = 1;
    DEVICE.lists_done = 0;
    DEVICE.lists_sent = DEVICE.lists_used;
    DEVICE.flip_pending = 1;
    STATS.ta_wait = 0;
    DEVICE.slot_state[slot] = SLOT_IN_TA;
    QACR0 = QACR_AREA(TA_INPUT);
    QACR1 = QACR_AREA(TA_INPUT);
    kmiMemCopy4((void *)SQ_BASE, (void *)&list_end, 4);
    prefetch((void *)SQ_BASE);
    send_buffered(slot, DEVICE.vertex_buffer_list);
    flip_wait();
    DEVICE.write_slot = (DEVICE.write_slot + 1) & (SLOTS - 1);
    DEVICE.slot_state[DEVICE.write_slot] = SLOT_FILLING;
    open_bank(DEVICE.write_slot & 1);
    return KMSTATUS_SUCCESS;
}

/* a call as compiled, even where the link-time compile could inline or
 * clone it: a debugger counts the flips at its entry */
__attribute__((noipa)) KMSTATUS kmFlipFrameBuffer(void)
{
    return flip(0);
}

KMSTATUS owner_flip_on(void)
{
    return flip(sixty);
}

void owner_sixty(int on)
{
    sixty_asked = on;
    if (on) {
        sixty = 1;
    }
}

/* throw the write slot's lists away: open its bank again, and start each
 * list over */
void kmResetPointer(void)
{
    if (owned()) {
        open_bank(DEVICE.write_slot & 1);
    }
    kmiFlipVertexBuffer(vertex_bank);
}

KMSTATUS kmRenderDirect(void)
{
    return KMSTATUS_NOT_IMPLEMENTED;
}

KMSTATUS kmRenderTexture(PKMSURFACEDESC pTextureSurface)
{
    return KMSTATUS_NOT_IMPLEMENTED;
}

KMSTATUS kmRenderTextureDirect(PKMSURFACEDESC surface)
{
    return KMSTATUS_NOT_IMPLEMENTED;
}

/* show the first rendered slot. under FLICKER_FREE each field reads its own
 * frame buffer, and otherwise both read its bank's. a pending display change
 * goes to the chip with it */
static void show_rendered(void)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (DEVICE.slot_state[i] != SLOT_RENDERED &&
            DEVICE.slot_state[i] != SLOT_OVERFLOW) {
            continue;
        }
        DEVICE.slot_state[i] = SLOT_SHOWN;
        if (FLICKER_FREE) {
            FB_READ_START1 = DEVICE.fb_start[0] + DEVICE.field_offset[0];
            FB_READ_START2 = DEVICE.fb_start[1] + DEVICE.field_offset[1];
        } else if (DEVICE.strip_buffer != 1) {
            FB_READ_START1 = DEVICE.fb_start[i & 1] + DEVICE.field_offset[0];
            FB_READ_START2 = DEVICE.fb_start[i & 1] + DEVICE.field_offset[1];
        }
        /* an overflowed slot makes the change too, as in Kamui's, whose
         * SLOT_OVERFLOW test always passes */
        if (DEVICE.display_changed == 1) {
            VIDEO_CONTROL = DEVICE.video_control;
            FB_READ_CONTROL = DEVICE.fb_read_control;
            DEVICE.display_changed = 0;
        }
        return;
    }
}

/* render slot i over the background plane of tag and depth */
static void render_slot(int i, KMDWORD tag, KMDWORD depth)
{
    int bank = i & 1;

    if (DEVICE.regions_stale & (bank + 1)) {
        kmiCreateRegionArray(DEVICE.tiles_x, DEVICE.tiles_y, 0, 1, bank,
                             REGION_ORDER);
        DEVICE.regions_stale = DEVICE.regions_stale & ~(bank + 1) & 3;
    }
    DEVICE.render_busy = 1;
    DEVICE.slot_state[i] = SLOT_RENDERING;
    if (DEVICE.capabilities & KMI_CAP_RESET_BEFORE_RENDER) {
        SOFT_RESET = RESET_RENDERER;
        SOFT_RESET = 0;
    }
    REGION_BASE = DEVICE.ta_bank[bank].region_base;
    PARAM_BASE = DEVICE.ta_bank[bank].isp_base;
    if (DEVICE.strip_buffer != 1) {
        FB_WRITE_START =
            FLICKER_FREE ? DEVICE.fb_start[field] : DEVICE.fb_start[bank];
    }
    BACKGROUND_TAG = tag;
    BACKGROUND_DEPTH = depth & ~15;
    FB_CLIP_X = DEVICE.fb_clip_x;
    FB_CLIP_Y = DEVICE.fb_clip_y;
    FB_WRITE_STRIDE = DEVICE.fb_write_stride;
    FB_WRITE_CONTROL = DEVICE.fb_write_control;
    STATS.render_wait = 0;
    if ((DEVICE.display_mode & ~(KM_PAL | KM_DSPMODE_WIDTH_640)) ==
        KM_DSPMODE_HEIGHT_480) {
        SCALER_CONTROL = DEVICE.scaler_control | SCALER_INTERLACE |
                         (field == 0 ? 0 : SCALER_FIELD);
    }
    STATS.render_start = TCNT0;
    DEVICE.render_slot = i;
    render_started = 1;
    START_RENDER = 0xffffffff;
}

/* render the first slot the tile accelerator finished, over the device's
 * plane, as Kamui's handler does. Kamui's frame starts each slot's render as
 * its flip goes on. the device's plane, the last end's, is then that slot's */
static void start_render(void)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (DEVICE.slot_state[i] == SLOT_TA_DONE && DEVICE.render_busy == 0) {
            render_slot(i, DEVICE.background_tag, DEVICE.background_depth);
            return;
        }
    }
}

/* at 60 fps, render the oldest slot the tile accelerator finished, over its
 * own plane. it starts none while the renderer is busy or a slot renders.
 * while a flip waits, its own slot and the one before it can both be
 * finished */
static void start_oldest(void)
{
    int k;
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (DEVICE.slot_state[i] == SLOT_RENDERING) {
            return;
        }
    }
    for (k = 1; k <= SLOTS; k++) {
        i = (DEVICE.write_slot + k) & (SLOTS - 1);
        if (DEVICE.slot_state[i] == SLOT_TA_DONE) {
            if (DEVICE.render_busy == 0) {
                render_slot(i, planes[i].tag, planes[i].depth);
            }
            return;
        }
    }
}

/* count a vblank off vsync_remaining. the result is 0 to let the vblank
 * through, and 1 to hold the slots. in an interlaced mode a vblank that
 * begins field 1 holds them uncounted */
static int count_down(void)
{
    if ((DEVICE.display_mode & KM_DSPMODE_INTERLACE) == KM_DSPMODE_INTERLACE &&
        field == 1) {
        return 1;
    }
    DEVICE.vsync_remaining--;
    if (DEVICE.vsync_remaining < 0) {
        DEVICE.vsync_remaining = DEVICE.vsync_wait;
        return 0;
    }
    if (slot_busy != 0) {
        slot_busy = 0;
        return 0;
    }
    return 1;
}

/* stop a transfer to the tile accelerator found running past
 * vertex_timeout checks, freeing its slot */
static void time_out_transfer(int slot)
{
    if (++STATS.ta_wait > (KMDWORD)DEVICE.vertex_timeout) {
        STATS.ta_timeouts++;
        kmiDMAHazardReset();
        DEVICE.slot_state[slot] = SLOT_FREE;
        DEVICE.ta_busy = 0;
        STATS.ta_wait = 0;
    }
}

/* the timeout check at a vblank count_down lets through. it returns 1 to
 * hold the slots while a slot is under way or render_busy is set. with
 * render_timeout 0, the first check stops a render instead. a render past
 * render_timeout checks is stopped, with the SDRAM interface reset and both
 * region arrays rebuilt */
static int time_out_slots(void)
{
    int busy = 0;
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (DEVICE.slot_state[i] < 0 || (DEVICE.slot_state[i] & 1)) {
            continue;
        }
        if (DEVICE.render_timeout == 0) {
            if (DEVICE.slot_state[i] == SLOT_RENDERING) {
                DEVICE.slot_state[i] = SLOT_RENDERED;
                if (DEVICE.render_busy != 0) {
                    STATS.render_timeouts++;
                }
                DEVICE.render_busy = 0;
                SOFT_RESET = RESET_RENDERER;
                SOFT_RESET = 0;
                continue;
            }
            slot_busy = 1;
            busy = 1;
            if (DEVICE.slot_state[i] == SLOT_IN_TA) {
                time_out_transfer(i);
            }
            continue;
        }
        slot_busy = 1;
        busy = 1;
        if (DEVICE.slot_state[i] == SLOT_RENDERING &&
            ++STATS.render_wait > (KMDWORD)DEVICE.render_timeout) {
            STATS.render_timeouts++;
            FB_READ_CONTROL = DEVICE.fb_read_control & ~FB_READ_ON;
            SOFT_RESET = RESET_RENDERER | RESET_SDRAM;
            SOFT_RESET = 0;
            FB_READ_CONTROL = DEVICE.fb_read_control;
            kmiCreateRegionArray(DEVICE.tiles_x, DEVICE.tiles_y, 0, 1, 0,
                                 REGION_ORDER);
            kmiCreateRegionArray(DEVICE.tiles_x, DEVICE.tiles_y, 0, 1, 1,
                                 REGION_ORDER);
            DEVICE.slot_state[i] = SLOT_RENDERED;
            DEVICE.render_busy = 0;
            STATS.render_wait = 0;
        }
        if (DEVICE.slot_state[i] == SLOT_IN_TA) {
            time_out_transfer(i);
        }
    }
    return busy | (DEVICE.render_busy != 0);
}

/* TRUE where a slot the tile accelerator finished waits for its render */
static int slot_waits(void)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (DEVICE.slot_state[i] == SLOT_TA_DONE) {
            return 1;
        }
    }
    return 0;
}

/* at 60 fps, let a pending flip go on once three things hold. its slot is
 * past the tile accelerator. the slot before it is past its render, since
 * the next slot takes that slot's bank. the vblank the flip waits for, if
 * any, has come. an overflowed slot counts as past both, as Kamui's handler
 * shows it as rendered */
static void release_flip(void)
{
    KMINT32 own = DEVICE.slot_state[DEVICE.write_slot];
    KMINT32 before =
        DEVICE.slot_state[(DEVICE.write_slot + SLOTS - 1) & (SLOTS - 1)];

    if (DEVICE.flip_pending != 0 && !vblank_awaited && own != SLOT_IN_TA &&
        before != SLOT_IN_TA && before != SLOT_TA_DONE &&
        before != SLOT_RENDERING) {
        DEVICE.flip_pending = 0;
    }
}

/* at 60 fps each vblank that count_down lets through shows the last render
 * and starts the next. the 60 fps frame ends where owner_sixty asked for
 * Kamui's and no finished slot waits. Kamui's frame holds the slots while
 * one fills. a slot left waiting would then wait out the next flip. that flip
 * can reopen the slot's bank before its render ends. the vblank that ends the
 * 60 fps frame lets a pending flip go on by release_flip. Kamui's frame would
 * hold that flip while a slot renders */
static void vblank_in(void)
{
    INT_STATUS = INT_VBLANK_IN;
    DEVICE.callback_pending |= RUN(VSYNC_CALLBACK);
    DEVICE.vsync_count++;
    if (DEVICE.capabilities & KMI_CAP_SYNC_STATUS) {
        field = (SYNC_STATUS & SYNC_FIELD) >> SYNC_FIELD_SHIFT;
    } else {
        field_toggle = (field_toggle + 1) & 1;
        field = field_toggle;
    }
    if (sixty) {
        if (count_down() == 0) {
            /* time_out_slots runs for its timeouts and slot_busy. at 60 fps
             * a slot under way holds nothing */
            time_out_slots();
            show_rendered();
            start_oldest();
            vblank_awaited = 0;
            if (!sixty_asked && !slot_waits()) {
                release_flip();
                sixty = 0;
            }
        }
        return;
    }
    if (count_down() == 0 && time_out_slots() == 0) {
        show_rendered();
        start_render();
        DEVICE.flip_pending = 0;
    }
}

/* the tile accelerator finished ta_slot. at 60 fps a vblank can start the
 * slot's render before the next flip. a list end after that leaves the slot
 * as it is */
static void ta_finished(void)
{
    KMDWORD bank = DEVICE.ta_slot & 1;
    KMDWORD end;
    KMDWORD tag;
    struct plane *plane;
    volatile KMDWORD *source;
    KMDWORD *destination;
    int n;

    if (DEVICE.slot_state[DEVICE.ta_slot] != SLOT_FREE &&
        !(sixty && DEVICE.slot_state[DEVICE.ta_slot] > SLOT_TA_DONE)) {
        DEVICE.callback_pending |= RUN(END_VERTEX_CALLBACK);
        DEVICE.slot_state[DEVICE.ta_slot] = SLOT_TA_DONE;
        end = (DEVICE.capabilities & KMI_CAP_ISP_CURRENT)
                  ? TA_ISP_CURRENT
                  : DEVICE.ta_bank[bank].isp_limit;
        STATS.isp_current[bank] = TA_ISP_CURRENT;
        tag = (DEVICE.background_skip & TAG_SKIP) |
              TAG_OFFSET(end - DEVICE.ta_bank[bank].isp_base);
        DEVICE.background_tag = tag;
        plane = &planes[DEVICE.ta_slot];
        plane->tag = tag;
        plane->depth = DEVICE.background_depth;
        source = DEVICE.background_packet;
        destination = (KMDWORD *)&WORD(end + TEXTURE_MEMORY);
        for (n = 0; n < PACKET_WORDS; n++) {
            *destination++ = *source++;
        }
    }
    DEVICE.ta_busy = 0;
    STATS.ta_time = STATS.ta_start - TCNT0;
}

/* give each bank's ISP parameters more room, or its object lists where lists
 * is set. a bank without room stops on KMI_FATAL_NO_ROOM once
 * isp_overflow_wait passes 0, or ol_overflow_wait passes OL_OVERFLOW_LIMIT */
static void grow(int lists)
{
    int bank;

    for (bank = 0; bank < 2; bank++) {
        volatile struct kmi_ta_bank *b = &DEVICE.ta_bank[bank];

        if (!lists) {
            if (b->isp_limit + ISP_STEP < b->ol_base - LIMIT_GAP) {
                b->isp_limit += ISP_STEP;
                b->ol_limit = b->isp_limit + LIMIT_GAP;
                STATS.isp_overflow_wait = 0;
            } else if (STATS.isp_overflow_wait > 0) {
                kmiFatalErrorEntry(KMI_FATAL_NO_ROOM);
            }
        } else if (b->ol_limit - OL_STEP > b->isp_base + OL_FLOOR) {
            if (STATS.isp_current[bank] < b->ol_limit - OL_STEP) {
                b->ol_limit -= OL_STEP;
                b->isp_limit = b->ol_limit - LIMIT_GAP;
            }
            STATS.ol_overflow_wait = 0;
        } else if (STATS.ol_overflow_wait > OL_OVERFLOW_LIMIT) {
            kmiFatalErrorEntry(KMI_FATAL_NO_ROOM);
        }
    }
}

/* the video chip's level 6 interrupt, which kmiInitInterrupt installs */
static KMINT32 handle_interrupt(void)
{
    KMDWORD status = INT_STATUS;
    KMDWORD list;
    int i;

    if (status & INT_VBLANK_IN) {
        vblank_in();
    }
    if (status & INT_HBLANK_IN) {
        INT_STATUS = INT_HBLANK_IN;
        DEVICE.callback_pending |= RUN(HSYNC_CALLBACK);
    }
    if (status & INT_RENDER_END_TSP) {
        INT_STATUS = INT_RENDER_END;
        DEVICE.render_busy = 0;
        STATS.render_wait = 0;
        STATS.render_time = STATS.render_start - TCNT0;
        DEVICE.slot_state[DEVICE.render_slot] = SLOT_RENDERED;
        DEVICE.callback_pending |= RUN(END_RENDER_CALLBACK);
    }
    if (status & INT_DMA2_END) {
        CHCR2 = CHCR2 & ~(CHCR_ENABLE | CHCR_TRANSFER_END);
        INT_STATUS = INT_DMA2_END;
        kmiExecuteDMAChain();
    }
    if (status & INT_YUV_END) {
        INT_STATUS = INT_YUV_END;
        DEVICE.callback_pending |= RUN(END_YUV_CALLBACK);
    }
    for (list = 0; list < LISTS; list++) {
        if (status & list_ends[list]) {
            INT_STATUS = list_ends[list];
            DEVICE.lists_done |= LIST_BIT(list);
            if (DEVICE.lists_sent == DEVICE.lists_done) {
                ta_finished();
            }
        }
    }
    status = INT_STATUS_ERROR;
    if (status & INT_ISP_OVERFLOW) {
        INT_STATUS_ERROR = INT_ISP_OVERFLOW;
        STATS.isp_overflows++;
        /* at 60 fps the next picture's direct list can overflow after
         * ta_slot is past the tile accelerator */
        if (!sixty || DEVICE.slot_state[DEVICE.ta_slot] == SLOT_IN_TA) {
            DEVICE.slot_state[DEVICE.ta_slot] = SLOT_OVERFLOW;
        }
        if (DEVICE.opb_downward == 1) {
            STATS.isp_overflow_wait++;
            grow(0);
        }
    }
    if (status & INT_OL_OVERFLOW) {
        INT_STATUS_ERROR = INT_OL_OVERFLOW;
        STATS.ol_overflows++;
        if (DEVICE.opb_downward == 1) {
            STATS.ol_overflow_wait++;
            grow(1);
        }
    }
    if (status & INT_STRIP_OVERRUN) {
        INT_STATUS_ERROR = INT_STRIP_OVERRUN;
        if (render_started == 0) {
            DEVICE.callback_pending |= RUN(STRIP_OVERRUN_CALLBACK);
        }
        render_started = 0;
    }
    /* while 60 fps is asked, any interrupt can let a flip go on. once Kamui's
     * frame is asked, no flip goes on before the vblank that ends the 60 fps
     * frame. no flip then leaves its slot finished and waiting, so that
     * vblank comes within one flip */
    if (sixty && sixty_asked) {
        release_flip();
    }
    for (i = 0; i < CALLBACKS; i++) {
        if (DEVICE.callback_pending & RUN(i)) {
            DEVICE.callback_pending &= ~RUN(i);
            kmiExCallBackFunc(DEVICE.callback[i].function,
                              DEVICE.callback[i].argument);
        }
    }
    return 0;
}

/* install the handler, and clear the state Kamui's clears */
void kmiInitInterrupt(void)
{
    KMDWORD i;

    kmiRegisterPhIntHandler(handle_interrupt);
    DEVICE.unk_1ec = 0;
    DEVICE.callback_pending = 0;
    DEVICE.texture_render = 0;
    DEVICE.slots_held = 0;
    STATS.render_wait = 0;
    STATS.ta_wait = 0;
    STATS.render_timeouts = 0;
    STATS.ta_timeouts = 0;
    STATS.ol_overflow_wait = 0;
    STATS.isp_overflow_wait = 0;
    STATS.hold_wait = 0;
    STATS.ol_overflows = 0;
    STATS.isp_overflows = 0;
    slot_busy = 0;
    for (i = 0; i < LISTS; i++) {
        STATS.list_max[i] = 0;
    }
}
