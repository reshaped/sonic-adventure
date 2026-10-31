/* streams: the music and the voices, played from the disc */
#include <sg_xpt.h>
#include <sofdec.h>

#include "sound/stream.h"
#include "system/task.h"

/* the voices' archive, the partition adx loads it as, and the most files
 * ADXF_LoadPartition reads from it */
#define VOICE_ARCHIVE "event_adx.afs"
#define VOICE_PARTITION 0
#define VOICE_FILES 2148

#define MUSIC_CHANNELS 2
#define VOICE_CHANNELS 1

/* each stream's work, music first, at a fixed place in main memory.
 * ADXT_Create lays a work out as an output buffer for each channel, then
 * whole 2048-byte sectors of input. the music's holds 74 of them, and the
 * voices' 19 */
#define STREAM_WORK 0x0cca8000
#define MUSIC_WORK_SIZE 0x2d0c4
#define VOICE_WORK_SIZE 0xd884

/* each stream's volume, in tenths of a decibel, and the one that silences
 * it */
#define MUSIC_VOLUME (-10)
#define VOICE_VOLUME 0
#define MUTED (-960)

/* each stream's server is told to expect this many calls a second */
#define SERVER_FREQ 20

enum interlude_mode {
    INTERLUDE_INIT,
    INTERLUDE_WAIT /* for the interlude to end */
};

struct interlude_vars {
    Sint32 music; /* the music to go back to */
    Uint8 unk_04[4];
    Sint32 interlude; /* the interlude's music */
};

#define VARS(work) ((struct interlude_vars *)&(work)->vars)

static void stream_step(struct task *task);
static void stream_close(struct task *task);
static void interlude_step(struct task *task);

static char *music_files[MUSIC_COUNT] = {
    "ADVAMY.ADX",   "ADVBIG.ADX",   "ADVE102.ADX",  "ADVKNKLS.ADX",
    "ADVMILES.ADX", "ADVSONIC.ADX", "AMY.ADX",      "BIG.ADX",
    "BOSSALL.ADX",  "BOSSE101.ADX", "BOSSEVNT.ADX", "BOSSTRGT.ADX",
    "CASINO1.ADX",  "CASINO2.ADX",  "CASINO3.ADX",  "CASINO4.ADX",
    "CHAO.ADX",     "CHAOGOAL.ADX", "CHAOHALL.ADX", "CHAORACE.ADX",
    "CHAOS.ADX",    "CHAOS_6.ADX",  "CHAOS_P1.ADX", "CHAOS_P2.ADX",
    "CHARACTR.ADX", "CIRCUIT.ADX",  "CONTINUE.ADX", "E102.ADX",
    "ECOAST1.ADX",  "ECOAST2.ADX",  "ECOAST3.ADX",  "EGCARER1.ADX",
    "EGCARER2.ADX", "EGGMAN.ADX",   "EGGMBL23.ADX", "EGGROBO.ADX",
    "EVTBGM00.ADX", "EVTBGM01.ADX", "EVTBGM02.ADX", "EVTBGM03.ADX",
    "EVTBGM04.ADX", "EVTBGM05.ADX", "FINALEG1.ADX", "FINALEG2.ADX",
    "FISHGET.ADX",  "FISHING.ADX",  "FISHMISS.ADX", "HAMMER.ADX",
    "HIGHWAY1.ADX", "HIGHWAY2.ADX", "HIGHWAY3.ADX", "HURRYUP.ADX",
    "ICECAP1.ADX",  "ICECAP2.ADX",  "ICECAP3.ADX",  "INVNCIBL.ADX",
    "ITEM1.ADX",    "JINGLE_1.ADX", "JINGLE_2.ADX", "JINGLE_3.ADX",
    "JINGLE_4.ADX", "JINGLE_5.ADX", "KNUCKLES.ADX", "LSTWRLD1.ADX",
    "LSTWRLD2.ADX", "LSTWRLD3.ADX", "MAINTHEM.ADX", "MSTCLN.ADX",
    "NIGHTS_A.ADX", "NIGHTS_K.ADX", "NIGHTS_S.ADX", "ONE_UP.ADX",
    "OPTION.ADX",   "REDMNTN1.ADX", "REDMNTN2.ADX", "RNDCLEAR.ADX",
    "S_SQUARE.ADX", "SANDHILL.ADX", "SCRAMBLE.ADX", "SHELTER1.ADX",
    "SHELTER2.ADX", "SKYDECK1.ADX", "SKYDECK2.ADX", "SONIC.ADX",
    "SONIC_CD.ADX", "SPEEDUP.ADX",  "SPRSONIC.ADX", "SSRACING.ADX",
    "TAILS.ADX",    "THEAMY.ADX",   "THEBIG.ADX",   "THEE102.ADX",
    "THEKNKLS.ADX", "THEMILES.ADX", "THESONIC.ADX", "TICAL.ADX",
    "TIMER.ADX",    "TITL_EGG.ADX", "TITL_MR1.ADX", "TITL_MR2.ADX",
    "TITL_SS.ADX",  "TITLE.ADX",    "TITLE2.ADX",   "TRIAL.ADX",
    "TWNKLPK1.ADX", "TWNKLPK2.ADX", "TWNKLPK3.ADX", "WNDYVLY1.ADX",
    "WNDYVLY2.ADX", "WNDYVLY3.ADX",
};

