/* Sofdec time: each handle's time sources, the library's vsync count, and
 * time code conversion */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

/* error codes for a call on an invalid handle */
#define SFD_ERR_GETTIME ((Sint32)0xff000110)
#define SFD_ERR_USRFRM ((Sint32)0xff000111)
#define SFD_ERR_USRTIMEFN ((Sint32)0xff000112)
#define SFD_ERR_USRSKIPFN ((Sint32)0xff000113)

/* the largest time: bound_time before any time code, and interval and
 * min_delta before any delta */
#define SFTIM_MAX 0x7fffffff

/* a time code conversion: stores the time code's time and its unit, for a
 * rate in frames per 1000 seconds */
typedef void (*sftim_tc2timefn)(Sint32 rate, struct sftim_tcode *tcode,
                                Sint32 *time, Sint32 *tunit);

static void sftim_InitTcode(struct sftim_tcode *tcode);
static Sint32 sftim_GetTimeNone(struct sfd_handle *hn, Sint32 *time,
                                Sint32 *tunit);
static Sint32 sftim_GetTimeVsync(struct sfd_handle *hn, Sint32 *time,
                                 Sint32 *tunit);
static Sint32 sftim_GetTimeUfrm(struct sfd_handle *hn, Sint32 *time,
                                Sint32 *tunit);
static void sftim_Tc2TimeN(Sint32 rate, struct sftim_tcode *tcode, Sint32 *time,
                           Sint32 *tunit);
static void sftim_Tc2Time23N(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit);
static void sftim_Tc2Time29N(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit);
static void sftim_Tc2Time59N(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit);
static void sftim_Tc2Time23D(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit);
static void sftim_Tc2Time29D(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit);
static void sftim_Tc2Time59D(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit);

/* each rate in frames per 1000 seconds, indexed by a time code's rate. each
 * NTSC rate is one less than 24000, 30000 or 60000 divided by 1.001 and
 * rounded down */
static const Sint32 sftim_prate[9] = {
    0, 23975, 24000, 25000, 29969, 30000, 50000, 59939, 60000,
};

/* each rate's conversion, without and with dropped frames */
static const sftim_tc2timefn sftim_tc2time[9][2] = {
    {NULL, NULL},
    {sftim_Tc2Time23N, sftim_Tc2Time23D},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
    {sftim_Tc2Time29N, sftim_Tc2Time29D},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
    {sftim_Tc2Time59N, sftim_Tc2Time59D},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
};

void SFTIM_Init(struct sftim_clock *clock, Sint32 tunit)
{
    clock->nvsync = 0;
    clock->unk_04 = 0;
    clock->tunit = tunit;
}

void SFTIM_Finish(struct sftim_clock *clock)
{
}

/* set up a handle's time: its sources for no time, the vsyncs and the
 * caller's frames, and no time code seen yet. the ADX decoder stage sets the
 * audio source, and SFD_SetUsrTimeFn the caller's own */
void SFTIM_InitHn(struct sfd_handle *hn, struct sftim *tim)
{
    SFTIM_SetTimeFn(hn, sftim_GetTimeNone, SFTIM_SRC_NONE);
    SFTIM_SetTimeFn(hn, sftim_GetTimeVsync, SFTIM_SRC_VSYNC);
    SFTIM_SetTimeFn(hn, NULL, SFTIM_SRC_AUDIO);
    SFTIM_SetTimeFn(hn, sftim_GetTimeUfrm, SFTIM_SRC_USER_FRAME);
    SFTIM_SetTimeFn(hn, NULL, SFTIM_SRC_USER);
    tim->skipfn = NULL;
    sftim_InitTcode(&tim->tcode);
    tim->bound_time = SFTIM_MAX;
    tim->bound_tunit = 1;
    tim->itime = -1;
    tim->interval = SFTIM_MAX;
    tim->max_delta = 0;
    tim->min_delta = SFTIM_MAX;
    tim->nufrm = 0;
    tim->nvsync = 0;
    tim->tunit = 1000;
}

static void sftim_InitTcode(struct sftim_tcode *tcode)
{
    tcode->rate = 0;
    tcode->drop = 0;
    tcode->hour = 0;
    tcode->minute = 0;
    tcode->second = 0;
    tcode->frame = 0;
    tcode->nadd = 0;
}

/* record an I picture's time, and fold the delta from the last one into the
 * interval between I pictures. the first call records the time alone, and a
 * delta of 0 changes nothing. the first delta replaces the interval. after
 * that, a delta 8 or more below the interval moves the interval down by an
 * eighth of the difference, rounded down. any other delta replaces it */
