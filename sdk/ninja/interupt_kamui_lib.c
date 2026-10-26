/* Kamui's interrupt handler. Kamui writes each frame's lists into one of four
 * slots. the slot's frame then passes from the tile accelerator to the
 * renderer, which draws it into a frame buffer, and on to the display. slot i
 * uses bank i & 1: ta_bank[i & 1], and frame buffer i & 1 but under
 * FLICKER_FREE. there each field has a frame buffer of its own. the handler
 * marks a slot's stage done when the chip signals its end. it moves the
 * waiting slots on at each vertical blank that count_down and time_out_slots
 * let through */
#include <kamui.h>

/* the system bus's interrupt status: INT_STATUS for normal interrupts and
 * INT_STATUS_ERROR for errors, each followed by its bits. a set bit marks an
 * interrupt pending, and writing 1 to the bit clears it */
#define INT_STATUS (*(volatile KMDWORD *)0xa05f6900)
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
#define INT_STATUS_ERROR (*(volatile KMDWORD *)0xa05f6908)
#define INT_STRIP_OVERRUN 0x02
#define INT_ISP_OVERFLOW 0x04
#define INT_OL_OVERFLOW 0x08

/* the video chip's registers */
#define SOFT_RESET (*(volatile KMDWORD *)0xa05f8008)
#define START_RENDER (*(volatile KMDWORD *)0xa05f8014)
#define PARAM_BASE (*(volatile KMDWORD *)0xa05f8020)
#define REGION_BASE (*(volatile KMDWORD *)0xa05f802c)
#define FB_READ_CONTROL (*(volatile KMDWORD *)0xa05f8044)
#define FB_WRITE_CONTROL (*(volatile KMDWORD *)0xa05f8048)
#define FB_WRITE_STRIDE (*(volatile KMDWORD *)0xa05f804c)
#define FB_READ_START1 (*(volatile KMDWORD *)0xa05f8050)
#define FB_READ_START2 (*(volatile KMDWORD *)0xa05f8054)
#define FB_WRITE_START (*(volatile KMDWORD *)0xa05f8060)
#define FB_CLIP_X (*(volatile KMDWORD *)0xa05f8068)
#define FB_CLIP_Y (*(volatile KMDWORD *)0xa05f806c)
#define BACKGROUND_DEPTH (*(volatile KMDWORD *)0xa05f8088)
#define BACKGROUND_TAG (*(volatile KMDWORD *)0xa05f808c)
#define VIDEO_CONTROL (*(volatile KMDWORD *)0xa05f80e8)
#define SCALER_CONTROL (*(volatile KMDWORD *)0xa05f80f4)
#define SCALER_INTERLACE 0x20000
#define SCALER_FIELD 0x40000
#define SYNC_STATUS (*(volatile KMDWORD *)0xa05f810c)
#define SYNC_FIELD 0x400
#define SYNC_FIELD_SHIFT 10
#define TA_OL_BASE (*(volatile KMDWORD *)0xa05f8124)
#define TA_ISP_BASE (*(volatile KMDWORD *)0xa05f8128)
#define TA_OL_LIMIT (*(volatile KMDWORD *)0xa05f812c)
#define TA_ISP_LIMIT (*(volatile KMDWORD *)0xa05f8130)
#define TA_ISP_CURRENT (*(volatile KMDWORD *)0xa05f8138)
#define TA_LIST_INIT (*(volatile KMDWORD *)0xa05f8144)
#define TA_NEXT_OPB_INIT (*(volatile KMDWORD *)0xa05f8164)

/* SOFT_RESET's bits */
#define RESET_TA 1
#define RESET_RENDERER 2
#define RESET_SDRAM 4

/* FB_READ_CONTROL's bit that turns the display's frame buffer reads on */
#define FB_READ_ON 1

/* the CPU's timer 0 count, which counts down */
#define TCNT0 (*(volatile KMDWORD *)0xffd8000c)
/* DMA channel 2's control register, and its bits that enable the channel and
 * flag the end of its transfer */
#define CHCR2 (*(volatile KMDWORD *)0xffa0002c)
#define CHCR_ENABLE 0x1
#define CHCR_TRANSFER_END 0x2

/* texture memory, through the uncached area */
#define TEXTURE_MEMORY 0xa5000000UL
/* the tile accelerator's polygon input, as a DMA target */
#define TA_INPUT 0x10000000UL
/* the size in bytes of the end-of-list parameter that follows each list */
#define END_OF_LIST 32

/* a list type in bits 24 to 27 of a parameter control word, and the mask of
 * those bits */
#define LIST_TYPE(type) ((KMDWORD)(type) << 24)
#define LIST_TYPE_MASK 0x0f000000

/* lists_sent's and lists_done's bit for a ListType */
#define LIST_BIT(type) (1 << (type))

/* the order kmiCreateRegionArray takes for ta_options: BY_COLUMNS when it
 * holds OPTION_BY_COLUMNS, and else BY_ROWS. the complement gives that only
 * because BY_COLUMNS is 0, and BY_ROWS and OPTION_BY_COLUMNS are both 1 */
#define REGION_ORDER (~kmiDeviceExtension.ta_options & OPTION_BY_COLUMNS)

/* callback_pending's bits, by the callback each runs */
#define RUN_END_RENDER (1 << END_RENDER_CALLBACK)
#define RUN_VSYNC (1 << VSYNC_CALLBACK)
#define RUN_HSYNC (1 << HSYNC_CALLBACK)
#define RUN_STRIP_OVERRUN (1 << STRIP_OVERRUN_CALLBACK)
#define RUN_END_VERTEX (1 << END_VERTEX_CALLBACK)
#define RUN_END_YUV (1 << END_YUV_CALLBACK)

#define SLOTS 4
#define LISTS 5 /* a slot's lists, one for each ListType */

