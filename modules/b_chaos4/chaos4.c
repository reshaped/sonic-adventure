/* Chaos 4's stage. its task starts the fight, and its sky's task sets the
 * colors behind the land */
#include <ninja.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/chaos4.h"
#include "b_chaos4/column.h"
#include "b_chaos4/util.h"
#include "chaos/effect.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"

#define TEXT_1_LENGTH 7
#include "b_chaos4/chaos4/text.inc"

/* the stage task's modes. nothing in the module sets CHAOS4_END, which
 * stops the stream */
enum chaos4_mode { CHAOS4_WAIT, CHAOS4_START, CHAOS4_FIGHT, CHAOS4_END };

enum sky_mode { SKY_START, SKY_RUN, SKY_RESET };

/* the steps the music's task counts down before the music starts */
#define MUSIC_DELAY 5

/* the three colors njSetBackColor puts behind the land: dark blue, violet
 * and a dull pink */
#define SKY_BACK_1 0x1040
#define SKY_BACK_2 0x401074
#define SKY_BACK_3 0x807474

static void chaos4_start(struct task *task);
static void chaos4_end(struct task *task);

/* the stage's task. it lies at the head of the module, where
 * 1ST_READ.BIN's stage_steps names it */
#pragma section HEAD
void chaos4_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CHAOS4_WAIT:
        work->mode = CHAOS4_START;
        break;
    case CHAOS4_START:
        chaos4_start(task);
        break;
    case CHAOS4_FIGHT:
        break;
    case CHAOS4_END:
        chaos4_end(task);
        break;
    }
}
#pragma section

/* the music's task. it counts work->timer down past 0, then asks for
 * track work->mode and destroys itself */
static void chaos4_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    --work->timer;
    if (work->timer < 0) {
        stream_reset();
        stream_request_music(work->mode);
        task_destroy(task);
    }
}

/* start the fight, a step after the stage's task starts: the boss, his
 * effects, his name and the music */
static void chaos4_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;

    stage_get_stage_act();
    time_stop();
    effect_counts_clear();
    chaos4_create();
    chaos_effect_init();
    chaos4_sprites_init();
    boss_name_create(TEXT_1, -1, 240, 120);
    work->mode = CHAOS4_FIGHT;
    music = task_create(TASK_WORK, 0, chaos4_music_wait_step);
    music->work->mode = MUSIC_CHAOS;
    music->work->timer = MUSIC_DELAY;
}

static void chaos4_end(struct task *task)
{
    stream_destroy();
}

#pragma section HEAD
/* the sky's task, which draws no sky. it sets the colors behind the land
 * once, and then each step turns the fog off round an empty push and pop
 * of the matrix. SKY_RESET starts it over. a mode past SKY_RESET frees the
 * task with heap_free, not task_destroy */
void chaos4_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SKY_START:
        njSetBackColor(SKY_BACK_1, SKY_BACK_2, SKY_BACK_3);
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
        /* @bug memory: a mode past SKY_RESET hands the task itself to
         * heap_free (JMP at 0x0c9000b0). a task is a slot of the pool
         * task_init takes from the heap, not a block heap_alloc gave */
        heap_free(task);
        break;
    }
    /* @hack redundant: a return at the end, which gives SKY_RESET's case an
     * exit of its own (RTS at 0x0c9000a8); without it, 9 of 42 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}
#pragma section
