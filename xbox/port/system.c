/* Shinobi's system calls the port layer serves: the interrupt mask, the
 * interrupt chains, the timer, the heap, the cable, the clock, the system
 * configuration, the caches and the hardware's setup.
 *
 * the vertical blank is the one interrupt: the host interrupts the game
 * at each, through interrupt.c's entry, as the console's level 6 interrupt
 * preempts it. while the mask is 6 or more, a blank waits, and runs once the mask
 * drops below 6 */
#include <machine.h>
#include <sg_chain.h>
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "xbox/port/host.h"
#include "xbox/port/port.h"

/* the vertical blank's level */
#define VBLANK_LEVEL 6

static volatile int imask;
static volatile int vblank_waiting;

/* each event's handlers, by priority: a slot is free where its isr is null.
 * a handle is its slot's index plus 1 */
#define CHAIN_SLOTS 64
static struct {
    SY_CHAIN_EVENT event;
    void (*isr)(void *);
    Uint32 priority;
    void *arg;
    Uint32 added; /* the count of handlers added before it */
} chains[CHAIN_SLOTS];
static Uint32 chains_added;

void pads_vblank(void);

static void run_chain(SY_CHAIN_EVENT event);
void port_fiber_interrupt_return(void);
void port_interrupt_init(void);

/* the blank's handlers, at the blank's level: the pads, the vertical
 * blank's chain, then the Maple bus's DMA end, which reads the pads within
 * the blank: its level 4 handlers, the backup library's server among them,
 * run then, and the G2 DMA ends of the transfers since the last blank. a blank that waited while they ran runs again */
volatile Uint32 port_blanks;

/* the G2 DMA channels whose transfers xbox/sdk/shinobi/sy_g2_dma_trigger.c
 * has done: each one's end chain, as its interrupt at level 4. the
 * interrupt is taken once the mask lets it, as the console takes it within
 * microseconds of the transfer: where the mask drops below 4
 * (_builtin_set_imask), or at the blank. an end handler that starts the next
 * transfer, as the sound library's queue does, has its end run in the same
 * pass, so a queue of transfers drains at once and not one a blank */
extern volatile Uint32 port_g2_dma_ended;
static int g2_dma_ends_running;

static void run_g2_dma_ends(void)
{
    static const SY_CHAIN_EVENT events[4] = {SYD_CHAIN_EVENT_IML4_AICADM, SYD_CHAIN_EVENT_IML4_EX0DMA,
                                             SYD_CHAIN_EVENT_IML4_EX1DMA, SYD_CHAIN_EVENT_IML4_DADMAE};
    int k;

    if (g2_dma_ends_running) {
        return;
    }
    g2_dma_ends_running = 1;
    while (port_g2_dma_ended != 0) {
        for (k = 0; k < 4; k++) {
            if (port_g2_dma_ended & (1u << k)) {
                port_g2_dma_ended &= ~(1u << k);
                run_chain(events[k]);
            }
        }
    }
    g2_dma_ends_running = 0;
}

/* the Maple bus's DMA end, at level 4, which a blank with the mask at 4 or
 * above owes until the mask drops below 4, as the console holds the
 * interrupt: its handlers, the backup library's server among them, run the
 * memory cards' fibers, and a blank during one of their host calls must not
 * run the server again from inside its own fiber */
static volatile int maple_end_owed;

void port_blank_handlers(void)
{
    int masked = imask;

    port_blanks++;
    do {
        vblank_waiting = 0;
        imask = VBLANK_LEVEL;
        pads_vblank();
        run_chain(SYD_CHAIN_EVENT_IML6_VBLANK);
        if (masked < 4) {
            imask = 4;
            maple_end_owed = 0;
            run_chain(SYD_CHAIN_EVENT_IML4_MPDMAE);
            run_g2_dma_ends();
        } else {
            maple_end_owed = 1;
        }
        imask = masked;
    } while (vblank_waiting && masked < VBLANK_LEVEL);
}

/* whether the mask holds a blank back, which then waits for it to drop */
int port_blank_masked(void)
{
    if (imask >= VBLANK_LEVEL) {
        vblank_waiting = 1;
        return 1;
    }
    return 0;
}

/* a blank outside the interrupt entry: one that waited for the mask, run as
 * it drops, with the delayed switch it may ask for */
static void run_vblank(void)
{
    port_blank_handlers();
    port_fiber_interrupt_return();
}

/* a handler's place in its chain: by priority, the lowest first, and among
 * equal priorities the one added last first */
static unsigned long long chain_order(int k)
{
    return (unsigned long long)chains[k].priority << 32 | (0xffffffffu - chains[k].added);
}

/* each of event's handlers, in their order. a handler may add or delete
 * handlers as it runs: each pass takes the next by order after the last */