/* @hack type: list_size's row converted to the pointer it decays to. SHC then
 * reads a list's size in a slot with one indexed load (MOV.L @(R0,R14),R6 at
 * 0x8c66551a). as list_size[type][slot], SHC forms the address first,
 * start_ta_transfer grows by 4 bytes, and 180 of its 341 words differ */
#define LIST_SIZE(type, slot)                                                  \
    (((volatile KMDWORD *)kmiDeviceExtension.list_size[type])[slot])
/* @hack type: list_start's row converted to the pointer it decays to. SHC then
 * loads the translucent modifier and punch-through lists' starts before it adds
 * END_OF_LIST to their sizes (0x8c6656aa, 0x8c6656d6). as
 * list_start.row[type][slot], the adds come first, and 4 of
 * start_ta_transfer's 341 words differ */
#define LIST_START(type, slot)                                                 \
    (((PKMDWORD volatile *)kmiDeviceExtension.list_start.row[type])[slot])

/* how many of time_out_slots' checks may find a render into a texture
 * unfinished before it restores the frame buffer's write registers */
#define TEXTURE_TIMEOUT 16

/* the words of background_packet that kmiFinalizeRender copies */
#define PACKET_WORDS 48

/* BACKGROUND_TAG's fields: background_skip's bits from bit 24, and from bit 3
 * the packet's offset in the ISP parameters, in words. TAG_OFFSET takes the
 * offset in bytes */
#define TAG_SKIP 0xff000000
#define TAG_OFFSET(bytes) ((bytes) << 1 & 0x00fffff8)

/* grow's areas, the bytes it moves an area's limit by, and its bounds:
 * OL_FLOOR on ol_limit above isp_base, and OL_OVERFLOW_LIMIT on
 * ol_overflow_wait */
#define ISP_PARAMS 0
#define OBJECT_LISTS 1
#define ISP_STEP 0x2000
#define OL_STEP 0x800
#define OL_FLOOR 0x140
#define OL_OVERFLOW_LIMIT 10
/* the bytes grow keeps between a bank's isp_limit and its ol_limit. on a chip
 * without KMI_CAP_ISP_CURRENT, kmiFinalizeRender copies the background packet
 * to isp_limit, into that gap */
#define LIMIT_GAP (PACKET_WORDS * 4)

/* set when a slot under way makes time_out_slots hold the slots. the next
 * blank count_down counts then goes through to time_out_slots, even with
 * vsync_remaining still 0 or more. that blank clears slot_busy, unless it takes
 * the count below 0 */
static KMDWORD slot_busy;
struct kmi_interrupt_statistics kmiIntStatisticData;

/* how many renders time_out_slots has stopped for running past render_timeout
 * since a render last ended. nothing reads the count */
static KMDWORD renders_stopped;
/* 1 from a render's start until the first strip overrun after it. strip_overrun
 * clears render_started on that first overrun and queues no callback. each
 * later overrun, with render_started 0, queues the strip overrun callback */
static KMINT32 render_started;
/* the field the last vertical blank began, 0 or 1. vblank_in reads it from
 * SYNC_STATUS on a chip with KMI_CAP_SYNC_STATUS, and otherwise flips
 * field_toggle and copies it */
static KMDWORD field;
static KMDWORD field_toggle = 1;

void kmiInitInterrupt(void)
{
    KMDWORD i;

    kmiRegisterPhIntHandler(kmiInterruptHandlerSH);
    kmiDeviceExtension.unk_1ec = 0;
    kmiDeviceExtension.callback_pending = 0;
    kmiDeviceExtension.texture_render = 0;
    kmiDeviceExtension.slots_held = 0;
    kmiIntStatisticData.render_wait = 0;
    kmiIntStatisticData.ta_wait = 0;
    kmiIntStatisticData.render_timeouts = 0;
    kmiIntStatisticData.ta_timeouts = 0;
    kmiIntStatisticData.ol_overflow_wait = 0;
    kmiIntStatisticData.isp_overflow_wait = 0;
    kmiIntStatisticData.hold_wait = 0;
    kmiIntStatisticData.ol_overflows = 0;
    kmiIntStatisticData.isp_overflows = 0;
    renders_stopped = 0;
    slot_busy = 0;
    for (i = 0; i < LISTS; i++) {
        kmiIntStatisticData.list_max[i] = 0;
    }
}

/* show the first slot that is SLOT_RENDERED or SLOT_OVERFLOW, and mark it
 * SLOT_SHOWN. under FLICKER_FREE the display reads field 0 from frame buffer 0
 * and field 1 from frame buffer 1. in any other mode both fields read the frame
 * buffer of the slot's bank. while strip_buffer is 1, the read starts stay as
 * they are instead. while display_changed is 1, showing a slot also writes
 * video_control and fb_read_control to the chip and clears display_changed */
static void show_rendered_frame(void)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (kmiDeviceExtension.slot_state[i] == SLOT_RENDERED ||
            kmiDeviceExtension.slot_state[i] == SLOT_OVERFLOW) {
            kmiDeviceExtension.slot_state[i] = SLOT_SHOWN;
            if (FLICKER_FREE) {
                FB_READ_START1 = kmiDeviceExtension.fb_start[0] +
                                 kmiDeviceExtension.field_offset[0];
                FB_READ_START2 = kmiDeviceExtension.fb_start[1] +
                                 kmiDeviceExtension.field_offset[1];
            } else if (kmiDeviceExtension.strip_buffer != 1) {
                FB_READ_START1 = kmiDeviceExtension.fb_start[i & 1] +
                                 kmiDeviceExtension.field_offset[0];
                FB_READ_START2 = kmiDeviceExtension.fb_start[i & 1] +
                                 kmiDeviceExtension.field_offset[1];
            }
            /* @bug logic: the state is read back after the SLOT_SHOWN store
             * (0x8c66536c, 0x8c6653c6), so it is never SLOT_OVERFLOW here. an
             * overflowed slot makes the pending display change too */
            if (kmiDeviceExtension.slot_state[i] != SLOT_OVERFLOW &&
                kmiDeviceExtension.display_changed == 1) {
                VIDEO_CONTROL = kmiDeviceExtension.video_control;
                FB_READ_CONTROL = kmiDeviceExtension.fb_read_control;
                kmiDeviceExtension.display_changed = 0;
            }
            break;
        }
    }
}

