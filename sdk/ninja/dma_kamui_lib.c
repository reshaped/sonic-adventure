/* Kamui DMA transfers into the tile accelerator's address space, on the CPU's
 * DMA channel 2. requests wait in two queues of eight slots, and one queued
 * transfer runs at a time. at each transfer's end, the interrupt handler calls
 * kmiExecuteDMAChain to start the next. kmiCh2DMAtoTA starts a transfer outside
 * the queues */
#include <kamui.h>
#include <machine.h>
#include <sg_xpt.h>

/* DMA channel 2's source, destination, transfer count and control registers,
 * and the DMA operation register */
#define SAR2 (*(volatile KMDWORD *)0xffa00020)
#define DAR2 (*(volatile KMDWORD *)0xffa00024)
#define DMATCR2 (*(volatile KMDWORD *)0xffa00028)
#define CHCR2 (*(volatile KMDWORD *)0xffa0002c)
#define DMAOR (*(volatile KMDWORD *)0xffa00040)
/* the store queues' address control registers. QACR_AREA moves an address's
 * area, its bits 28 to 26, to bits 4 to 2, where QACR0 and QACR1 take it */
#define QACR0 (*(volatile KMDWORD *)0xff000038)
#define QACR1 (*(volatile KMDWORD *)0xff00003c)
#define QACR_AREA(address) ((address) >> 24)
/* channel 2's registers on the system bus: the target in the tile
 * accelerator's address space, the length in bytes, and the start register */
#define CH2_DMA_TARGET (*(volatile KMDWORD *)0xa05f6800)
#define CH2_DMA_LENGTH (*(volatile KMDWORD *)0xa05f6804)
#define CH2_DMA_START (*(volatile KMDWORD *)0xa05f6808)
/* the bus channel 2 takes to texture memory. TEXTURE_BUS_64BIT selects the
 * 64-bit bus */
#define CH2_TEXTURE_BUS (*(volatile KMDWORD *)0xa05f6884)
#define TEXTURE_BUS_64BIT 0
/* the video chip's soft reset register, and its bit that resets the tile
 * accelerator */
#define SOFT_RESET (*(volatile KMDWORD *)0xa05f8008)
#define RESET_TA 1

/* DMAOR's bits */
#define DMAOR_ENABLE 0x1 /* the master enable of every channel */
#define DMAOR_CH2_FIRST 0x200 /* priority to channel 2, then 0, 1 and 3 */
#define DMAOR_ON_DEMAND 0x8000 /* on-demand data transfer mode */

/* CHCR2's bits */
#define CHCR_ENABLE 0x1
#define CHCR_32_BYTES 0x40 /* a 32-byte unit at a time */
#define CHCR_BURST 0x80 /* burst mode, rather than cycle steal */
#define CHCR_TO_BUS 0x200 /* memory to the system bus, at its request */
#define CHCR_SOURCE_UP 0x1000 /* a source address that increments */
#define CONTROL_CYCLE_STEAL (CHCR_TO_BUS | CHCR_32_BYTES | CHCR_ENABLE)
#define CONTROL_BURST (CONTROL_CYCLE_STEAL | CHCR_BURST)

/* the request queues, each a ring of SLOTS slots. SLOTS must be a power of 2:
 * a slot index wraps by a mask of SLOTS - 1 */
#define QUEUES 2
#define SLOTS 8
/* the bytes in a CHCR_32_BYTES unit. DMATCR2 counts a transfer's units */
#define TRANSFER_UNIT 32

/* the tile accelerator's polygon input, as a DMA target */
#define TA_INPUT 0x10000000UL

/* a slot's state holds the flags its request was made with, and DMA_WAITING or
 * DMA_RUNNING while the request waits or runs. a slot with neither is free. the
 * second queue's slots hold DMA_SECOND_QUEUE even while free */
#define DMA_WAITING 1
#define DMA_RUNNING 2
#define DMA_BUSY (DMA_WAITING | DMA_RUNNING)

struct dma_request {
    KMDWORD handle;
    KMDWORD source;
    KMDWORD unk_08;
    KMDWORD target;
    KMDWORD unk_10;
    KMDWORD length;
    KMDWORD state;
    KMDWORD unk_1c;
};

