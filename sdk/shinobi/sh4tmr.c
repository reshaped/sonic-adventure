/* SH-4 timer unit: a free-running count, a one-shot interrupt and a
 * general-purpose timer */
#include <sg_xpt.h>
#include <shinobi.h>

/* the timer unit's registers, by their SH-4 names: the output control and
 * start registers, then each timer's constant, count and control registers */
/* @hack type: the 8- and 16-bit registers signed, which SHC tests without
 * an EXTU.B or EXTU.W after the load (MOV.B @R13,R0 and TST #2,R0 at
 * 0x8c61cd7c, and at 0x8ce0cef8 in LOADER.BIN); unsigned, 104 of the 253
 * words of the eleven functions that use them differ, 57 of the 63 of
 * syTmrInt1SetEx */
#define TOCR (*(volatile Sint8 *)0xffd80000)
#define TSTR (*(volatile Sint8 *)0xffd80004)
#define TCOR0 (*(volatile Uint32 *)0xffd80008)
#define TCNT0 (*(volatile Uint32 *)0xffd8000c)
#define TCR0 (*(volatile Sint16 *)0xffd80010)
#define TCOR1 (*(volatile Uint32 *)0xffd80014)
#define TCNT1 (*(volatile Uint32 *)0xffd80018)
#define TCR1 (*(volatile Sint16 *)0xffd8001c)
#define TCOR2 (*(volatile Uint32 *)0xffd80020)
#define TCNT2 (*(volatile Uint32 *)0xffd80024)
#define TCR2 (*(volatile Sint16 *)0xffd80028)

/* each timer's start bit, in TSTR */
#define STR0 0x1
#define STR1 0x2
#define STR2 0x4
/* TCR bits */
#define UNF 0x100 /* underflow */
#define UNIE 0x20 /* underflow interrupt enable */
#define TPSC 0x7 /* clock select */

/* the IPRA fields of timers 1 and 2, both set to priority 15 */
/* @hack type: IPRA signed, as the timer registers are, which has SHC load
 * ~IPRA_TMU1 and ~IPRA_TMU2 as sign-extended words (MOV.W at 0x8c61cdb4
 * and 0x8c61ceda, and at 0x8ce0cf30 and 0x8ce0d056 in LOADER.BIN);
 * unsigned, they load as longs and 19 of 117 words differ */
#define IPRA (*(volatile Sint16 *)0xffd00004)
#define IPRA_TMU1 0x0f00
#define IPRA_TMU2 0x00f0

#define COUNT_MAX 0xffffffff

typedef void (*timer_func)(void *arg);

const char *sygTmrBuild = "\nsyTmr Ver 1.00 Build:Sep 25 1998 12:34:18\n";

static timer_func one_shot_func;
static void *one_shot_arg;
static timer_func gen_func;
static void *gen_arg;

static void one_shot_handler(void);
static void gen_handler(void);

void syTmrInit(void)
{
    TOCR = 0;
    TSTR &= ~STR0;
    TCR0 = SYD_TMR_CLOCK_P64;
    TCOR0 = COUNT_MAX;
    TCNT0 = COUNT_MAX;
    TSTR |= STR0;
}

Uint32 syTmrGetCount(void)
{
    /* @hack type: TCNT0 cast to its own type, which puts COUNT_MAX's MOV
     * #-1,R0 ahead of the load (0x8c61cd12, and 0x8ce0ce8e in LOADER.BIN);
     * without it, 3 of 5 words differ */
    return COUNT_MAX - (Uint32)TCNT0;
}

Uint32 syTmrDiffCount(Uint32 count1, Uint32 count2)
{
    return count2 - count1;
}

/* count / 100 and count % 100 are scaled apart, which keeps each product
 * within 32 bits */
Uint32 syTmrCountToMicro(Uint32 count)
{
    return ((count / 100) * 128) + ((count % 100) * 128 / 100);
}

Uint32 syTmrMicroToCount(Uint32 micro)
{
    return ((micro >> 7) * 100) + ((micro & 127) * 100 >> 7);
}

Sint32 syTmrInt1Set(Uint32 count, void (*func)(void *), void *arg)
{
    return syTmrInt1SetEx(count, func, arg, SYD_TMR_CLOCK_P64);
}

Sint32 syTmrInt1SetEx(Uint32 count, void (*func)(void *), void *arg,
                      SY_TMR_CLOCK clock)
{
    if (TSTR & STR1) {
        return SYD_TMR_ERR_EXIST;
    }
    TOCR = 0;
    TSTR &= ~STR1;
    TCR1 = clock;
    if (func != NULL) {
        USER_VECT(SYD_INT_EVENT_TUNI1) = (Uint32)one_shot_handler;
        one_shot_func = func;
        one_shot_arg = arg;
        TCR1 |= UNIE;
        IPRA = (IPRA & ~IPRA_TMU1) | IPRA_TMU1;
        TCR1 &= ~UNF;
    }
    TCOR1 = count;
    TCNT1 = count;
    TSTR |= STR1;
    if (func == NULL) {
        while (!(TCR1 & UNF)) {
        }
        TSTR &= ~STR1;
        TCR1 &= ~UNF;
    }
    return SYD_TMR_ERR_OK;
}