/* if the tile accelerator is idle, send it each nonempty list of the first
 * SLOT_READY slot by DMA, with the list's end-of-list parameter. it first
 * resets the tile accelerator and sets it up for the slot's bank. while
 * common_modifier is 1, the common modifier data goes ahead of each modifier
 * list. returns 1, or 0 for a request that finds the DMA queue full. a 0
 * leaves the slot SLOT_IN_TA and ta_busy set, with its remaining lists
 * unsent */
static int start_ta_transfer(void)
{
    int i;
    int bank;
    KMSTATUS result;
    KMDWORD *handle;

    for (i = 0; i < SLOTS; i++) {
        if (kmiDeviceExtension.slot_state[i] == SLOT_READY &&
            kmiDeviceExtension.ta_busy == 0) {
            kmiDeviceExtension.ta_busy = 1;
            kmiDeviceExtension.slot_state[i] = SLOT_IN_TA;
            kmiDeviceExtension.ta_slot = i;
            bank = i & 1;
            /* @hack type: TCNT0 converted to unsigned int, which loads
             * ta_start's address and SOFT_RESET's ahead of the timer's read
             * (0x8c6654b6, 0x8c6654b8); without it, 4 of 341 words differ */
            kmiIntStatisticData.ta_start = (unsigned int)TCNT0;
            SOFT_RESET = RESET_TA;
            SOFT_RESET = 0;
            TA_OL_BASE = kmiDeviceExtension.ta_bank[bank].ol_base;
            TA_OL_LIMIT = kmiDeviceExtension.ta_bank[bank].ol_limit;
            TA_ISP_BASE = kmiDeviceExtension.ta_bank[bank].isp_base;
            TA_ISP_LIMIT = kmiDeviceExtension.ta_bank[bank].isp_limit;
            TA_NEXT_OPB_INIT = kmiDeviceExtension.ta_bank[bank].next_opb_init;
            TA_LIST_INIT = 0x80000000;
            (void)TA_LIST_INIT;
            kmiDeviceExtension.lists_sent = 0;
            kmiDeviceExtension.lists_done = 0;
            kmiIntStatisticData.ta_wait = 0;
            if (LIST_SIZE(KM_OPAQUE_POLYGON, i)) {
                kmiDeviceExtension.lists_sent |= LIST_BIT(KM_OPAQUE_POLYGON);
                result = kmiDMAtoTARequest(
                    TA_INPUT, LIST_START(KM_OPAQUE_POLYGON, i),
                    LIST_SIZE(KM_OPAQUE_POLYGON, i) + END_OF_LIST,
                    DMA_SOURCE_UP, &handle);
                if (result == DMA_QUEUE_FULL) {
                    return 0;
                }
            }
            if (LIST_SIZE(KM_OPAQUE_MODIFIER, i)) {
                kmiDeviceExtension.lists_sent |= LIST_BIT(KM_OPAQUE_MODIFIER);
                if (kmiDeviceExtension.common_modifier == 1) {
                    *kmiDeviceExtension.common_modifier_data =
                        (*kmiDeviceExtension.common_modifier_data &
                         ~LIST_TYPE_MASK) |
                        LIST_TYPE(KM_OPAQUE_MODIFIER);
                    result = kmiDMAtoTARequest(
                        TA_INPUT, kmiDeviceExtension.common_modifier_data,
                        kmiDeviceExtension.common_modifier_size, DMA_SOURCE_UP,
                        &handle);
                    if (result == DMA_QUEUE_FULL) {
                        return 0;
                    }
                }
                result = kmiDMAtoTARequest(
                    TA_INPUT, LIST_START(KM_OPAQUE_MODIFIER, i),
                    LIST_SIZE(KM_OPAQUE_MODIFIER, i) + END_OF_LIST,
                    DMA_SOURCE_UP, &handle);
                if (result == DMA_QUEUE_FULL) {
                    return 0;
                }
            }
            if (LIST_SIZE(KM_TRANS_POLYGON, i)) {
                kmiDeviceExtension.lists_sent |= LIST_BIT(KM_TRANS_POLYGON);
                result = kmiDMAtoTARequest(
                    TA_INPUT, LIST_START(KM_TRANS_POLYGON, i),
                    LIST_SIZE(KM_TRANS_POLYGON, i) + END_OF_LIST, DMA_SOURCE_UP,
                    &handle);
                if (result == DMA_QUEUE_FULL) {
                    return 0;
                }
            }
            if (LIST_SIZE(KM_TRANS_MODIFIER, i)) {
                kmiDeviceExtension.lists_sent |= LIST_BIT(KM_TRANS_MODIFIER);
                if (kmiDeviceExtension.common_modifier == 1) {
                    *kmiDeviceExtension.common_modifier_data =
                        (*kmiDeviceExtension.common_modifier_data &
                         ~LIST_TYPE_MASK) |
                        LIST_TYPE(KM_TRANS_MODIFIER);
                    result = kmiDMAtoTARequest(
                        TA_INPUT, kmiDeviceExtension.common_modifier_data,
                        kmiDeviceExtension.common_modifier_size, DMA_SOURCE_UP,
                        &handle);
                    if (result == DMA_QUEUE_FULL) {
                        return 0;
                    }
                }
                result = kmiDMAtoTARequest(
                    TA_INPUT, LIST_START(KM_TRANS_MODIFIER, i),
                    LIST_SIZE(KM_TRANS_MODIFIER, i) + END_OF_LIST,
                    DMA_SOURCE_UP, &handle);
                if (result == DMA_QUEUE_FULL) {
                    return 0;
                }
            }
            if (LIST_SIZE(KM_PUNCHTHROUGH_POLYGON, i)) {
                kmiDeviceExtension.lists_sent |=
                    LIST_BIT(KM_PUNCHTHROUGH_POLYGON);
                result = kmiDMAtoTARequest(
                    TA_INPUT, LIST_START(KM_PUNCHTHROUGH_POLYGON, i),
                    LIST_SIZE(KM_PUNCHTHROUGH_POLYGON, i) + END_OF_LIST,
                    DMA_SOURCE_UP, &handle);
                if (result == DMA_QUEUE_FULL) {
                    return 0;
                }
            }
            return 1;
        }
    }
    return 1;
}