#ifdef GCC
/* nothing writes these, so GCC would read each as TRUE. it keeps them as
 * used, as the image keeps them, for a probe of the running game to turn
 * the streams off by */
static Bool music_enabled __attribute__((used));
static Bool voice_enabled __attribute__((used));
#endif
static Bool music_enabled = TRUE;
static Bool voice_enabled = TRUE;
static void *music_work = (void *)STREAM_WORK;
static void *voice_work = (void *)(STREAM_WORK + MUSIC_WORK_SIZE);
static ADXT music_stream = NULL;
static ADXT voice_stream = NULL;
/* each ADXT_STAT value's name, padded to seven characters */
static char *stream_state_names[] = {
    "STOP   ", "DECINFO", "PREP   ", "PLAYING", "DECEND ", "PLAYEND",
};
static int dat_8c612510[4] = {0};
static int voice_request = -1; /* the voice to start next, or -1 */
static Sint32 voice_state = ADXT_STAT_STOP;
static int music_request = -1; /* the music to start next, or -1 */
static Sint32 music_state = ADXT_STAT_STOP;
static int music_current = -1; /* the music asked for last, or -1 */
static int dat_8c612534 = -1;
/* what stream_request_and_save_music asked for last */
static int music_saved = -1;
/* the one task that runs the streams, or NULL */
static struct task *stream_task = NULL;

/* the file table of the voices' archive, as ADXF_LoadPartition reads it.
 * its 0x1138 bytes have room for 2192 files, more than VOICE_FILES */
static Uint8 voice_ptinfo[0x1138];
/* the size of both streams' work */
static Sint32 stream_work_size;

static void stream_ignore_error(void *obj, char *msg)
{
}

void stream_init_step(struct task *task)
{
    if (stream_task == NULL) {
        stream_task = task;
        stream_work_size = MUSIC_WORK_SIZE + VOICE_WORK_SIZE;
        ADXT_EntryErrFunc(stream_ignore_error, NULL);
        ADXF_LoadPartition(VOICE_PARTITION, VOICE_ARCHIVE, voice_ptinfo,
                           VOICE_FILES);
        music_stream = ADXT_Create(MUSIC_CHANNELS, music_work, MUSIC_WORK_SIZE);
        voice_stream = ADXT_Create(VOICE_CHANNELS, voice_work, VOICE_WORK_SIZE);
        if (music_stream != NULL) {
            ADXT_SetAutoRcvr(music_stream, ADXT_RMODE_REPLAY);
            ADXT_SetOutVol(music_stream, MUSIC_VOLUME);
            ADXT_SetSvrFreq(music_stream, SERVER_FREQ);
        }
        if (voice_stream != NULL) {
            ADXT_SetAutoRcvr(voice_stream, ADXT_RMODE_STOP);
            ADXT_SetOutVol(voice_stream, VOICE_VOLUME);
            ADXT_SetSvrFreq(voice_stream, SERVER_FREQ);
        }
        task->destructor = stream_close;
        task->step = stream_step;
    } else {
        task_destroy(task);
    }
}

static void stream_step(struct task *task)
{
    /* @hack redundant: 256 bytes of frame that no code reads, of a type
     * unknown, declared as a char[256]. the image's 260-byte frame holds them
     * (ADD R0,R15 at 0x8c11671e, ADD R1,R15 at 0x8c116788); without them, 59 of
     * 61 words differ */
    char unused[256];

#ifdef GCC
    /* a skip starts no stream behind its curtain, so no voice takes time */
    if (event_skipping) {
        music_request = voice_request = -1;
    }
#endif
    if (music_stream != NULL) {
        if (music_request >= 0) {
            ADXT_StartFname(music_stream, music_files[music_request]);
        }
        music_request = -1;
        music_state = ADXT_GetStat(music_stream);
    }
    if (voice_stream != NULL) {
        if (voice_request >= 0) {
            ADXT_StartAfs(voice_stream, VOICE_PARTITION, voice_request);
        }
        voice_request = -1;
        voice_state = ADXT_GetStat(voice_stream);
    }
    if (adxpd_internal_error) {
        adxpd_internal_error = 0;
    }
    task_step_children(task);
}