static struct dma_request requests[QUEUES][SLOTS];
static KMDWORD fill_slot[QUEUES];
static KMDWORD next_handle;
static KMDWORD start_slot[QUEUES];
/* the slot and queue of the request started last, which may since have
 * finished. no queued transfer runs while that slot is free */
static KMDWORD started_slot;
static KMDWORD started_queue;

/* @bug memory: kmiDMAHazardReset sets started_queue to 7, past requests'
 * two queues, and started_slot to the last slot, 7. STARTED_REQUEST.state is
 * then requests[7][7].state, the word 0x7f8 bytes past requests' start:
 * adxsjd_obj[7].trapobj at 0x8c8ca128 in 1ST_READ.BIN, and fb_blocks[62].offset
 * at 0x8ce52d98 in LOADER.BIN. until a queued transfer starts,
 * kmiDMAtoTARequest reads that word (1ST_READ.BIN 0x8c663b80-0x8c663b96,
 * LOADER.BIN 0x8ce17d80-0x8ce17d96), and kmiExecuteDMAChain clears its low two
 * bits (1ST_READ.BIN 0x8c663cbe-0x8c663cd6, LOADER.BIN
 * 0x8ce17ea6-0x8ce17ec2) */
#define STARTED_REQUEST (requests[started_queue][started_slot])

void kmiInitializeDMA(void)
{
    KMDWORD i;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    KMDWORD control;

    /* CHCR2's transfer end bit clears only when a write of 0 to it follows a
     * read of the register */
    /* @hack redundant: control is never read. with the store, SHC keeps one
     * address in R0 for the loop's stores to requests[0][i].state and
     * requests[1][i].state, and adds 0x100 between them (1ST_READ.BIN
     * 0x8c663950-0x8c663968, LOADER.BIN 0x8ce17bfc-0x8ce17c0a); with CHCR2
     * read and discarded, each store takes an address of its own, LOADER.BIN's
     * function changes size, and 20 of 57 words differ in 1ST_READ.BIN */
    control = CHCR2;
    SAR2 = 0;
    DAR2 = 0;
    DMATCR2 = 0;
    CHCR2 = (CONTROL_BURST | CHCR_SOURCE_UP) & ~CHCR_ENABLE;
    DMAOR = DMAOR_ON_DEMAND | DMAOR_CH2_FIRST | DMAOR_ENABLE;
    CH2_TEXTURE_BUS = TEXTURE_BUS_64BIT;
    QACR0 = QACR_AREA(TA_INPUT);
    QACR1 = QACR_AREA(TA_INPUT);
    fill_slot[0] = 0;
    fill_slot[1] = 0;
    start_slot[0] = 0;
    start_slot[1] = 0;
    next_handle = 0;
    /* no request has started, so started_slot and started_queue name a free
     * slot: the second queue's last, which the loop below frees */
    started_slot = SLOTS - 1;
    started_queue = 1;
    for (i = 0; i < SLOTS; i++) {
        requests[0][i].state = 0;
        requests[1][i].state = DMA_SECOND_QUEUE;
    }
}

KMINT32 kmiDMAHazardReset(void)
{
    KMDWORD i;
    KMDWORD control;

    CH2_DMA_START = 0;
    SOFT_RESET = RESET_TA;
    SOFT_RESET = 0;
    while (CH2_DMA_START != 0) {
    }
    control = CHCR2;
    CHCR2 = control & ~CHCR_ENABLE;
    SOFT_RESET = RESET_TA;
    SOFT_RESET = 0;
    fill_slot[0] = 0;
    fill_slot[1] = 0;
    start_slot[0] = 0;
    start_slot[1] = 0;
    next_handle = 0;
    started_slot = SLOTS - 1;
#ifdef GCC
    /* repairs the read and clear past requests that STARTED_REQUEST's tag
     * names. GCC sets started_queue as kmiInitializeDMA does, so
     * STARTED_REQUEST names a free slot, which no request fills before the
     * next transfer starts */
    started_queue = 1;
#else
    started_queue = 7;
#endif
    for (i = 0; i < SLOTS; i++) {
        requests[0][i].state = 0;
        requests[1][i].state = DMA_SECOND_QUEUE;
    }
    return 0;
}