/* if the renderer is idle, render the first SLOT_TA_DONE slot. when
 * regions_stale holds the bit of the slot's bank, the bank's region array is
 * rebuilt first and the bit cleared. the render's target is the bank's frame
 * buffer, or under FLICKER_FREE the current field's. strip_buffer 1 leaves
 * FB_WRITE_START as it is */
static void start_render(void)
{
    int i;
    int bank;

    for (i = 0; i < SLOTS; i++) {
        if (kmiDeviceExtension.slot_state[i] == SLOT_TA_DONE &&
            kmiDeviceExtension.render_busy == 0) {
            bank = i & 1;
            if (kmiDeviceExtension.regions_stale & (bank + 1)) {
                kmiCreateRegionArray(kmiDeviceExtension.tiles_x,
                                     kmiDeviceExtension.tiles_y, 0, 1, bank,
                                     REGION_ORDER);
                kmiDeviceExtension.regions_stale =
                    kmiDeviceExtension.regions_stale & ~(bank + 1) & 3;
            }
            kmiDeviceExtension.render_busy = 1;
            kmiDeviceExtension.slot_state[i] = SLOT_RENDERING;
            if (kmiDeviceExtension.capabilities & KMI_CAP_RESET_BEFORE_RENDER) {
                SOFT_RESET = RESET_RENDERER;
                SOFT_RESET = 0;
            }
            REGION_BASE = kmiDeviceExtension.ta_bank[bank].region_base;
            PARAM_BASE = kmiDeviceExtension.ta_bank[bank].isp_base;
            if (kmiDeviceExtension.strip_buffer != 1) {
                if (FLICKER_FREE) {
                    FB_WRITE_START = kmiDeviceExtension.fb_start[field];
                } else {
                    FB_WRITE_START = kmiDeviceExtension.fb_start[bank];
                }
            }
            BACKGROUND_TAG = kmiDeviceExtension.background_tag;
            BACKGROUND_DEPTH = kmiDeviceExtension.background_depth & ~15;
            FB_CLIP_X = kmiDeviceExtension.fb_clip_x;
            FB_CLIP_Y = kmiDeviceExtension.fb_clip_y;
            FB_WRITE_STRIDE = kmiDeviceExtension.fb_write_stride;
            FB_WRITE_CONTROL = kmiDeviceExtension.fb_write_control;
            kmiIntStatisticData.render_wait = 0;
            if ((kmiDeviceExtension.display_mode &
                 ~(KM_PAL | KM_DSPMODE_WIDTH_640)) == KM_DSPMODE_HEIGHT_480) {
                if (field == 0) {
                    SCALER_CONTROL =
                        kmiDeviceExtension.scaler_control | SCALER_INTERLACE;
                } else {
                    SCALER_CONTROL = kmiDeviceExtension.scaler_control |
                                     SCALER_INTERLACE | SCALER_FIELD;
                }
            }
            /* @hack type: TCNT0 converted to unsigned int, which loads
             * render_start's address ahead of the timer's read (0x8c6658ec);
             * without it, 2 of 315 words differ */
            kmiIntStatisticData.render_start = (unsigned int)TCNT0;
            kmiDeviceExtension.render_slot = i;
            render_started = 1;
            START_RENDER = 0xffffffff;
            break;
        }
    }
}

#pragma inline(count_down, time_out_slots)

/* count a vertical blank off vsync_remaining. returns 0 to let the blank
 * through to time_out_slots, and 1 to hold the slots. when the count falls
 * below 0, count_down sets it to vsync_wait again and returns 0. while
 * slot_busy is set, count_down returns 0 with the count at 0 or more too, and
 * clears slot_busy. in an interlaced mode, count_down returns 1 for a blank
 * that begins field 1, and doesn't count it */
static int count_down(void)
{
    int wait;

    if ((kmiDeviceExtension.display_mode & KM_DSPMODE_INTERLACE) ==
        KM_DSPMODE_INTERLACE) {
        /* @hack spelling: the test of field nested inside the test of the mode.
         * SHC then reads field through R5, which still holds field's address
         * from vblank_in's store (MOV.L R0,@R5 at 0x8c6659d2, MOV.L @R5,R0 at
         * 0x8c6659de). as one test with &&, SHC loads the address again, and 30
         * of vblank_in's 431 words differ */
        if (field == 1) {
            /* @hack spelling: 1 returned through wait. SHC then gives this
             * return its own copies of R12's 1, in R5 and R4 (0x8c6659e4,
             * 0x8c6659e8). returned directly, 181 of vblank_in's 431 words
             * differ, and 32 of kmiFinalizeRender's 145 */
            wait = 1;
            return wait;
        }
    }
    kmiDeviceExtension.vsync_remaining--;
    if (kmiDeviceExtension.vsync_remaining >= 0) {
        if (slot_busy != 0) {
            wait = 0;
            slot_busy = 0;
        } else {
            wait = 1;
        }
    } else {
        kmiDeviceExtension.vsync_remaining = kmiDeviceExtension.vsync_wait;
        wait = 0;
    }
    return wait;
}