static void stream_close(struct task *task)
{
    if (music_stream != NULL) {
        ADXT_Destroy(music_stream);
        music_stream = NULL;
    }
    if (voice_stream != NULL) {
        ADXT_Destroy(voice_stream);
        voice_stream = NULL;
    }
    stream_reset_music();
    stream_reset_voice();
    stream_task = NULL;
}

void stream_destroy(void)
{
    if (stream_task != NULL) {
        task_destroy(stream_task);
        stream_task = NULL;
    }
}

void stream_request_music(int music)
{
    if (music_enabled) {
        music_request = music;
        music_current = music;
    }
}

void stream_stop_music(void)
{
    if (music_stream != NULL) {
        ADXT_Stop(music_stream);
        music_current = -1;
    }
}

Sint32 stream_music_state(void)
{
    return music_state;
}

int stream_music_time(void)
{
    if (music_stream != NULL) {
        Sint32 count;
        Sint32 scale;

        ADXT_GetTime(music_stream, &count, &scale);
        return count;
    }
    return 0;
}

Bool stream_restart_music(void)
{
    if (music_current != -1) {
        stream_request_music(music_current);
        return TRUE;
    }
    return FALSE;
}

void stream_request_voice(int voice)
{
    if (voice_enabled) {
        voice_request = voice;
    }
}

void stream_stop_voice(void)
{
    if (voice_stream != NULL) {
        ADXT_Stop(voice_stream);
    }
}

Sint32 stream_voice_state(void)
{
    return voice_state;
}

int stream_voice_time(void)
{
    if (voice_stream != NULL) {
        Sint32 count;
        Sint32 scale;

        ADXT_GetTime(voice_stream, &count, &scale);
        return count;
    }
    return 0;
}

void stream_mute(void)
{
    if (music_stream != NULL) {
        ADXT_SetOutVol(music_stream, MUTED);
    }
    if (voice_stream != NULL) {
        ADXT_SetOutVol(voice_stream, MUTED);
    }
}

void stream_unmute(void)
{
    if (music_stream != NULL) {
        ADXT_SetOutVol(music_stream, MUSIC_VOLUME);
    }
    if (voice_stream != NULL) {
        ADXT_SetOutVol(voice_stream, VOICE_VOLUME);
    }
}

void stream_request_interlude(int music)
{
    struct task *task;

    if (music_enabled) {
        music_request = music;
        if (stream_task != NULL) {
            task = task_create_child(TASK_WORK, interlude_step, stream_task);
            VARS(task->work)->interlude = music;
            VARS(task->work)->music = music_current;
        }
    }
}

static void interlude_destroy(struct task *task)
{
}

static void interlude_step(struct task *task)
{
    struct task_work *work;

    work = task->work;
    switch (work->mode) {
    case INTERLUDE_INIT:
        task->destructor = interlude_destroy;
        work->mode = INTERLUDE_WAIT;
        break;
    case INTERLUDE_WAIT:
        if (music_current != VARS(work)->music) {
            task_kill(task);
        } else {
            switch (stream_music_state()) {
            case ADXT_STAT_STOP:
            case ADXT_STAT_PLAYEND:
                stream_request_music(music_current);
                task_kill(task);
                break;
            }
        }
        break;
    }
}

void stream_reset_music(void)
{
    music_request = -1;
    music_state = ADXT_STAT_STOP;
    music_current = -1;
}

void stream_reset_voice(void)
{
    voice_request = -1;
    voice_state = ADXT_STAT_STOP;
}

void stream_reset(void)
{
    /* @hack redundant: task, set to task_create's result and never read,
     * which keeps the call a JSR that returns here (0x8c116b40). with the
     * result dropped, SHC makes the call a jump, and 16 of 21 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    struct task *task;

    stream_reset_music();
    stream_reset_voice();
    task = task_create(0, 1, stream_init_step);
}

void stream_request_and_save_music(int music)
{
    if (music_enabled) {
        music_request = music;
        music_saved = music;
        music_current = music;
    }
}

Bool stream_restore_music(void)
{
    if (music_saved != -1) {
        stream_request_and_save_music(music_saved);
        return TRUE;
    }
    return FALSE;
}
