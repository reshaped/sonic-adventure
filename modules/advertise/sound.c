/* the menus' sound: the effects of their sound bank, and the music and voices,
 * which wait for a screen's textures */
#include <sg_xpt.h>

#include "advertise/sound.h"
#include "advertise/textures.h"
#include "object/common.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"

/* each row of summary_voices holds SUMMARY_LINES voices. enum summary_line
 * names the first seven lines, and no call plays the last two */
#define SUMMARY_LINES 9

/* the work of a task that waits to request music or a voice */
struct wait_work {
    int number; /* the music or voice it requests */
    int steps; /* its steps so far */
};

/* each advertise_tune's music */
static const int music_tracks[] = {
    MUSIC_TITLE2, MUSIC_TITLE, MUSIC_OPTION, MUSIC_CHARACTR, MUSIC_TRIAL,
};
/* the voice of each wheel story, as enum wheel_story numbers them */
static const int voice_numbers[] = {719, 677, 660, 594, 612, 639, 403};
/* summary_voices[file][line] is the voice of each summary_line, for each voice
 * file menu_state.summary numbers. row 0 is for no voice file */
static const int summary_voices[9][SUMMARY_LINES] = {
#include "advertise/sound/summary_voices.inc"
};

/* the tasks whose music and voice wait, or NULL */
static struct task *music_task;
static struct task *voice_task;

void se_open(void)
{
    sound_play(SE_MENU_OPEN, 0, 0, 0);
}

void se_close(void)
{
    sound_play(SE_MENU_CLOSE, 0, 0, 0);
}

void se_move(void)
{
    sound_play(SE_MOVE, 0, 0, 0);
}

void se_choose(void)
{
    sound_play(SE_CHOOSE, 0, 0, 0);
}

void se_cancel(void)
{
    sound_play(SE_CANCEL, 0, 0, 0);
}

void advertise_sound_init(void)
{
    sound_load_file("mobile1.mlt");
    music_task = voice_task = NULL;
}

void advertise_sound_stop(void)
{
    if (voice_task) {
        task_destroy(voice_task);
        voice_task = NULL;
    }
    if (music_task) {
        task_destroy(music_task);
        music_task = NULL;
    }
    stream_destroy();
}

void advertise_music_stop(void)
{
    stream_stop_music();
}

int advertise_music_state(void)
{
    return stream_music_state();
}

static void music_wait(struct task *task)
{
    struct wait_work *work = task->extra;

    if (work->steps++ > 0 && textures_ready) {
        stream_request_music(work->number);
        task_destroy(task);
        music_task = NULL;
    }
}

void advertise_music_start(int track)
{
    struct task *task;
    struct wait_work *work;

    advertise_sound_stop();
    stream_reset();
    /* @hack spelling: the task assigned in the test, after the other two tests.
     * SHC then tests it in R0 (TST R0,R0 at 0x8c901928), and keeps it on the
     * stack (MOV.L R0,@R15 at 0x8c90192a). assigned in a block of its own, it
     * is kept in R14, which the function then saves, and 35 of 39 words
     * differ */
    if (track >= 0 && !music_task &&
        (music_task = task = task_create(0, 4, music_wait))) {
        work = heap_calloc(1, sizeof(struct wait_work));
        task->extra = work;
        work->number = track;
        work->steps = 0;
    }
}

void advertise_music_play(enum advertise_tune tune)
{
    advertise_music_start(music_tracks[tune]);
}

static void voice_wait(struct task *task)
{
    struct wait_work *work = task->extra;

    if (work->steps++ > 0 && textures_ready) {
        stream_request_voice(work->number);
        task_destroy(task);
        voice_task = NULL;
    }
}

/* TRUE once a task waits to request voice number. FALSE, requesting nothing,
 * while another voice waits, or when task_create fails */
static Bool voice_start(int number)
{
    struct task *task;
    struct wait_work *work;

    /* @hack spelling: the task assigned in the test, after the test of
     * voice_task. SHC then tests it in R0 (TST R0,R0 at 0x8c9019ac), and keeps
     * it on the stack (MOV.L R0,@R15 at 0x8c9019ae). assigned in a block of its
     * own, it is kept in R14, which the function then saves, the function runs
     * 4 bytes longer, and 37 of 37 words differ */
    if (!voice_task && (voice_task = task = task_create(0, 4, voice_wait))) {
        work = heap_calloc(1, sizeof(struct wait_work));
        task->extra = work;
        work->number = number;
        work->steps = 0;
        return TRUE;
    }
    return FALSE;
}

Bool advertise_voice_summary(enum summary_line line)
{
    return voice_start(summary_voices[menu_state.summary[0]][line]);
}

Bool advertise_voice_play(int voice)
{
    return voice_start(voice_numbers[voice]);
}
