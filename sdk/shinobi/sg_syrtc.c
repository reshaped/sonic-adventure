/* the real-time clock: a count of seconds since the start of 1950, in the
 * AICA's clock registers */
#include <sg_chain.h>
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* the clock's registers on the G2 bus: the count's high and low halves, each
 * in the low 16 bits of its word, and the write enable */
#define RTC_HI ((void *)0xa0710000)
#define RTC_LO ((void *)0xa0710004)
#define RTC_WRITE_ENABLE ((void *)0xa0710008)

/* a new moon, as a count of seconds since 1950, and the lunar month and the
 * day in seconds */
#define NEW_MOON 0x5cb417d1
#define LUNAR_MONTH 0x26ee7f
#define DAY_SECONDS 86400
/* syRtcGetAndCheckCount's readings, and the readings syRtcGetCountForce
 * takes after its first, which must all equal it */
#define CHECK_READINGS 2
#define FORCE_READINGS 3

void syRtcDbgSetLogAddr(Uint32 begin, Uint32 limit);
void syRtcDbgWriteLog(Uint32 value);
Uint32 syRtcGetAndCheckCount(Uint32 previous);
Uint32 syRtcGetCountForce(void);
Sint32 syRtcAicaGetCount(Uint32 *count);
Sint32 syRtcAicaSetWriteFlag(void);
Sint32 syRtcAicaSetDateLo(Uint32 value);
Sint32 syRtcAicaSetDateHi(Uint32 value);
Uint8 syRtcMakeDayOfWeek(Uint16 year, Uint8 month, Uint8 day);
Uint8 syRtcMakeAgeOfMoon(SYS_RTC_DATE *date);

char *sygRtcBuild = "\nsyRtc Ver 1.00 Build:Sep 28 1998 05:02:55\n";
/* the last count read, and the server's mode bits. the vblank interrupt
 * runs syRtcExecServer, which reads both and writes the count, so both are
 * volatile. the image reloads them at every use */
volatile Uint32 sygRtcCount;
volatile Sint32 sygRtcStat;
SYCHAIN sygRtcChain;
Sint32 sygRtcChangeStatFlag;
/* the next log word's address, and the address the log stops at */
Uint32 sygRtcDbgLogAddr;
Uint32 sygRtcDbgLogMax;

Sint32 syRtcInit(void)
{
    sygRtcCount = syRtcGetCountForce();
    sygRtcStat = 0;
    syRtcExecServer();
    /* @bug mismatched-call: syRtcExecServer takes no argument, and the chain
     * calls it as a handler, with the NULL registered here in R4, which it
     * ignores, at 0x8c61c702 and 0x8c65f26e in 1ST_READ.BIN, 0x8ce09886 and
     * 0x8ce0c942 in LOADER.BIN */
    sygRtcChain =
        syChainAddHandler(SYD_CHAIN_EVENT_IML6_VBLANK,
                          (void (*)(void *))syRtcExecServer, 92, NULL);
    return 0;
}

void syRtcFinish(void)
{
    sygRtcCount = 0;
    syChainDeleteHandler(sygRtcChain);
}

void syRtcExecServer(void)
{
    if (!(sygRtcStat & RTC_STAT_DIRECT)) {
        sygRtcCount = syRtcGetAndCheckCount(sygRtcCount);
    }
    if (sygRtcStat & RTC_STAT_FULL_READ) {
        sygRtcCount = syRtcGetCountForce();
    }
}

Sint32 syRtcGetDate(SYS_RTC_DATE *date)
{
    Uint32 count;

    if ((sygRtcStat & RTC_STAT_DIRECT) == RTC_STAT_DIRECT) {
        count = syRtcGetCountForce();
        syRtcCountToDate(count, date);
        sygRtcCount = count;
    } else {
        syRtcCountToDate(sygRtcCount, date);
    }
    date->dayofweek = syRtcMakeDayOfWeek(date->year, date->month, date->day);
    date->ageofmoon = syRtcMakeAgeOfMoon(date);
    return sygRtcStat;
}

Sint32 syRtcSetDate(const SYS_RTC_DATE *date)
{
    Uint32 count;
    Sint32 err;
    Uint32 read;

    syRtcDateToCount(date, &count);
    err = syRtcAicaSetWriteFlag();
    if (err == 0) {
        err = syRtcAicaSetDateLo(count & 0xffff);
        if (err == 0) {
            err = syRtcAicaSetDateHi((count >> 16) & 0xffff);
            if (err == 0) {
                /* the clock may tick once in between */
                read = syRtcGetCountForce();
                if (read == count || read == count + 1) {
                    sygRtcCount = read;
                    err = 0;
                } else {
                    err = -1;
                }
            }
        }
    }
    return err;
}

