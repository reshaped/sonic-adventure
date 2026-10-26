/* Chaos 0's stage, the fight in the rain at Station Square: the stage's
 * task, its sky's task, and the boss music's wait */
#include <ninja.h>

#include "b_chaos0/boss.h"
#include "b_chaos0/chaos0.h"
#include "b_chaos0/rain.h"
#include "chaos/effect.h"
#include "chaos/start.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"

enum chaos0_mode { CHAOS0_START, CHAOS0_FIGHT, CHAOS0_END };

enum sky_mode { SKY_START, SKY_RUN, SKY_RESET };

/* a sound the stage starts with the fight, which plays until it is
 * stopped */
#define SE_HELD 0x3ee

#define SKY_BACK 0xff000000

static void chaos0_start(struct task *task);
static void chaos0_end(struct task *task);

/* in the HEAD section, at the module's first address, 0x0c900000, which
 * 1ST_READ.BIN's stage_steps names */
#pragma section HEAD
void chaos0_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CHAOS0_START:
        chaos0_start(task);
        break;
    case CHAOS0_FIGHT:
        break;
    case CHAOS0_END:
        chaos0_end(task);
        break;
    }
}
#pragma section

void chaos0_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    --work->timer;
    if (work->timer < 0) {
        stream_reset();
        stream_request_and_save_music(work->mode);
        task_destroy(task);
    }
}

static void chaos0_start(struct task *task)
{
    struct task_work *work = task->work;

    stage_get_stage_act();
    chaos_start();
    chaos0_create();
    chaos_effect_init();
    chaos0_rain_start();
    work->mode = CHAOS0_FIGHT;
    sound_play_held(SE_HELD, -1, 8, 0);
}

static void chaos0_end(struct task *task)
{
    stream_destroy();
}

/* in the HEAD section, at 0x0c900060, which 1ST_READ.BIN's sky_steps
 * names. it draws no sky, only the black back color behind the land */
#pragma section HEAD
void chaos0_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SKY_START:
        njSetBackColor(SKY_BACK, SKY_BACK, SKY_BACK);
        work->mode = SKY_RUN;
    case SKY_RUN:
        njFogDisable();
        njPushMatrix(NULL);
        njPopMatrix(1);
        njFogEnable();
        break;
    case SKY_RESET:
        work->mode = SKY_START;
        break;
    default:
        /* @bug memory: any other mode hands the task itself to heap_free (JMP
         * at 0x0c9000b2). a task is a slot of the pool task_init takes from the
         * heap, not a block heap_alloc gave */
        heap_free(task);
        break;
    }
}
#pragma section