void SFTIM_UpdateItime(struct sftim *tim, Sint32 itime)
{
    Sint32 delta;
    Uint32 step;

    if (tim->itime == -1) {
        tim->itime = itime;
        return;
    }
    delta = itime - tim->itime;
    if (delta == 0) {
        return;
    }
    tim->itime = itime;
    tim->max_delta = (tim->max_delta > delta) ? tim->max_delta : delta;
    tim->min_delta = (tim->min_delta < delta) ? tim->min_delta : delta;
    /* @hack spelling: the test against delta nested inside the test against
     * SFTIM_MAX, which reads interval again for it (0x8ced89fe, 0x8ced8a04);
     * as one test with &&, SHC reads it once and 21 of 51 words differ */
    if (tim->interval != SFTIM_MAX) {
        if (tim->interval > delta) {
            step = (Uint32)(tim->interval - delta) >> 3;
            if (step != 0) {
                tim->interval -= step;
                return;
            }
        }
    }
    tim->interval = delta;
}

/* the time expected for the next I picture */
Sint32 SFTIM_GetNextItime(struct sftim *tim)
{
    return tim->itime + tim->interval;
}

/* count a vsync in the library's clock and in each playing handle */
void SFTIM_CntupVbIn(void)
{
    struct sftim_clock *clock;
    struct sfd_handle *hn;
    struct sfd_handle *hns;
    struct sftim *tim;
    Sint32 nhn;
    Sint32 stat;
    Sint32 i;

    /* @hack spelling: the clock's address formed from SFLIB_libwork's, which
     * SHC loads and offsets (ADD #104,R4 at 0x8ced8a4a); as
     * &SFLIB_libwork.clock, SHC loads it whole from the pool and 31 of 34 words
     * differ */
    clock = &(&SFLIB_libwork)->clock;
    clock->nvsync++;
    /* @hack spelling: nhn and hn read through SFLIB_libwork's address, which
     * SHC loads from the pool for each (0x8ced8a48, 0x8ced8a58); as
     * SFLIB_libwork.nhn and .hn, 29 of 34 words differ */
    nhn = (&SFLIB_libwork)->nhn;
    hns = (&SFLIB_libwork)->hn;
    for (i = 0; i < nhn; i++) {
        hn = &hns[i];
        /* @hack spelling: the state through stat, which SHC copies into R1
         * (MOV R0,R1 at 0x8ced8a6e), leaving the handle's size to R13; tested
         * as hn->stat, 33 of 34 words differ */
        stat = hn->stat;
        if (stat == SFD_STAT_PLAY) {
            tim = &hn->tim;
            tim->nvsync++;
        }
    }
}

Sint32 SFD_GetTime(struct sfd_handle *hn, Sint32 *time, Sint32 *tunit)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_GETTIME);
    }
    return SFTIM_GetTime(hn, time, tunit);
}

Sint32 SFTIM_GetTime(struct sfd_handle *hn, Sint32 *time, Sint32 *tunit)
{
    struct sftim *tim;

    tim = &hn->tim;
    return tim->timefn(hn, time, tunit);
}

/* the source for no time: SFTIM_NONE once the handle's time has started, and
 * SFTIM_NOT_YET before */
static Sint32 sftim_GetTimeNone(struct sfd_handle *hn, Sint32 *time,
                                Sint32 *tunit)
{
    /* @hack spelling: an empty arm for FALSE, with the stores in the else,
     * which puts SFTIM_NONE in R3 and time in R2 (0x8ced8ad2, 0x8ced8ad4); with
     * the stores under a test for TRUE, the two trade and 6 of 18 words differ
     */
    if (!SFTIM_ChkRegularTime(hn, time, tunit)) {
    } else {
        *time = SFTIM_NONE;
        *tunit = 1;
    }
    return 0;
}

/* the vsync source: 1000 for each vsync counted while the handle played, in
 * the library clock's unit */
static Sint32 sftim_GetTimeVsync(struct sfd_handle *hn, Sint32 *time,
                                 Sint32 *tunit)
{
    struct sftim_clock *clock;
    struct sftim *tim;

    if (SFTIM_ChkRegularTime(hn, time, tunit)) {
        tim = &hn->tim;
        *time = tim->nvsync * 1000;
        /* @hack spelling: the clock's address formed from SFLIB_libwork's,
         * which SHC loads and offsets (ADD #104,R4 at 0x8ced8b0e); as
         * &SFLIB_libwork.clock, SHC loads it whole from the pool and 14 of 29
         * words differ */
        clock = &(&SFLIB_libwork)->clock;
        *tunit = clock->tunit;
    }
    return 0;
}

/* the source for the caller's frames, which gives no time. it stores
 * SFTIM_NOT_YET before the handle's time starts, and nothing after */
