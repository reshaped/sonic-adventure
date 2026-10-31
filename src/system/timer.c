/* the frame timer, on the SH-4's timer 1 */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/host.h"
#include "draw/model.h"
#include "system/timer.h"

/* the timer unit's registers, by their SH-4 names: the start register, and
 * timer 1's constant, count and control registers */
/* @hack type: the registers not volatile, which lets SHC schedule other
 * instructions among their reads and writes: timer_start puts pool loads
 * and the saves of R14 and PR between TSTR's address and its read
 * (0x8c06fae0 to 0x8c06faf0). as volatile, 40 of the 126 words of the six
 * functions that use them differ */
#ifdef GCC
#define TSTR (*(volatile Uint8 *)0xffd80004)
#define TCOR1 (*(volatile Uint32 *)0xffd80014)
#define TCNT1 (*(volatile Uint32 *)0xffd80018)
#define TCR1 (*(volatile Uint16 *)0xffd8001c)
#else
#define TSTR (*(Uint8 *)0xffd80004)
#define TCOR1 (*(Uint32 *)0xffd80014)
#define TCNT1 (*(Uint32 *)0xffd80018)
#define TCR1 (*(Uint16 *)0xffd8001c)
#endif
/* TSTR's bit that runs timer 1 */
#define STR1 0x02
/* TCR's setting for a count at a quarter of the 50 MHz peripheral clock,
 * with no underflow interrupt */
#define TCR_PCLK_4 0

/* a second's ticks at 12.5 MHz, TCR_PCLK_4's rate. timer_start has timer 1
 * count down from this and reload it after 0, so the timer starts over each
 * second */
#define TICKS_PER_SECOND 12500000
/* a tick, 0.08 microseconds, in thousandths of a vsync. a microsecond is
 * 0.001 milliseconds, and a millisecond 60 thousandths of a vsync */
#define TICK (0.08F * 0.001F * 60.0F)

#define TIMER_COUNTS 8

static int dat_8c1d4878 = 6;
int debug_page = 0;
Bool draw_off = FALSE;
static int dat_8c1d4884 = 0;

static int dat_8c795150;
/* the counts timer_show_counts prints. timer_start sets each to -1, and
 * only it writes them, so timer_show_counts prints none */
static int timer_counts[TIMER_COUNTS];
/* TCNT1 at the mark, and as Kamui's last render ended */
static Uint32 timer_marked;
static Uint32 timer_rendered;

static void timer_end_of_render(void *arg);

void timer_start(void)
{
    int i;
    Uint8 tstr = TSTR;

    TSTR = tstr & ~STR1;
    timer_marked = TICKS_PER_SECOND;
    TCR1 = TCR_PCLK_4;
    TCOR1 = timer_marked;
    TCNT1 = timer_marked;
    tstr |= STR1;
    TSTR = tstr;
    /* @hack type: a plain 0 as the callback's argument, which puts TSTR's
     * address in R5 and timer_marked's in R6 (0x8c06fae0, 0x8c06fae4); as
     * NULL, the two trade and 8 of 50 words differ */
    kmSetEORCallback(timer_end_of_render, 0);
    for (i = 0; i < TIMER_COUNTS; i++) {
        timer_counts[i] = -1;
    }
    for (i = 0; i < DRAW_DEBUG_COUNTS; i++) {
        draw_debug_counts[i] = 0;
    }
}

Float timer_elapsed_and_mark(void)
{
    /* @hack spelling: TCNT1's count through a local, which SHC reads into
     * R4 (0x8c06fb48); without it, the count takes R3, and 21 of 22 words
     * differ */
    Uint32 count = TCNT1;
    /* @hack spelling: the time through a local, which SHC computes in FR4
     * and copies to FR0 in the RTS's slot (0x8c06fb68 to 0x8c06fb6e);
     * without it, the time takes FR0, and 6 of 22 words differ */
    Float time;

    timer_marked = count;
    time = ((Float)TICKS_PER_SECOND - timer_marked) * TICK;
    return time;
}

Float timer_lap(void)
{
    Uint32 count = TCNT1;
    Uint32 ticks = timer_marked - count;
    /* @hack spelling: the time through a local, which SHC computes in FR4
     * and copies to FR0 in the RTS's slot (0x8c06fbde to 0x8c06fbe6);
     * without it, the time takes FR0, and 4 of 20 words differ */
    Float time;

    timer_marked = count;
    time = ticks * TICK;
    return time;
}

void timer_mark(void)
{
    /* @hack spelling: TCNT1's count through a local, which SHC reads into
     * R4 (0x8c06fbec); without it, the count takes R3, and 4 of 5 words
     * differ */
    Uint32 count = TCNT1;

    timer_marked = count;
}

Float timer_render_time(void)
{
    /* @hack spelling: the time through a local, which SHC computes in FR4
     * and copies to FR0 in the RTS's slot (0x8c06fc10 to 0x8c06fc16);
     * without it, the time takes FR0, and 6 of 19 words differ */
    Float time = ((Float)TICKS_PER_SECOND - timer_rendered) * TICK;

    return time;
}

Float timer_elapsed(void)
{
    /* @hack spelling: TCNT1's count through a local, which SHC reads into
     * R4 (0x8c06fc1a); without it, the count takes R3, and 7 of 19 words
     * differ */
    Uint32 count = TCNT1;
    /* @hack spelling: the time through a local, which SHC computes in FR4
     * and copies to FR0 in the RTS's slot (0x8c06fc36 to 0x8c06fc3c);
     * without it, the time takes FR0, and 6 of 19 words differ */
    Float time = ((Float)TICKS_PER_SECOND - count) * TICK;

    return time;
}

/* the callback Kamui calls as each render ends. it takes the place of
 * Ninja's callback, njSetEORflag, and calls it in turn */
static void timer_end_of_render(void *arg)
{
    /* @hack spelling: TCNT1's count through a local, which SHC reads into
     * R4 and stores in the JSR's slot (0x8c06fc46, 0x8c06fc4a); without it,
     * the count takes R3, the store moves ahead of the call, and 10 of 10
     * words differ */
    Uint32 count = TCNT1;

    timer_rendered = count;
    /* @bug mismatched-call: njSetEORflag takes Kamui's callback argument, and
     * gets none: as the JSR at 0x8c06fc48 calls it, R4 holds TCNT1's count,
     * read at 0x8c06fc46, which njSetEORflag ignores */
    njSetEORflag();
    host_serve_request();
}

void timer_show_counts(void)
{
    int i;
    int row;

    if (debug_page) {
        row = 16;
        for (i = 0; i < TIMER_COUNTS; i++) {
            if (timer_counts[i] > 0) {
                njPrintD(NJM_LOCATION(35, row), timer_counts[i] + 1, 4);
            }
            row++;
        }
    }
}