KMSTATUS kmiDMAtoTARequest(KMDWORD target, const KMDWORD *source,
                           KMDWORD length, KMDWORD flags, KMDWORD **handle)
{
    int mask;
    KMDWORD queue;
    KMDWORD slot;
    KMDWORD address;
    KMDWORD control;
    KMSTATUS status;

    mask = get_imask();
    set_imask(14);
    if (length == 0) {
        set_imask(mask);
        return DMA_EMPTY;
    }
    status = DMA_QUEUE_FULL;
    queue = (flags & DMA_SECOND_QUEUE) / DMA_SECOND_QUEUE;
    address = (KMDWORD)source;
    slot = fill_slot[queue];
    if (!(requests[queue][slot].state & DMA_BUSY)) {
        if (address & (TRANSFER_UNIT - 1)) {
            address += TRANSFER_UNIT - 1;
            address &= ~(TRANSFER_UNIT - 1);
        }
        if (length & (TRANSFER_UNIT - 1)) {
            length += TRANSFER_UNIT - 1;
            length &= ~(TRANSFER_UNIT - 1);
        }
        requests[queue][slot].handle = next_handle;
        requests[queue][slot].source = address;
        requests[queue][slot].target = target;
        requests[queue][slot].length = length;
        requests[queue][slot].state = flags;
        requests[queue][slot].state |= DMA_WAITING;
        *handle = &requests[queue][slot].handle;
        status = DMA_QUEUED;
        fill_slot[queue] = (fill_slot[queue] + 1) & (SLOTS - 1);
        next_handle++;
    }
    if (status == DMA_QUEUE_FULL) {
        set_imask(mask);
        return status;
    }
    if (!(STARTED_REQUEST.state & DMA_BUSY)) {
        if ((requests[queue][slot].state & DMA_BUSY) == DMA_WAITING) {
            requests[queue][slot].state =
                (requests[queue][slot].state & ~DMA_BUSY) | DMA_RUNNING;
            SAR2 = requests[queue][slot].source;
            DAR2 = (requests[queue][slot].target - TA_INPUT) * 2;
            DMATCR2 = length / TRANSFER_UNIT;
            control = CONTROL_CYCLE_STEAL;
            if (!(requests[queue][slot].state & DMA_CYCLE_STEAL)) {
                control = CONTROL_BURST;
            }
            if (requests[queue][slot].state & DMA_SOURCE_UP) {
                control |= CHCR_SOURCE_UP;
            } else {
                /* @hack redundant: an empty else, which has SHC load the
                 * addresses of CHCR2, CH2_DMA_TARGET and CH2_DMA_LENGTH for the
                 * stores below into R1, R2 and R3, not R3, R1 and R2
                 * (1ST_READ.BIN 0x8c663bf0-0x8c663c00, LOADER.BIN
                 * 0x8ce17df2-0x8ce17e04); without it, 16 of 206 words differ in
                 * 1ST_READ.BIN and 11 of 195 in LOADER.BIN */
            }
            CHCR2 = control;
            CH2_DMA_TARGET = requests[queue][slot].target;
            CH2_DMA_LENGTH = requests[queue][slot].length;
            CH2_DMA_START = 1;
            started_slot = slot;
            started_queue = queue;
            start_slot[queue] = (start_slot[queue] + 1) & (SLOTS - 1);
        }
    }
    set_imask(mask);
    return status;
}