static Sint32 sftim_GetTimeUfrm(struct sfd_handle *hn, Sint32 *time,
                                Sint32 *tunit)
{
    /* @hack redundant: tim, set and never read, for which SHC keeps hn in a
     * frame slot across the call and tests its result (MOV.L R4,@R15 at
     * 0x8ced8b26, TST R0,R0 at 0x8ced8b28); without it, 8 of 9 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    struct sftim *tim;

    if (SFTIM_ChkRegularTime(hn, time, tunit)) {
        tim = &hn->tim;
    }
    return 0;
}

/* TRUE while the handle plays, pauses or has finished, whether or not it has
 * failed in that state. otherwise it stores SFTIM_NOT_YET and returns FALSE */
Bool SFTIM_ChkRegularTime(struct sfd_handle *hn, Sint32 *time, Sint32 *tunit)
{
    Sint32 stat;

    stat = hn->stat;
    if (stat != SFD_STAT_PLAY && stat != -SFD_STAT_PLAY &&
        stat != SFD_STAT_PAUSE && stat != -SFD_STAT_PAUSE &&
        stat != SFD_STAT_FIN && stat != -SFD_STAT_FIN) {
        *time = SFTIM_NOT_YET;
        *tunit = 1;
        return FALSE;
    }
    return TRUE;
}

Sint32 SFD_OutUsrFrmSync(struct sfd_handle *hn)
{
    struct sftim *tim;

    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_USRFRM);
    }
    /* @hack spelling: the time state through tim, which has SHC add its
     * offset to hn in R4 (ADD R3,R4 at 0x8ced8ba6) before it counts nufrm;
     * counted through hn, the offsets fold into one and 11 of 24 words
     * differ */
    tim = &hn->tim;
    tim->nufrm++;
    return 0;
}

Sint32 SFD_SetUsrIsSkipFn(struct sfd_handle *hn, sfd_skipfn func)
{
    struct sftim *tim;

    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_USRSKIPFN);
    }
    /* @hack spelling: the time state through tim, which has SHC add its
     * offset to hn in R4 (ADD R3,R4 at 0x8ced8bda) before it stores skipfn;
     * stored through hn, the offsets fold into one and 9 of 24 words differ */
    tim = &hn->tim;
    tim->skipfn = func;
    return 0;
}

/* TRUE when a picture at time1 is due by time2 */
Bool SFD_IsDispTime(Sint32 time1, Sint32 tunit1, Sint32 time2, Sint32 tunit2)
{
    if (UTY_CmpTime(time1, tunit1, time2, tunit2)) {
        return TRUE;
    }
    return FALSE;
}

/* set the caller's time source and switch the handle to it */
Sint32 SFD_SetUsrTimeFn(struct sfd_handle *hn, sfd_timefn func)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_USRTIMEFN);
    }
    SFSET_SetCond(hn, SFD_COND_TIME_SRC, SFTIM_SRC_USER);
    SFTIM_SetTimeFn(hn, func, SFTIM_SRC_USER);
    return 0;
}

void SFTIM_SetTimeFn(struct sfd_handle *hn, sfd_timefn func, Sint32 src)
{
    struct sftim *tim;

    /* @hack spelling: the time state through tim, which SHC forms in R7 (ADD
     * R4,R7 at 0x8ced8c42) and offsets again for srcfn; stored through hn, the
     * offsets fold into one and 7 of 8 words differ */
    tim = &hn->tim;
    tim->srcfn[src] = func;
    SFTIM_SetMasterTimeFn(hn);
}

/* select the source SFD_COND_TIME_SRC names */
void SFTIM_SetMasterTimeFn(struct sfd_handle *hn)
{
    struct sftim *tim;
    Sint32 src;

    /* @hack spelling: the time state through tim, which SHC forms in R5 (ADD
     * R4,R5 at 0x8ced8c52) and stores timefn through (MOV.L R2,@R5 at
     * 0x8ced8c62); through hn, timefn is stored at an offset from hn and 11 of
     * 11 words differ */
    tim = &hn->tim;
    src = hn->cond[SFD_COND_TIME_SRC];
    tim->timefn = tim->srcfn[src];
}

void SFTIM_Tc2Time(struct sftim_tcode *tcode, Sint32 *time, Sint32 *tunit)
{
    sftim_tc2timefn func;
    Sint32 no;
    Sint32 rate;

    no = tcode->rate;
    rate = sftim_prate[no];
    func = sftim_tc2time[no][tcode->drop];
    func(rate, tcode, time, tunit);
}

/* the time code's time, in units of 1/rate second, with rate in frames per 1000
 * seconds */
