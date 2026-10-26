/* Chaos 2's stage, the fight in a hall of tables and chandeliers: the
 * stage's task, its sky's task, and the boss music's wait */
#include <ninja.h>

#include "b_chaos2/boss.h"
#include "b_chaos2/chaos2.h"
#include "b_chaos2/effects.h"
#include "chaos/effect.h"
#include "chaos/start.h"
#include "sound/stream.h"
#include "system/task.h"

enum chaos2_mode { CHAOS2_START, CHAOS2_FIGHT, CHAOS2_END };

enum sky_mode { SKY_START, SKY_RUN, SKY_RESET };

/* the colors njSetBackColor blends across the screen from its top left,
 * top right and bottom left corners: dark blue, violet and a dull pink */
#define BACK_TOP_LEFT 0x1040
#define BACK_TOP_RIGHT 0x401074
#define BACK_BOTTOM_LEFT 0x807474

static void chaos2_start(struct task *task);
static void chaos2_end(struct task *task);

/* in the HEAD section, at 0x0c900080, which 1ST_READ.BIN's stage_steps
 * names */
#pragma section HEAD
void chaos2_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CHAOS2_START:
        chaos2_start(task);
        break;
    case CHAOS2_FIGHT:
        break;
    case CHAOS2_END:
        chaos2_end(task);
        break;
    }
}
#pragma section

void chaos2_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    --work->timer;
    if (work->timer < 0) {
        stream_reset();
        stream_request_and_save_music(work->mode);
        task_destroy(task);
    }
}

static void chaos2_start(struct task *task)
{
    struct task_work *work = task->work;

    chaos_start();
    chaos2_create();
    chaos_effect_init();
    task_create(TASK_WORK, 1, chaos2_land_fade_step);
    work->mode = CHAOS2_FIGHT;
}

static void chaos2_end(struct task *task)
{
    stream_destroy();
}

/* in the HEAD section, at 0x0c9000e0, which 1ST_READ.BIN's sky_steps
 * names. it draws no sky, only the back color behind the land */
#pragma section HEAD
void chaos2_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SKY_START:
        njSetBackColor(BACK_TOP_LEFT, BACK_TOP_RIGHT, BACK_BOTTOM_LEFT);
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
         * at 0x0c900130). a task is a slot of the pool task_init takes from the
         * heap, not a block heap_alloc gave */
        heap_free(task);
        break;
    }
    /* @hack redundant: a return where the function ends, which gives
     * SKY_RESET's break an RTS of its own (0x0c900122 to 0x0c90012a); without
     * it, the break branches to the epilogue past the pool, and 9 of 42 words
     * differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}
#pragma section
