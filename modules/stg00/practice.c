/* the stage task of the Hedgehog Hammer, STAGE_PRACTICE. it sets up the act
 * the stage starts in, and each act it changes to */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/light.h"
#include "hud/hud.h"
#include "player/player.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg00/practice.h"
#include "system/task.h"

/* the act the task last set up, kept in the work's timer */
#define PRACTICE_ACT(work) TASK_TIMER(work)

/* the stage task's modes, in work->mode */
enum practice_mode {
    PRACTICE_INIT,
    PRACTICE_START,
    PRACTICE_RUN,
    PRACTICE_END
};

/* the stage and act whose light sources and palettes each act reads from
 * the disc */
#define LIGHT_STAGE STAGE_EGG_CARRIER_INSIDE
#define LIGHT_ACT 2

/* the steps the music task waits. it asks for MUSIC_HAMMER on the step after
 * them */
#define MUSIC_DELAY 10

static void practice_act_start(struct task *task);
static void practice_destroy(struct task *task);
static void practice_music_wait_step(struct task *task);

/* the step of the act in progress, which does nothing */
static void practice_act_step(struct task *task)
{
    /* @hack redundant: a read of the work, which keeps the task passed in, MOV
     * R13,R4 at 0x0c900054, where SHC expands the step in practice_step;
     * without it, 34 of 49 words differ */
    (void)task->work;
}

/* under section HEAD, SHC writes this step to section PHEAD, which the module's
 * link places first. 1ST_READ.BIN's stage_steps holds its address there */
#pragma section HEAD
void practice_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PRACTICE_INIT:
        PRACTICE_ACT(work) = (Uint8)stage_get_stage_act();
        task->destructor = practice_destroy;
        work->mode = PRACTICE_START;
        break;
    case PRACTICE_START:
        practice_act_start(task);
        work->mode = PRACTICE_RUN;
        break;
    case PRACTICE_RUN:
        if ((Uint8)stage_get_stage_act() != PRACTICE_ACT(work)) {
            work->mode = PRACTICE_END;
        } else {
            practice_act_step(task);
        }
        break;
    case PRACTICE_END:
        work->mode = PRACTICE_START;
        break;
    }
}
#pragma section

/* set up the act. player 0 stands again with no speed, and the back color
 * turns black. a task asks for MUSIC_HAMMER after MUSIC_DELAY steps.
 * practice_act_start also reads the lights of LIGHT_STAGE's act LIGHT_ACT,
 * resets the lives and allows pausing */
static void practice_act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;

    PRACTICE_ACT(work) = (Uint8)stage_get_stage_act();
    player_set_mode(0, PLAYER_MODE_RESET);
    player_set_speed(0, 0.0F, 0.0F, 0.0F);
    njSetBackColor(0, 0, 0);
    music = task_create(TASK_WORK, 0, practice_music_wait_step);
    music->work->mode = MUSIC_HAMMER;
    music->work->timer = MUSIC_DELAY;
    light_read_disc_sources(LIGHT_STAGE, LIGHT_ACT);
    light_read_disc_palettes(LIGHT_STAGE, LIGHT_ACT);
    lives_reset();
    pause_enable();
}

static void practice_destroy(struct task *task)
{
}

/* count down work->timer. once it falls below 0, start the streams' task
 * and ask for the music work->mode names. the task then destroys itself */
static void practice_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    work->timer--;
    if (work->timer < 0) {
        task_create(0, 1, stream_init_step);
        stream_request_music(work->mode);
        task_destroy(task);
    }
}