KMBOOLEAN kmiExecuteDMAChain(void)
{
    struct dma_request *done;
    KMDWORD queue;
    KMDWORD slot;
    KMDWORD control;
    KMBOOLEAN started;

    /* @hack spelling: done is set in the clear's own statement, before a comma.
     * that keeps ~DMA_BUSY, for this clear and the loop's, in R9 rather than
     * R1 in 1ST_READ.BIN (0x8c663ca2-0x8c663cba), and in R10 in LOADER.BIN
     * (0x8ce17e92-0x8ce17e9e); as two statements, LOADER.BIN's function changes
     * size and 108 of 124 words differ in 1ST_READ.BIN */
    done = &STARTED_REQUEST, done->state &= ~DMA_BUSY;
    /* @hack spelling: started is set after the clear. that keeps 0 in R7 and
     * DMA_BUSY in R12 in 1ST_READ.BIN (0x8c663ca2, 0x8c663cb2), and
     * DMA_RUNNING in R14 in LOADER.BIN (0x8ce17e92); set before it, 89 of 124
     * words differ in 1ST_READ.BIN and 27 of 125 in LOADER.BIN */
    started = FALSE;
    for (queue = 0; queue < QUEUES; queue++) {
        slot = start_slot[queue];
        if ((requests[queue][slot].state & DMA_BUSY) == DMA_WAITING) {
            requests[queue][slot].state =
                (requests[queue][slot].state & ~DMA_BUSY) | DMA_RUNNING;
            SAR2 = requests[queue][slot].source;
            DAR2 = (requests[queue][slot].target - TA_INPUT) * 2;
            DMATCR2 = requests[queue][slot].length / TRANSFER_UNIT;
            control = CONTROL_CYCLE_STEAL;
            if (!(requests[queue][slot].state & DMA_CYCLE_STEAL)) {
                control = CONTROL_BURST;
            }
            if (requests[queue][slot].state & DMA_SOURCE_UP) {
                control |= CHCR_SOURCE_UP;
            } else {
                /* @hack redundant: an empty else, which has SHC load the
                 * addresses of CHCR2, CH2_DMA_TARGET and CH2_DMA_LENGTH for the
                 * stores below into R1, R2 and R3, not R3, R1 and R2, in
                 * 1ST_READ.BIN (0x8c663d4c-0x8c663d5c), and into R3, R1 and R2,
                 * not R2, R1 and R3, in LOADER.BIN (0x8ce17f36-0x8ce17f48);
                 * without it, 13 of 124 words differ in 1ST_READ.BIN and 16 of
                 * 125 in LOADER.BIN */
            }
            CHCR2 = control;
            CH2_DMA_TARGET = requests[queue][slot].target;
            CH2_DMA_LENGTH = requests[queue][slot].length;
            CH2_DMA_START = 1;
            started_slot = slot;
            started_queue = queue;
            start_slot[queue] = (start_slot[queue] + 1) & (SLOTS - 1);
            started = TRUE;
            break;
        }
    }
    return started;
}

KMBOOLEAN kmiQueryFinishDMA(KMDWORD handle)
{
    KMDWORD queue;
    KMDWORD slot;

    for (queue = 0; queue < QUEUES; queue++) {
        for (slot = 0; slot < SLOTS; slot++) {
            if (requests[queue][slot].handle == handle) {
#ifdef GCC
                /* the DMA's end interrupt clears DMA_BUSY, so each call reads
                 * the state anew. read as plain memory, GCC took this query
                 * for pure, and kmiWaitForFinishDMA asked it once and spun
                 * for good where the transfer was still running */
                if (*(volatile KMDWORD *)&requests[queue][slot].state &
                    DMA_BUSY) {
#else
                if (requests[queue][slot].state & DMA_BUSY) {
#endif
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

KMBOOLEAN kmiWaitForFinishDMA(KMDWORD handle)
{
    while (kmiQueryFinishDMA(handle) != TRUE) {
    }
    return TRUE;
}

/* send length bytes, rounded up to 32, from source to the tile accelerator's
 * polygon input at once, outside both queues. the caller must make sure no
 * queued transfer runs: kmiCh2DMAtoTA sets the channel up without testing it.
 * the result is TRUE, or FALSE for a length of 0, which starts no transfer */
KMSTATUS kmiCh2DMAtoTA(const KMDWORD *source, KMDWORD length)
{
    length = (length + TRANSFER_UNIT - 1) & ~(TRANSFER_UNIT - 1);
    SAR2 = (KMDWORD)source;
    DAR2 = 0;
    DMATCR2 = length / TRANSFER_UNIT;
    CHCR2 = CONTROL_BURST | CHCR_SOURCE_UP;
    if (length != 0) {
        CH2_DMA_TARGET = TA_INPUT;
        CH2_DMA_LENGTH = length;
        CH2_DMA_START = 1;
        return TRUE;
    }
    return FALSE;
}