void syTmrInt1Cancel(void)
{
    TSTR &= ~STR1;
    TCR1 &= ~UNIE;
}

static void one_shot_handler(void)
{
    TSTR &= ~STR1;
    TCR1 &= ~UNIE;
    TCR1 &= ~UNF;
    one_shot_func(one_shot_arg);
}

static void gen_handler(void)
{
    TSTR &= ~STR2;
    TCR2 &= ~UNIE;
    TCR2 &= ~UNF;
    gen_func(gen_arg);
}

void syTmrGenStart(void)
{
    TOCR = 0;
    TSTR |= STR2;
}

void syTmrGenStop(void)
{
    TSTR &= ~STR2;
}

void syTmrGenSetInt(void (*func)(void *), void *arg)
{
    Sint8 started;

    started = TSTR;
    TSTR &= ~STR2;
    if (func != NULL) {
        USER_VECT(SYD_INT_EVENT_TUNI2) = (Uint32)gen_handler;
        gen_func = func;
        gen_arg = arg;
        TCR2 |= UNIE;
        IPRA = (IPRA & ~IPRA_TMU2) | IPRA_TMU2;
        TCR2 &= ~UNF;
    }
    TSTR = started;
    if (func == NULL) {
        TOCR = 0;
        TSTR |= STR2;
        while (!(TCR2 & UNF)) {
        }
        TSTR &= ~STR2;
        TCR2 &= ~UNF;
    }
}

void syTmrGenCancelInt(void)
{
    Sint8 started;

    started = TSTR;
    TSTR &= ~STR2;
    TCR2 &= ~UNIE;
    TSTR = started;
}

void syTmrGenSetClock(SY_TMR_CLOCK clock)
{
    Sint8 started;

    if (clock != SYD_TMR_CLOCK_P4 && clock != SYD_TMR_CLOCK_P16 &&
        clock != SYD_TMR_CLOCK_P64 && clock != SYD_TMR_CLOCK_P256 &&
        clock != SYD_TMR_CLOCK_P1024) {
        clock = SYD_TMR_CLOCK_P64;
    }
    started = TSTR;
    TSTR &= ~STR2;
    TCR2 = (TCR2 & ~TPSC) | clock;
    TSTR = started;
}

/* TCNT2 takes -count, so syTmrGenGetCount's COUNT_MAX - TCNT2 reads
 * count - 1 */
void syTmrGenSetCount(Uint32 count)
{
    Sint8 started;

    started = TSTR;
    TSTR &= ~STR2;
    TCOR2 = COUNT_MAX;
    TCNT2 = -count;
    TSTR = started;
}

Uint32 syTmrGenGetCount(void)
{
    /* @hack type: TCNT2 cast to its own type, which puts COUNT_MAX's MOV
     * #-1,R0 between the address's load and the count's (0x8c61cfc2, and
     * 0x8ce0d13e in LOADER.BIN); without it, 2 of 5 words differ */
    return COUNT_MAX - (Uint32)TCNT2;
}

/* the microseconds in count at clock, scaled as syTmrCountToMicro scales
 * them: a count is the clock's divisor in cycles of the 50 MHz peripheral
 * clock. any clock but SYD_TMR_CLOCK_P4, _P16, _P256 and _P1024 counts as
 * SYD_TMR_CLOCK_P64 */
Uint32 syTmrGenCountToMicro(Uint32 count, SY_TMR_CLOCK clock)
{
    Uint32 scale;

    switch (clock) {
    case SYD_TMR_CLOCK_P4:
        scale = 8;
        break;
    case SYD_TMR_CLOCK_P16:
        scale = 32;
        break;
    case SYD_TMR_CLOCK_P256:
        scale = 512;
        break;
    case SYD_TMR_CLOCK_P1024:
        scale = 2048;
        break;
    default:
        scale = 128;
        break;
    }
    return ((count / 100) * scale) + ((count % 100) * scale / 100);
}

Uint32 syTmrGenMicroToCount(Uint32 micro, SY_TMR_CLOCK clock)
{
    Uint32 scale;

    switch (clock) {
    case SYD_TMR_CLOCK_P4:
        scale = 8;
        break;
    case SYD_TMR_CLOCK_P16:
        scale = 32;
        break;
    case SYD_TMR_CLOCK_P256:
        scale = 512;
        break;
    case SYD_TMR_CLOCK_P1024:
        scale = 2048;
        break;
    default:
        scale = 128;
        break;
    }
    return ((micro / scale) * 100) + ((micro % scale) * 100 / scale);
}

Uint32 syTmrGenDiffCount(Uint32 count1, Uint32 count2)
{
    return syTmrDiffCount(count1, count2);
}
