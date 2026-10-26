/* the menus' sound: the effects of their sound bank, and the music and voices,
 * which wait for a screen's textures */
#ifndef ADVERTISE_SOUND_H
#define ADVERTISE_SOUND_H

#include <sg_xpt.h>

/* play the menus' effects for a menu's opening and closing, a move of the
 * cursor, a choice and a cancel. nothing calls se_open or se_close */
void se_open(void);
void se_close(void);
void se_move(void);
void se_choose(void);
void se_cancel(void);
/* load the menus' sound bank, mobile1.mlt, and forget any music or voice
 * waiting */
void advertise_sound_init(void);
/* cancel any music or voice still waiting, and destroy the streams */
void advertise_sound_stop(void);
/* the menus' own tunes, as advertise_music_play takes them, and the screens
 * that play them */
enum advertise_tune {
    TUNE_TITLE2, /* the card screen */
    TUNE_TITLE, /* mode select */
    TUNE_OPTION, /* the options, drive and file screens */
    TUNE_CHARACTER, /* the wheel, for the adventure */
    TUNE_TRIAL /* the wheel in the trial, and the trial's lists */
};

/* the voice lines advertise_voice_summary plays, each for the screen that plays
 * it as it opens */
enum summary_line {
    LINE_DRIVE,
    LINE_FILE,
    LINE_CHARACTER,
    LINE_TRIAL_STAGE,
    LINE_MINI_GAME,
    LINE_OPTIONS,
    LINE_SOUND_TEST
};

/* advertise_music_start stops the music and voices, as advertise_sound_stop
 * does, and starts the streams again as stream_reset does. a task then waits a
 * step, and then until textures_ready is TRUE, and requests track, as
 * stream_request_music does. a negative track starts nothing.
 * advertise_music_play starts tune's track. advertise_music_stop stops the
 * music as stream_stop_music does */
void advertise_music_start(int track);
void advertise_music_play(enum advertise_tune tune);
void advertise_music_stop(void);
/* the music's ADXT_STAT, as stream_music_state gives it */
int advertise_music_state(void);
/* advertise_voice_summary plays line from the voice file menu_state.summary
 * picks, and advertise_voice_play the voice of a wheel story, 0 to 6. a task
 * waits a step, and then until textures_ready is TRUE, and requests the voice,
 * as stream_request_voice does. each returns TRUE once the voice waits. it
 * returns FALSE, playing nothing, while another voice waits, or when
 * task_create fails */
Bool advertise_voice_summary(enum summary_line line);
Bool advertise_voice_play(int voice);

#endif