/* the timeout check, at each vertical blank that count_down lets through.
 * returns 1 to hold the slots, and 0 to let them move.
 *
 * while slots_held is set, it returns 1 and checks nothing. while a frame
 * renders into a texture, it returns 1 and counts the check in hold_wait. a
 * count past TEXTURE_TIMEOUT restores the frame buffer's write registers,
 * clears render_busy and zeroes hold_wait.
 *
 * otherwise it checks each slot whose state is even, a stage under way. a slot
 * in state 0 or SLOT_IN_TA holds the slots and sets slot_busy. so does a slot
 * in SLOT_RENDERING while render_timeout is nonzero. such a slot holds them in
 * the check that stops its work too. render_busy still set after the checks
 * holds them as well.
 *
 * with render_timeout 0, a render is stopped at the first check that finds it
 * running. otherwise it is stopped once more than render_timeout checks have
 * found it running. that stop also resets the SDRAM interface, with the
 * display's frame buffer reads off, and rebuilds both banks' region arrays.
 * either way the renderer is reset, and the slot becomes SLOT_RENDERED. a
 * transfer to the tile accelerator is stopped once more than vertex_timeout
 * checks have found it running. kmiDMAHazardReset stops its DMA, and the slot
 * becomes SLOT_FREE */
static int time_out_slots(void)
{
    int i;
    int busy;

    busy = 0;
    if (kmiDeviceExtension.slots_held != 0) {
        return 1;
    }
    if (kmiDeviceExtension.texture_render != 0) {
        kmiIntStatisticData.hold_wait++;
        if (kmiIntStatisticData.hold_wait > TEXTURE_TIMEOUT) {
            FB_WRITE_CONTROL = kmiDeviceExtension.fb_write_control;
            FB_WRITE_STRIDE = kmiDeviceExtension.fb_write_stride;
            FB_CLIP_X = kmiDeviceExtension.fb_clip_x;
            FB_CLIP_Y = kmiDeviceExtension.fb_clip_y;
            kmiDeviceExtension.render_busy = 0;
            kmiIntStatisticData.hold_wait = 0;
        }
        /* @hack spelling: 1 returned through busy, which keeps this return
         * apart from the return for slots_held. that one sets R4 in its BRA's
         * delay slot (BRA and MOV R12,R4 at 0x8c665a82, 0x8c665a84). returned
         * directly, the two returns merge: 168 of vblank_in's 431 words differ,
         * and 32 of kmiFinalizeRender's 145 */
        busy = 1;
        return busy;
    }
    for (i = 0; i < SLOTS; i++) {
        /* @hack redundant: the else after the continue, which reads the slot's
         * state again into R3 (0x8c665aea); without it, the state takes R2 and
         * 33 of vblank_in's 431 words differ */
        if (kmiDeviceExtension.slot_state[i] < 0) {
            continue;
        } else if (!(kmiDeviceExtension.slot_state[i] & 1)) {
            if (kmiDeviceExtension.render_timeout == 0) {
                if (kmiDeviceExtension.slot_state[i] == SLOT_RENDERING) {
                    kmiDeviceExtension.slot_state[i] = SLOT_RENDERED;
                    if (kmiDeviceExtension.render_busy != 0) {
                        kmiIntStatisticData.render_timeouts++;
                    }
                    kmiDeviceExtension.render_busy = 0;
                    SOFT_RESET = RESET_RENDERER;
                    SOFT_RESET = 0;
                } else {
                    slot_busy = 1;
                    busy = 1;
                    if (kmiDeviceExtension.slot_state[i] == SLOT_IN_TA) {
                        kmiIntStatisticData.ta_wait++;
                        /* @hack type: vertex_timeout converted to the type it
                         * has, which reads ta_wait back before it (MOV.L
                         * @(4,R13),R0 at 0x8c665b3a); without it,
                         * vertex_timeout is read first and 2 of vblank_in's
                         * 431 words differ */
                        if (kmiIntStatisticData.ta_wait >
                            (KMDWORD)kmiDeviceExtension.vertex_timeout) {
                            kmiIntStatisticData.ta_timeouts++;
                            kmiDMAHazardReset();
                            kmiDeviceExtension.slot_state[i] = SLOT_FREE;
                            kmiDeviceExtension.ta_busy = 0;
                            kmiIntStatisticData.ta_wait = 0;
                        }
                    }
                }
            } else {
                slot_busy = 1;
                busy = 1;
                if (kmiDeviceExtension.slot_state[i] == SLOT_RENDERING) {
                    kmiIntStatisticData.render_wait++;
                    /* @hack type: render_timeout converted to the type it
                     * has, which reads render_wait back before it (MOV.L
                     * @R13,R0 at 0x8c665bd4); without it, render_timeout is
                     * read first and 2 of vblank_in's 431 words differ */
                    if (kmiIntStatisticData.render_wait >
                        (KMDWORD)kmiDeviceExtension.render_timeout) {
                        kmiIntStatisticData.render_timeouts++;
                        FB_READ_CONTROL =
                            kmiDeviceExtension.fb_read_control & ~FB_READ_ON;
                        SOFT_RESET = RESET_RENDERER | RESET_SDRAM;
                        SOFT_RESET = 0;
                        FB_READ_CONTROL = kmiDeviceExtension.fb_read_control;
                        kmiCreateRegionArray(kmiDeviceExtension.tiles_x,
                                             kmiDeviceExtension.tiles_y, 0, 1,
                                             0, REGION_ORDER);
                        kmiCreateRegionArray(kmiDeviceExtension.tiles_x,
                                             kmiDeviceExtension.tiles_y, 0, 1,
                                             1, REGION_ORDER);
                        kmiDeviceExtension.slot_state[i] = SLOT_RENDERED;
                        kmiDeviceExtension.render_busy = 0;
                        renders_stopped++;
                        kmiIntStatisticData.render_wait = 0;
                    }
                }
                if (kmiDeviceExtension.slot_state[i] == SLOT_IN_TA) {
                    kmiIntStatisticData.ta_wait++;
                    /* @hack type: vertex_timeout converted to the type it has.
                     * SHC then loads its address before it reads ta_wait, and
                     * copies ta_wait to R1 before storing it (0x8c665c54, MOV
                     * R3,R1 at 0x8c665c5a). without it, 5 of vblank_in's 431
                     * words differ */
                    if (kmiIntStatisticData.ta_wait >
                        (KMDWORD)kmiDeviceExtension.vertex_timeout) {
                        kmiIntStatisticData.ta_timeouts++;
                        kmiDMAHazardReset();
                        kmiDeviceExtension.slot_state[i] = SLOT_FREE;
                        kmiDeviceExtension.ta_busy = 0;
                        kmiIntStatisticData.ta_wait = 0;
                    }
                }
            }
        }
    }
    busy |= kmiDeviceExtension.render_busy;
    return busy;
}