void syRtcSetServerMode(Sint32 mode)
{
    sygRtcStat = mode;
}

Sint32 syRtcGetStat(void)
{
    return sygRtcStat;
}

Sint32 syRtcCompareDate(const SYS_RTC_DATE *date1, const SYS_RTC_DATE *date2)
{
    Uint32 count1;
    Uint32 count2;
    Sint32 order;

    syRtcDateToCount(date1, &count1);
    syRtcDateToCount(date2, &count2);
    if (count1 == count2) {
        order = 0;
    } else if (count1 > count2) {
        order = 1;
    } else {
        order = -1;
    }
    return order;
}

/* every year divisible by 4 is a leap year */
void syRtcDateToCount(const SYS_RTC_DATE *date, Uint32 *count)
{
    Uint32 years;
    Uint32 days;
    Uint32 months[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    years = date->year - 1950;
    days = (years * 365) + ((years + 2) / 4);
    if ((years + 2) % 4 == 0 && date->month <= 2) {
        days--;
    }
    days += months[date->month - 1];
    days += date->day - 1;
    *count = ((((((days * 24) + date->hour) * 60) + date->minute) * 60) +
              date->second);
}

void syRtcCountToDate(Uint32 count, SYS_RTC_DATE *date)
{
    Uint32 rest;
    Uint32 years;
    Uint32 days;
    Uint32 month;
    Uint32 months[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    date->second = count % 60;
    /* @hack spelling: count divided in place, then copied to rest, which puts
     * the dividend's MOV R4,R1 in the division call's delay slot
     * (0x8c61c904, and 0x8ce09a88 in LOADER.BIN); as rest = count / 60, 93
     * of 163 words differ */
    count /= 60;
    rest = count;
    date->minute = rest % 60;
    rest /= 60;
    date->hour = rest % 24;
    rest /= 24;
    years = rest / 365;
    days = rest % 365;
    /* the leap days before the year */
    rest = (years + 1) / 4;
    if (rest <= days) {
        /* @hack spelling: days spelled days = days - rest, which keeps days
         * in R5 and the month table's address in R6 (MOV.L @(4,R6),R1 and
         * CMP/HS R1,R5 at 0x8c61c99c, and at 0x8ce09b20 in LOADER.BIN); as
         * -=, the two trade and 23 of 163 words differ */
        days = days - rest;
        date->year = years + 1950;
    } else {
        date->year = years + 1949;
        days += 365 - rest;
        if (date->year % 4 == 0) {
            days++;
        }
    }
    month = 1;
    if (date->year % 4 == 0) {
        if (days >= months[1]) {
            for (month = 2; month < 12; month++) {
                if (days < months[month] + 1) {
                    break;
                }
            }
        }
    } else {
        for (month = 1; month < 12; month++) {
            if (days < months[month]) {
                break;
            }
        }
    }
    date->month = month;
    date->day = days - months[month - 1] + 1;
    if (date->year % 4 == 0 && date->month > 2) {
        date->day--;
    }
    date->dayofweek = syRtcMakeDayOfWeek(date->year, date->month, date->day);
    date->ageofmoon = syRtcMakeAgeOfMoon(date);
}

/* the count two readings agree on, or previous when they differ. a reading
 * that fails counts as previous */
Uint32 syRtcGetAndCheckCount(Uint32 previous)
{
    Uint32 readings[CHECK_READINGS];
    Uint32 count;
    Sint32 i;
    Sint32 changed;

    count = previous;
    for (i = 0; i < CHECK_READINGS; i++) {
        if (syRtcAicaGetCount(&readings[i]) != 0) {
            readings[i] = previous;
        }
    }
    changed = 0;
    for (i = 0; i < CHECK_READINGS - 1; i++) {
        if (readings[i] != readings[i + 1]) {
            changed = 1;
            break;
        }
    }
    if (changed == 0) {
        count = readings[0];
    }
    return count;
}

/* the clock's count, once FORCE_READINGS readings in a row after the first
 * equal it. a reading that fails or differs starts them again, against the
 * same first. if the clock ticks before they agree, the function never
 * returns */
Uint32 syRtcGetCountForce(void)
{
    Sint32 i;
    Sint32 err;
    Sint32 done;
    Uint32 first;
    Uint32 next;

    done = 0;
    do {
        err = syRtcAicaGetCount(&first);
        if (err == 0) {
            done = 1;
        }
    } while (done == 0);
    /* @hack redundant: a return of -1 that never runs, since the loop above
     * ends only with done set (TST R14,R14 and BRA at 0x8c61cacc and
     * 0x8c61cad0, and at 0x8ce09c50 and 0x8ce09c54 in LOADER.BIN); without
     * it, the function is 8 bytes shorter and 35 of 49 words differ */
    if (done == 0) {
        return -1;
    }
    done = 0;
    do {
        for (i = 0; i < FORCE_READINGS; i++) {
            /* @bug logic: the loop never reads first again. after a
             * reading that differs, the readings start over against the old
             * first. once the clock ticks past it, the test (CMP/EQ R3,R2 at
             * 0x8c61cae6, and at 0x8ce09c6a in LOADER.BIN) never holds, and
             * the loop never ends */
            if (syRtcAicaGetCount(&next) != 0) {
                break;
            } else if (first != next) {
                break;
            }
            first = next;
        }
        if (i == FORCE_READINGS) {
            done = 1;
        }
    } while (done == 0);
    return next;
}

Sint32 syRtcAicaGetCount(Uint32 *count)
{
    Uint32 halves[2];

    if (syG2Read(RTC_HI, halves, SYE_G2_DATA_WIDTH_32BIT, 2,
                 SYE_G2_ADR_MODE_ADD, SYE_G2_ADR_MODE_ADD) != 0) {
        return -1;
    }
    *count = ((halves[0] << 16) & 0xffff0000) | (halves[1] & 0xffff);
    syRtcDbgWriteLog(halves[0]);
    syRtcDbgWriteLog(halves[1]);
    return 0;
}

Sint32 syRtcAicaSetWriteFlag(void)
{
    union {
        Uint16 low;
        Uint32 word;
    } enable;

    enable.low = 1;
    /* @bug uninitialized: the write enable gets enable's high half,
     * which no path writes: MOV.W R3,@R15 stores the low two bytes and
     * syG2Write's MOV.L @R3,R2 sends all four, at 0x8c61cb7c and 0x8c11c7c6
     * in 1ST_READ.BIN, 0x8ce09d00 and 0x8ce0089a in LOADER.BIN */
    syG2Write(&enable, RTC_WRITE_ENABLE, SYE_G2_DATA_WIDTH_32BIT, 1,
              SYE_G2_ADR_MODE_FIX, SYE_G2_ADR_MODE_FIX);
    return 0;
}

Sint32 syRtcAicaSetDateLo(Uint32 value)
{
    syG2Write(&value, RTC_LO, SYE_G2_DATA_WIDTH_32BIT, 1, SYE_G2_ADR_MODE_FIX,
              SYE_G2_ADR_MODE_FIX);
    return 0;
}

Sint32 syRtcAicaSetDateHi(Uint32 value)
{
    syG2Write(&value, RTC_HI, SYE_G2_DATA_WIDTH_32BIT, 1, SYE_G2_ADR_MODE_FIX,
              SYE_G2_ADR_MODE_FIX);
    return 0;
}

/* 0 is Sunday */
Uint8 syRtcMakeDayOfWeek(Uint16 year, Uint8 month, Uint8 day)
{
    if (month <= 2) {
        year--;
        month += 12;
    }
    return ((year / 4) + (((month * 13) + 8) / 5) + year + day - 15) % 7;
}

/* whole days since the last new moon */
Uint8 syRtcMakeAgeOfMoon(SYS_RTC_DATE *date)
{
    Uint32 count;
    Uint32 phase;

    syRtcDateToCount(date, &count);
    /* @hack spelling: each arm's difference and remainder in statements of
     * their own, which keep NEW_MOON in R4 (MOV.L at 0x8c61cc2c, and at
     * 0x8ce09db0 in LOADER.BIN); as one expression an arm, NEW_MOON takes
     * R6, the function is 10 bytes shorter and 33 of 38 words differ */
    if (count >= NEW_MOON) {
        phase = count - NEW_MOON;
        phase %= LUNAR_MONTH;
    } else {
        phase = NEW_MOON - count;
        phase %= LUNAR_MONTH;
        phase = LUNAR_MONTH - phase;
    }
    return phase / DAY_SECONDS;
}

void syRtcDbgSetLogAddr(Uint32 begin, Uint32 limit)
{
    if (sygRtcStat & RTC_STAT_LOG) {
        sygRtcDbgLogAddr = begin;
        sygRtcDbgLogMax = limit;
    }
}

void syRtcDbgWriteLog(Uint32 value)
{
    if (sygRtcStat & RTC_STAT_LOG) {
        if (sygRtcDbgLogAddr < sygRtcDbgLogMax) {
            *(Uint32 *)sygRtcDbgLogAddr = value;
            sygRtcDbgLogAddr += 4;
        }
    }
}
