/* streams: the music and the voices, played from the disc */
#ifndef SOUND_STREAM_H
#define SOUND_STREAM_H
#include <sg_xpt.h>

struct task;

/* each track, named for its file on the disc */
enum music {
    MUSIC_ADVAMY = 0,
    MUSIC_ADVBIG,
    MUSIC_ADVE102,
    MUSIC_ADVKNKLS,
    MUSIC_ADVMILES,
    MUSIC_ADVSONIC,
    MUSIC_AMY,
    MUSIC_BIG,
    MUSIC_BOSSALL,
    MUSIC_BOSSE101,
    MUSIC_BOSSEVNT,
    MUSIC_BOSSTRGT,
    MUSIC_CASINO1,
    MUSIC_CASINO2,
    MUSIC_CASINO3,
    MUSIC_CASINO4,
    MUSIC_CHAO,
    MUSIC_CHAOGOAL,
    MUSIC_CHAOHALL,
    MUSIC_CHAORACE,
    MUSIC_CHAOS,
    MUSIC_CHAOS_6,
    MUSIC_CHAOS_P1,
    MUSIC_CHAOS_P2,
    MUSIC_CHARACTR,
    MUSIC_CIRCUIT,
    MUSIC_CONTINUE,
    MUSIC_E102,
    MUSIC_ECOAST1,
    MUSIC_ECOAST2,
    MUSIC_ECOAST3,
    MUSIC_EGCARER1,
    MUSIC_EGCARER2,
    MUSIC_EGGMAN,
    MUSIC_EGGMBL23,
    MUSIC_EGGROBO,
    MUSIC_EVTBGM00,
    MUSIC_EVTBGM01,
    MUSIC_EVTBGM02,
    MUSIC_EVTBGM03,
    MUSIC_EVTBGM04,
    MUSIC_EVTBGM05,
    MUSIC_FINALEG1,
    MUSIC_FINALEG2,
    MUSIC_FISHGET,
    MUSIC_FISHING,
    MUSIC_FISHMISS,
    MUSIC_HAMMER,
    MUSIC_HIGHWAY1,
    MUSIC_HIGHWAY2,
    MUSIC_HIGHWAY3,
    MUSIC_HURRYUP,
    MUSIC_ICECAP1,
    MUSIC_ICECAP2,
    MUSIC_ICECAP3,
    MUSIC_INVNCIBL,
    MUSIC_ITEM1,
    MUSIC_JINGLE_1,
    MUSIC_JINGLE_2,
    MUSIC_JINGLE_3,
    MUSIC_JINGLE_4,
    MUSIC_JINGLE_5,
    MUSIC_KNUCKLES,
    MUSIC_LSTWRLD1,
    MUSIC_LSTWRLD2,
    MUSIC_LSTWRLD3,
    MUSIC_MAINTHEM,
    MUSIC_MSTCLN,
    MUSIC_NIGHTS_A,
    MUSIC_NIGHTS_K,
    MUSIC_NIGHTS_S,
    MUSIC_ONE_UP,
    MUSIC_OPTION,
    MUSIC_REDMNTN1,
    MUSIC_REDMNTN2,
    MUSIC_RNDCLEAR,
    MUSIC_S_SQUARE,
    MUSIC_SANDHILL,
    MUSIC_SCRAMBLE,
    MUSIC_SHELTER1,
    MUSIC_SHELTER2,
    MUSIC_SKYDECK1,
    MUSIC_SKYDECK2,
    MUSIC_SONIC,
    MUSIC_SONIC_CD,
    MUSIC_SPEEDUP,
    MUSIC_SPRSONIC,
    MUSIC_SSRACING,
    MUSIC_TAILS,
    MUSIC_THEAMY,
    MUSIC_THEBIG,
    MUSIC_THEE102,
    MUSIC_THEKNKLS,
    MUSIC_THEMILES,
    MUSIC_THESONIC,
    MUSIC_TICAL,
    MUSIC_TIMER,
    MUSIC_TITL_EGG,
    MUSIC_TITL_MR1,
    MUSIC_TITL_MR2,
    MUSIC_TITL_SS,
    MUSIC_TITLE,
    MUSIC_TITLE2,
    MUSIC_TRIAL,
    MUSIC_TWNKLPK1,
    MUSIC_TWNKLPK2,
    MUSIC_TWNKLPK3,
    MUSIC_WNDYVLY1,
    MUSIC_WNDYVLY2,
    MUSIC_WNDYVLY3,
    MUSIC_COUNT
};

/* the first step of the streams' task: load the file table of the voices'
 * archive, and create both streams. one such task runs at a time, and a
 * second destroys itself */
void stream_init_step(struct task *task);
/* destroy the streams' task, if one runs. its destructor destroys both
 * streams, then calls stream_reset_music and stream_reset_voice */
void stream_destroy(void);
/* ask for music on the streams' next step, replacing any request, and
 * keep it as the music asked for last. music must be below MUSIC_COUNT,
 * and nothing checks it. a negative one starts nothing */
void stream_request_music(int music);
/* stop the music playing, if the stream exists, and forget it for
 * stream_restart_music. a pending request still starts; stream_reset_music
 * drops it */
void stream_stop_music(void);
/* the music's ADXT_STAT, as the streams' last step read it */
Sint32 stream_music_state(void);
/* the samples the music has played, or 0 without the music stream */
int stream_music_time(void);
/* ask again for the music asked for last. TRUE if there is one, else
 * FALSE */
Bool stream_restart_music(void);
/* ask for voice on the streams' next step, replacing any request. voice
 * must index a file in the voices' archive; a negative one starts nothing */
void stream_request_voice(int voice);
/* stop the voice playing. a pending request still starts */
void stream_stop_voice(void);
/* the voice's ADXT_STAT, as the streams' last step read it */
Sint32 stream_voice_state(void);
/* the samples the voice has played, or 0 without the voice stream */
int stream_voice_time(void);
/* stream_mute silences both streams, and stream_unmute gives each its
 * volume back */
void stream_mute(void);
void stream_unmute(void);
/* play music as an interlude, and leave the music asked for last as it
 * is. once the music stream stops or plays to its end, that music is asked
 * for again. nothing asks again if a request, a stop or a reset changes it
 * first, or without the streams' task. when the interlude's task can't be
 * created, it reads through NULL */
void stream_request_interlude(int music);
/* stream_reset_music forgets the pending music and the music asked for
 * last, and stream_reset_voice forgets the pending voice. what plays goes
 * on. the state reads ADXT_STAT_STOP until the streams' next step reads it
 * again, which it does only while the stream exists */
void stream_reset_music(void);
void stream_reset_voice(void);
/* forget every request, and start the streams' task unless one already runs */
void stream_reset(void);
/* ask for music, and keep it for stream_restore_music */
void stream_request_and_save_music(int music);
/* ask again for the music stream_request_and_save_music kept. TRUE if it
 * has kept one, else FALSE */
Bool stream_restore_music(void);

#endif