/* handle the start of a vertical blank. when count_down and time_out_slots let
 * the slots move, call show_rendered_frame, start_ta_transfer and start_render,
 * and clear flip_pending. returns 0, or KMI_FATAL_DMA_QUEUE_FULL when
 * start_ta_transfer finds the DMA queue full, which kmiInterruptHandlerSH
 * passes to kmiFatalErrorEntry. that result skips start_render and leaves
 * flip_pending as it was */
static int vblank_in(void)
{
    INT_STATUS = INT_VBLANK_IN;
    kmiDeviceExtension.callback_pending |= RUN_VSYNC;
    kmiDeviceExtension.vsync_count++;
    if (kmiDeviceExtension.capabilities & KMI_CAP_SYNC_STATUS) {
        field = (SYNC_STATUS & SYNC_FIELD) >> SYNC_FIELD_SHIFT;
    } else {
        field_toggle = (field_toggle + 1) & 1;
        field = field_toggle;
    }
    if (count_down() == 0) {
        if (time_out_slots() == 0) {
            show_rendered_frame();
            if (start_ta_transfer() == 0) {
                return KMI_FATAL_DMA_QUEUE_FULL;
            }
            start_render();
            kmiDeviceExtension.flip_pending = 0;
        }
    }
    return 0;
}

/* finish ta_slot's transfer to the tile accelerator. the handler calls it once
 * lists_done matches lists_sent. it marks the tile accelerator idle and times
 * the transfer. a slot that isn't SLOT_FREE gets the background plane. its
 * packet is copied where the bank's ISP parameters end, and background_tag
 * points at the copy. they end at TA_ISP_CURRENT on a chip with
 * KMI_CAP_ISP_CURRENT, and otherwise at the bank's isp_limit. such a slot also
 * leaves TA_ISP_CURRENT in the bank's entry of kmiIntStatisticData.isp_current,
 * which grow reads. outside a render into a texture, the slot becomes
 * SLOT_TA_DONE, with the end of vertex callback pending */
void kmiFinalizeRender(void)
{
    /* @hack type: destination holds the end of the ISP parameters as an
     * offset, then as the uncached address the packet is copied to. SHC
     * converts it in place (ADD R2,R4 at 0x8c665dd0). with the end in a
     * variable of its own, and a separate destination pointer assigned after
     * source, 28 of 145 words differ */
    KMDWORD *destination;
    volatile KMDWORD *source;
    int n;

    if (kmiDeviceExtension.slot_state[kmiDeviceExtension.ta_slot] !=
        SLOT_FREE) {
        if (kmiDeviceExtension.texture_render == 0) {
            kmiDeviceExtension.callback_pending |= RUN_END_VERTEX;
            kmiDeviceExtension.slot_state[kmiDeviceExtension.ta_slot] =
                SLOT_TA_DONE;
        }
        if (kmiDeviceExtension.capabilities & KMI_CAP_ISP_CURRENT) {
            destination = (KMDWORD *)TA_ISP_CURRENT;
        } else {
            destination = (KMDWORD *)kmiDeviceExtension
                              .ta_bank[kmiDeviceExtension.ta_slot & 1]
                              .isp_limit;
        }
        kmiIntStatisticData.isp_current[kmiDeviceExtension.ta_slot & 1] =
            TA_ISP_CURRENT;
        kmiDeviceExtension.background_tag =
            (kmiDeviceExtension.background_skip & TAG_SKIP) |
            TAG_OFFSET(
                (KMDWORD)destination -
                kmiDeviceExtension.ta_bank[kmiDeviceExtension.ta_slot & 1]
                    .isp_base);
        source = kmiDeviceExtension.background_packet;
        destination = (KMDWORD *)((KMDWORD)destination + TEXTURE_MEMORY);
        for (n = 0; n < PACKET_WORDS; n++) {
            /* @hack type: each packet word converted to the type it has. SHC
             * copies three words a pass, and then takes 3 from the count and
             * tests it before the pass's first store (ADD #-3,R5 and TST R5,R5
             * at 0x8c665dd4, 0x8c665dd6). without it, the two move beside the
             * pass's last store, and 9 of 145 words differ */
            *destination++ = (KMDWORD)*source++;
        }
    }
    kmiDeviceExtension.ta_busy = 0;
    kmiIntStatisticData.ta_time = kmiIntStatisticData.ta_start - TCNT0;
}