static void sftim_Tc2TimeN(Sint32 rate, struct sftim_tcode *tcode, Sint32 *time,
                           Sint32 *tunit)
{
    Sint32 hour;
    Sint32 minute;
    Sint32 second;
    Sint32 frame;
    /* @hack redundant: four pointers to the parts, set and never read, which
     * keep the parts in a 16-byte frame (ADD #-16,R15 at 0x8ced8cba); without
     * them, SHC keeps frame in R5, takes 12 bytes of stack, and 32 of 37 words
     * differ */
    Sint32 *hour_at = &hour;
    Sint32 *minute_at = &minute;
    Sint32 *second_at = &second;
    Sint32 *frame_at = &frame;
    Sint32 total;

    hour = tcode->hour;
    minute = tcode->minute;
    second = tcode->second;
    frame = tcode->frame + tcode->nadd;
    total = (hour * rate * 3600) + (minute * rate * 60) + (second * rate) +
            (frame * 1000);
    *time = total;
    *tunit = rate;
}

/* a time code at an NTSC rate converts as at the whole rate above it, then
 * takes its own rate as the unit */
static void sftim_Tc2Time23N(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit)
{
    sftim_Tc2TimeN(24000, tcode, time, tunit);
    *tunit = rate;
}

static void sftim_Tc2Time29N(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit)
{
    sftim_Tc2TimeN(30000, tcode, time, tunit);
    *tunit = rate;
}

static void sftim_Tc2Time59N(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit)
{
    sftim_Tc2TimeN(60000, tcode, time, tunit);
    *tunit = rate;
}

/* with dropped frames, a minute holds 2 frames fewer than 60 seconds do.
 * every tenth minute, from minute 0, holds them all. the time counts each
 * frame as 1000 units of 1/rate second */
static void sftim_Tc2Time23D(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit)
{
    Sint32 hour;
    Sint32 minute;
    Sint32 second;
    Sint32 frame;
    /* @hack redundant: four pointers to the parts, set and never read, which
     * keep the parts in a 16-byte frame (ADD #-16,R15 at 0x8ced8d54); without
     * them, SHC saves R14 for minute, takes 8 bytes of stack, and 46 of 46
     * words differ */
    Sint32 *hour_at = &hour;
    Sint32 *minute_at = &minute;
    Sint32 *second_at = &second;
    Sint32 *frame_at = &frame;
    Sint32 total;

    hour = tcode->hour;
    minute = tcode->minute;
    second = tcode->second;
    frame = tcode->frame + tcode->nadd;
    total = ((hour * 86292) + (minute * 1438) + (minute / 10 * 2) +
             (second * 24) + frame) *
            1000;
    *time = total;
    *tunit = rate;
}

static void sftim_Tc2Time29D(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit)
{
    Sint32 hour;
    Sint32 minute;
    Sint32 second;
    Sint32 frame;
    /* @hack redundant: four pointers to the parts, set and never read, which
     * keep the parts in a 16-byte frame (ADD #-16,R15 at 0x8ced8db0); without
     * them, SHC saves R14, takes 8 bytes of stack, and 43 of 44 words
     * differ */
    Sint32 *hour_at = &hour;
    Sint32 *minute_at = &minute;
    Sint32 *second_at = &second;
    Sint32 *frame_at = &frame;
    Sint32 total;

    hour = tcode->hour;
    minute = tcode->minute;
    second = tcode->second;
    frame = tcode->frame + tcode->nadd;
    total = ((hour * 107892) + (minute * 1798) + (minute / 10 * 2) +
             (second * 30) + frame) *
            1000;
    *time = total;
    *tunit = rate;
}

static void sftim_Tc2Time59D(Sint32 rate, struct sftim_tcode *tcode,
                             Sint32 *time, Sint32 *tunit)
{
    Sint32 hour;
    Sint32 minute;
    Sint32 second;
    Sint32 frame;
    /* @hack redundant: four pointers to the parts, set and never read, which
     * keep the parts in a 16-byte frame (ADD #-16,R15 at 0x8ced8e2e); without
     * them, SHC saves R14, takes 8 bytes of stack, and 43 of 44 words
     * differ */
    Sint32 *hour_at = &hour;
    Sint32 *minute_at = &minute;
    Sint32 *second_at = &second;
    Sint32 *frame_at = &frame;
    Sint32 total;

    hour = tcode->hour;
    minute = tcode->minute;
    second = tcode->second;
    frame = tcode->frame + tcode->nadd;
    total = ((hour * 215892) + (minute * 3598) + (minute / 10 * 2) +
             (second * 60) + frame) *
            1000;
    *time = total;
    *tunit = rate;
}