static void run_chain(SY_CHAIN_EVENT event)
{
    unsigned long long last = 0;
    int first = 1;

    for (;;) {
        int next = -1;
        int k;

        for (k = 0; k < CHAIN_SLOTS; k++) {
            if (chains[k].isr != NULL && chains[k].event == event && (first || chain_order(k) > last) &&
                (next < 0 || chain_order(k) < chain_order(next))) {
                next = k;
            }
        }
        if (next < 0) {
            return;
        }
        first = 0;
        last = chain_order(next);
        chains[next].isr(chains[next].arg);
    }
}

/* njSystem.c's waitVsync, on the port's side: the host's next blank */
void port_wait_vsync(void)
{
    HOST->wait_vblank();
}

void _builtin_set_imask(int mask)
{
    imask = mask & 15;
    if (imask < VBLANK_LEVEL && vblank_waiting) {
        vblank_waiting = 0;
        run_vblank();
    }
    if (imask < 4 && maple_end_owed) {
        int masked = imask;

        imask = 4;
        maple_end_owed = 0;
        run_chain(SYD_CHAIN_EVENT_IML4_MPDMAE);
        imask = masked;
    }
    if (imask < 4 && port_g2_dma_ended != 0 && !g2_dma_ends_running) {
        int masked = imask;

        imask = 4;
        run_g2_dma_ends();
        imask = masked;
    }
}

int _builtin_get_imask(void)
{
    return imask;
}

void *_builtin_get_vbr(void)
{
    return NULL;
}

/* FPSCR: the port's FPU keeps the console's mode throughout, so a write
 * keeps its bits for a read and changes nothing else */
static int fpscr = 0x00040001;

void _builtin_set_fpscr(int cr)
{
    fpscr = cr;
}

int _builtin_get_fpscr(void)
{
    return fpscr;
}

/* a handler runs ahead of the event's first of no lower priority, as
 * Shinobi orders them: the slots keep that order for each event */
SYCHAIN syChainAddHandler(SY_CHAIN_EVENT event, void (*usrIsr)(void *), Uint32 priority, void *arg)
{
    int free;

    /* a handler keeps its slot, so the handle, the slot's number, stays its
     * own until it is deleted. run_chain orders the slots */
    for (free = 0; free < CHAIN_SLOTS && chains[free].isr != NULL; free++) {
    }
    if (free == CHAIN_SLOTS) {
        return 0;
    }
    chains[free].event = event;
    chains[free].isr = usrIsr;
    chains[free].priority = priority;
    chains[free].arg = arg;
    chains[free].added = chains_added++;
    return (SYCHAIN)(free + 1);
}

void syChainDeleteHandler(SYCHAIN chain)
{
    int k = (int)chain - 1;

    if (k >= 0 && k < CHAIN_SLOTS) {
        chains[k].isr = NULL;
    }
}

/* the timer counts 781250 times a second, as the console's TMU at the
 * peripheral clock over 64, from the host's microseconds */
Uint32 syTmrGetCount(void)
{
    return (Uint32)((unsigned long long)HOST->time_us() * 25 / 32);
}

Uint32 syTmrDiffCount(Uint32 count1, Uint32 count2)
{
    return count2 - count1;
}

Uint32 syTmrCountToMicro(Uint32 count)
{
    return (Uint32)((unsigned long long)count * 32 / 25);
}

Uint32 syTmrMicroToCount(Uint32 micro)
{
    return (Uint32)((unsigned long long)micro * 25 / 32);
}

/* timer.c's timer 1, which counts down from TCOR1 at 12.5 MHz and reloads
 * after 0. timer_start restarts it each frame, and stage.c takes a frame's
 * extra step by the time since: port_tcnt1_set is its write of TCNT1, and
 * the count runs down from that moment. with the write lost, the count was
 * the host clock's place in the second, a frame looked late 29 times in 30,
 * and play ran three steps a frame for two, half as fast again */
Uint8 port_tstr;
Uint32 port_tcor1 = 0xffffffffu;
Uint16 port_tcr1;
static volatile Uint32 tcnt1;
/* the host's time at the last write, and the ticks the count then stood
 * below TCOR1 */
static unsigned tcnt1_set_us;
static Uint32 tcnt1_set_below;

void port_tcnt1_set(Uint32 count)
{
    tcnt1_set_us = HOST->time_us();
    tcnt1_set_below = port_tcor1 - count;
}

volatile Uint32 *port_tcnt1(void)
{
    /* the host's microseconds wrap, so their difference is taken unsigned */
    unsigned long long ticks =
        tcnt1_set_below + (unsigned long long)(HOST->time_us() - tcnt1_set_us) * 25 / 2;
    unsigned long long period = (unsigned long long)port_tcor1 + 1;

    tcnt1 = port_tcor1 - (Uint32)(ticks % period);
    return &tcnt1;
}