/* give area more room in each bank, bank 0 first. a bank's isp_limit and
 * ol_limit move together, LIMIT_GAP bytes apart. grow's result, always 0, is
 * never read.
 *
 * a bank has room for ISP_PARAMS while the moved ol_limit stays below ol_base.
 * grow moves such a bank's limits up ISP_STEP bytes and zeroes
 * isp_overflow_wait. it calls kmiFatalErrorEntry(KMI_FATAL_NO_ROOM) for a bank
 * without that room while isp_overflow_wait is above 0. isp_overflow adds 1 to
 * the count before each call, so only bank 0's room spares bank 1 the error.
 *
 * a bank has room for OBJECT_LISTS while the moved ol_limit stays more than
 * OL_FLOOR bytes above isp_base. grow zeroes ol_overflow_wait for such a bank.
 * it moves the bank's limits down OL_STEP bytes unless
 * kmiIntStatisticData.isp_current[bank] has reached the moved ol_limit. it
 * calls kmiFatalErrorEntry(KMI_FATAL_NO_ROOM) for a bank without that room
 * while ol_overflow_wait is above OL_OVERFLOW_LIMIT */
static int grow(int area)
{
    int bank;

    switch (area) {
    case ISP_PARAMS:
        for (bank = 0; bank < 2; bank++) {
            if (kmiDeviceExtension.ta_bank[bank].isp_limit + ISP_STEP <
                kmiDeviceExtension.ta_bank[bank].ol_base - LIMIT_GAP) {
                kmiDeviceExtension.ta_bank[bank].isp_limit += ISP_STEP;
                kmiDeviceExtension.ta_bank[bank].ol_limit =
                    kmiDeviceExtension.ta_bank[bank].isp_limit + LIMIT_GAP;
                kmiIntStatisticData.isp_overflow_wait = 0;
            } else if (kmiIntStatisticData.isp_overflow_wait > 0) {
                kmiFatalErrorEntry(KMI_FATAL_NO_ROOM);
            }
        }
        break;
    case OBJECT_LISTS:
        for (bank = 0; bank < 2; bank++) {
            if (kmiDeviceExtension.ta_bank[bank].ol_limit - OL_STEP >
                kmiDeviceExtension.ta_bank[bank].isp_base + OL_FLOOR) {
                if (kmiIntStatisticData.isp_current[bank] <
                    kmiDeviceExtension.ta_bank[bank].ol_limit - OL_STEP) {
                    kmiDeviceExtension.ta_bank[bank].ol_limit -= OL_STEP;
                    kmiDeviceExtension.ta_bank[bank].isp_limit =
                        kmiDeviceExtension.ta_bank[bank].ol_limit - LIMIT_GAP;
                }
                kmiIntStatisticData.ol_overflow_wait = 0;
            } else if (kmiIntStatisticData.ol_overflow_wait >
                       OL_OVERFLOW_LIMIT) {
                kmiFatalErrorEntry(KMI_FATAL_NO_ROOM);
            }
        }
        break;
    }
    return 0;
}

static int isp_overflow(void)
{
    INT_STATUS_ERROR = INT_ISP_OVERFLOW;
    kmiIntStatisticData.isp_overflows++;
    kmiDeviceExtension.slot_state[kmiDeviceExtension.ta_slot] = SLOT_OVERFLOW;
    if (kmiDeviceExtension.opb_downward == 1) {
        kmiIntStatisticData.isp_overflow_wait++;
        grow(ISP_PARAMS);
    }
    return 0;
}

static int ol_overflow(void)
{
    INT_STATUS_ERROR = INT_OL_OVERFLOW;
    kmiIntStatisticData.ol_overflows++;
    if (kmiDeviceExtension.opb_downward == 1) {
        kmiIntStatisticData.ol_overflow_wait++;
        grow(OBJECT_LISTS);
    }
    return 0;
}

static int strip_overrun(void)
{
    INT_STATUS_ERROR = INT_STRIP_OVERRUN;
    if (render_started == 0) {
        kmiDeviceExtension.callback_pending |= RUN_STRIP_OVERRUN;
    }
    render_started = 0;
    return 0;
}

