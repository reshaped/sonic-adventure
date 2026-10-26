/* Chaos 6's stage, the fight on the Egg Carrier's deck: its task, and the
 * task that starts its music */
#include "b_chaos6/chaos6.h"
#include "b_chaos6/boss.h"
#include "b_chaos6/sky.h"
#include "chaos/effect.h"
#include "chaos/start.h"
#include "sound/stream.h"
#include "system/task.h"

enum chaos6_mode { CHAOS6_START, CHAOS6_FIGHT };

static void chaos6_start(struct task *task);

/* under section HEAD, SHC writes the step to section PHEAD, which leads the
 * module. so the step lies at 0x0c900000, where 1ST_READ.BIN's stage_steps
 * reaches it */
#pragma section HEAD
void chaos6_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CHAOS6_START:
        chaos6_start(task);
        break;
    case CHAOS6_FIGHT:
        task_step_children(task);
        break;
    }
}
#pragma section

void chaos6_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    --work->timer;
    if (work->timer < 0) {
        stream_reset();
        stream_request_and_save_music(work->mode);
        task_destroy(task);
    }
}

/* the stage's first step: start the boss and the clouds, and set up the
 * Chaos effects. it also sets boss_waiting and clears chaos_handed_over,
 * through chaos_start */
static void chaos6_start(struct task *task)
{
    struct task_work *work = task->work;

    chaos_start();
    chaos6_create();
    chaos_effect_init();
    task_create(TASK_WORK, 3, chaos6_clouds_step);
    work->mode = CHAOS6_FIGHT;
}