/* the heap: first fit in the block syMallocInit gives, each allocation
 * after a header of its size, 32-byte aligned as Shinobi's are */
struct block {
    Uint32 size; /* the block's bytes, its header's among them */
    Uint32 used;
    Uint32 pad[6];
};
static struct block *heap_first;
static Uint8 *heap_end;

void syMallocInit(void *heap, Uint32 size)
{
    Uint8 *start = (Uint8 *)(((Uint32)heap + 31) & ~31u);

    heap_end = (Uint8 *)heap + size;
    heap_first = (struct block *)start;
    heap_first->size = (Uint32)(heap_end - start) & ~31u;
    heap_first->used = 0;
}

void syMallocFinish(void)
{
    heap_first = NULL;
}

void *syMalloc(Uint32 size)
{
    struct block *at = heap_first;
    Uint32 need = (size + sizeof(struct block) + 31) & ~31u;

    while (at != NULL && (Uint8 *)at < heap_end) {
        if (!at->used && at->size >= need) {
            if (at->size - need >= 64) {
                struct block *rest = (struct block *)((Uint8 *)at + need);

                rest->size = at->size - need;
                rest->used = 0;
                at->size = need;
            }
            at->used = 1;
            return at + 1;
        }
        at = (struct block *)((Uint8 *)at + at->size);
    }
    return NULL;
}

void syFree(void *ptr)
{
    struct block *at;
    struct block *next;

    if (ptr == NULL) {
        return;
    }
    at = (struct block *)ptr - 1;
    at->used = 0;
    /* join the free blocks after it */
    next = (struct block *)((Uint8 *)at + at->size);
    while ((Uint8 *)next < heap_end && !next->used) {
        at->size += next->size;
        next = (struct block *)((Uint8 *)at + at->size);
    }
}

/* the heap's free bytes, headers among them, and its largest free block's */
void syMallocStat(Uint32 *wholeFreeSize, Uint32 *biggestFreeBlockSize)
{
    struct block *at = heap_first;
    Uint32 whole = 0, biggest = 0;

    while (at != NULL && (Uint8 *)at < heap_end && at->size != 0) {
        if (!at->used) {
            whole += at->size;
            biggest = at->size > biggest ? at->size : biggest;
        }
        at = (struct block *)((Uint8 *)at + at->size);
    }
    if (wholeFreeSize != NULL) {
        *wholeFreeSize = whole;
    }
    if (biggestFreeBlockSize != NULL) {
        *biggestFreeBlockSize = biggest;
    }
}

/* the port's picture is VGA's, 640 by 480 */
SYE_CBL_CABLE syCblCheckCable(void)
{
    return SYE_CBL_CABLE_VGA;
}

void syHwInit(void)
{
    port_interrupt_init();
}

/* the G2 bus's work, which the sound library's DMA and register calls use */
void syHwInit2(void)
{
    syG2SysInit(NULL);
}

void syHwFinish(void)
{
}

void syBtExit(void)
{
    HOST->exit(0);
}

Sint32 syRtcInit(void)
{
    return 0;
}

void syRtcFinish(void)
{
}

Sint32 syRtcGetDate(SYS_RTC_DATE *date)
{
    int fields[7];

    HOST->get_date(fields);
    date->year = (Uint16)fields[0];
    date->month = (Uint8)fields[1];
    date->day = (Uint8)fields[2];
    date->hour = (Uint8)fields[3];
    date->minute = (Uint8)fields[4];
    date->second = (Uint8)fields[5];
    date->dayofweek = (Uint8)fields[6];
    return 0;
}

/* the store queues' address registers, QACR0 and QACR1, which the console
 * has at 0xff000038 and 0xff00003c and the Xbox's flash ROM covers: the SDK
 * sources write them here under XBOX, and nothing reads them */
volatile Uint32 port_qacr[2];

/* the system configuration: stereo sound, and nothing else kept */
static Sint32 sound_mode;

Sint32 syCfgInit(void *work)
{
    (void)work;
    return 0;
}

Sint32 syCfgExit(void)
{
    return 0;
}

Sint32 syCfgSetSoundMode(Sint32 mode)
{
    sound_mode = mode;
    return 0;
}

Sint32 syCfgGetSoundMode(Sint32 *mode)
{
    *mode = sound_mode;
    return 0;
}

/* x86 keeps its caches coherent */
void syCachePurgeAll(void)
{
}

void syCacheICI_P2_(void)
{
}

void syCacheOCWB(void *address, Uint32 size)
{
    (void)address;
    (void)size;
}