KMINT32 kmiInterruptHandlerSH(void)
{
    KMDWORD status;
    int i;
    KMDWORD error;
    KMDWORD control;

    status = INT_STATUS;
    if (status & INT_VBLANK_IN) {
        /* @hack spelling: the call in a loop of one pass. with the three loops
         * below, it keeps kmiFatalErrorEntry's address in R14 and status in R12
         * (0x8c666008, 0x8c666004). without the loop, the two swap registers,
         * and 266 of 442 words differ */
        for (;;) {
            error = vblank_in();
            /* @hack spelling: the break before the call when error is 0, which
             * copies status to R2 for the next test (MOV R12,R2 at
             * 0x8c66601c). with if (error) around the call instead of the
             * break, 2 of 442 words differ */
            if (!error) {
                break;
            }
            kmiFatalErrorEntry(error);
            break;
        }
    }
    if (status & INT_HBLANK_IN) {
        INT_STATUS = INT_HBLANK_IN;
        kmiDeviceExtension.callback_pending |= RUN_HSYNC;
    }
    if (status & INT_RENDER_END_TSP) {
        INT_STATUS = INT_RENDER_END;
        kmiDeviceExtension.render_busy = 0;
        kmiIntStatisticData.render_wait = 0;
        /* @hack type: TCNT0 converted to unsigned int, which loads its
         * address ahead of kmiIntStatisticData's (0x8c666044); without it, 2
         * of 442 words differ */
        kmiIntStatisticData.render_time =
            kmiIntStatisticData.render_start - (unsigned int)TCNT0;
        if (kmiDeviceExtension.texture_render != 0) {
            kmiDeviceExtension.texture_render = 0;
            kmiIntStatisticData.hold_wait = 0;
            FB_WRITE_CONTROL = kmiDeviceExtension.fb_write_control;
            FB_WRITE_STRIDE = kmiDeviceExtension.fb_write_stride;
            FB_CLIP_X = kmiDeviceExtension.fb_clip_x;
            FB_CLIP_Y = kmiDeviceExtension.fb_clip_y;
        } else {
            kmiDeviceExtension.slot_state[kmiDeviceExtension.render_slot] =
                SLOT_RENDERED;
        }
        kmiDeviceExtension.callback_pending |= RUN_END_RENDER;
        renders_stopped = 0;
    }
    /* @hack redundant: an empty never-run if, which copies status to R3 for
     * the DMA test (MOV R12,R3 at 0x8c66611a); without it, the copy takes R2
     * and 2 of 442 words differ */
    if (0) {
    }
    if (status & INT_DMA2_END) {
        control = CHCR2 & ~(CHCR_ENABLE | CHCR_TRANSFER_END);
        CHCR2 = control;
        INT_STATUS = INT_DMA2_END;
        /* @hack redundant: a never-run call kmiFatalErrorEntry(control),
         * which leaves INT_DMA2_END in R5 and CHCR2's address in R6
         * (0x8c666118, 0x8c666120); without it and its else, the two swap
         * registers and 6 of 442 words differ */
        if (0) {
            kmiFatalErrorEntry(control);
        } else {
            /* @hack redundant: an empty else, which loads kmiExecuteDMAChain's
             * address into R2 and then copies status to R3 (0x8c66612a,
             * 0x8c666130); without it, the two take R3 and R2, and 4 of 442
             * words differ */
        }
        kmiExecuteDMAChain();
    }
    if (status & INT_YUV_END) {
        INT_STATUS = INT_YUV_END;
        kmiDeviceExtension.callback_pending |= RUN_END_YUV;
    }
    if (status & INT_OPAQUE_END) {
        INT_STATUS = INT_OPAQUE_END;
        kmiDeviceExtension.lists_done |= LIST_BIT(KM_OPAQUE_POLYGON);
        if (kmiDeviceExtension.lists_sent == kmiDeviceExtension.lists_done) {
            kmiFinalizeRender();
        }
    }
    if (status & INT_OPAQUE_MODIFIER_END) {
        INT_STATUS = INT_OPAQUE_MODIFIER_END;
        kmiDeviceExtension.lists_done |= LIST_BIT(KM_OPAQUE_MODIFIER);
        if (kmiDeviceExtension.lists_sent == kmiDeviceExtension.lists_done) {
            kmiFinalizeRender();
        }
    }
    if (status & INT_TRANS_END) {
        INT_STATUS = INT_TRANS_END;
        kmiDeviceExtension.lists_done |= LIST_BIT(KM_TRANS_POLYGON);
        if (kmiDeviceExtension.lists_sent == kmiDeviceExtension.lists_done) {
            kmiFinalizeRender();
        }
    }
    if (status & INT_TRANS_MODIFIER_END) {
        INT_STATUS = INT_TRANS_MODIFIER_END;
        kmiDeviceExtension.lists_done |= LIST_BIT(KM_TRANS_MODIFIER);
        if (kmiDeviceExtension.lists_sent == kmiDeviceExtension.lists_done) {
            kmiFinalizeRender();
        }
    }
    if (status & INT_PUNCHTHROUGH_END) {
        INT_STATUS = INT_PUNCHTHROUGH_END;
        kmiDeviceExtension.lists_done |= LIST_BIT(KM_PUNCHTHROUGH_POLYGON);
        if (kmiDeviceExtension.lists_sent == kmiDeviceExtension.lists_done) {
            kmiFinalizeRender();
        }
    }
    status = INT_STATUS_ERROR;
    if (status & INT_ISP_OVERFLOW) {
        error = isp_overflow();
        /* @hack spelling: the test in a loop of one pass, which keeps
         * kmiFatalErrorEntry's address in R14 and status in R12 with the other
         * loops. without the loop, 266 of 442 words differ */
        for (;;) {
            /* @hack spelling: the break before the call when error is 0, which
             * loads the next test's mask into R2 (MOV #8,R2 at 0x8c666246).
             * with if (error) around the call instead of the break, 2 of 442
             * words differ */
            if (!error) {
                break;
            }
            kmiFatalErrorEntry(error);
            break;
        }
    }
    if (status & INT_OL_OVERFLOW) {
        /* @hack spelling: the call in a loop of one pass, which keeps
         * kmiFatalErrorEntry's address in R14 and status in R12 with the other
         * loops. without the loop, 27 of 442 words differ */
        for (;;) {
            error = ol_overflow();
            /* @hack spelling: the break before the call when error is 0, which
             * loads the next test's mask into R2 (MOV #2,R2 at 0x8c66625a).
             * with if (error) around the call instead of the break, 2 of 442
             * words differ */
            if (!error) {
                break;
            }
            kmiFatalErrorEntry(error);
            break;
        }
    }
    if (status & INT_STRIP_OVERRUN) {
        /* @hack spelling: the call in a loop of one pass, which keeps
         * kmiFatalErrorEntry's address in R14 and status in R12 with the other
         * loops. without the loop, 33 of 442 words differ */
        for (;;) {
            error = strip_overrun();
            /* @hack spelling: the break before the call when error is 0, which
             * loads the callback loop's 1 << i into R2 and callback_pending
             * into R3 (MOV R13,R2 at 0x8c666274, 0x8c666278). with if (error)
             * around the call instead of the break, 4 of 442 words differ */
            if (!error) {
                break;
            }
            kmiFatalErrorEntry(error);
            break;
        }
    }
    for (i = 0; i < CALLBACKS; i++) {
        if (kmiDeviceExtension.callback_pending & (1 << i)) {
            kmiDeviceExtension.callback_pending &= ~(1 << i);
            kmiExCallBackFunc(kmiDeviceExtension.callback[i].function,
                              kmiDeviceExtension.callback[i].argument);
        }
    }
    return 0;
}

void kmiDefaultInterruptHandler(KMINT32 event)
{
    kmiDeviceExtension.callback[event].function(
        kmiDeviceExtension.callback[event].argument);
}
