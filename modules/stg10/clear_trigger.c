/* C RELEASE trigger that clears the stage when player 0 comes within its
 * radius. once Sonic has cleared Final Egg, it places a capsule instead */
#include <ninja.h>

#include "debug/editor.h"
#include "object/capsule.h"
#include "object/helpers.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "stg10/clear_trigger.h"
#include "system/task.h"

/* the trigger's radius is REACH times (scale x + 1) */
#define REACH 14.0F

enum clear_trigger_mode {
    CLEAR_INIT = 0,
    CLEAR_WAIT = 2, /* test for player 0, unless Sonic has cleared Final Egg */
    CLEAR_CAPSULE = 3, /* Sonic has cleared Final Egg: place the capsule */
    CLEAR_PLACED = 4, /* the capsule stands where the trigger does */
    CLEAR_DONE = 5 /* player 0 has cleared the stage */
};

static void final_clear_trigger_draw(struct task *task);
static void final_clear_trigger_touch(struct task *task);

void final_clear_trigger_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *capsule;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CLEAR_INIT:
        work->mode = CLEAR_WAIT;
        task->draw = NULL;
        break;
    case CLEAR_WAIT:
        if (event_flag_check(FLAG_SONIC_CLEAR_FINAL_EGG)) {
            work->mode = CLEAR_CAPSULE;
        } else {
            if (!placement_editing(task)) {
                final_clear_trigger_touch(task);
            }
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- SCALE");
                final_clear_trigger_draw(task);
            }
        }
        break;
    case CLEAR_CAPSULE:
        capsule = task_create(TASK_WORK, 2, capsule_start);
        capsule->work->pos.x = work->pos.x;
        capsule->work->pos.y = work->pos.y;
        capsule->work->pos.z = work->pos.z;
        work->mode = CLEAR_PLACED;
        break;
    case CLEAR_PLACED:
    case CLEAR_DONE:
        break;
    }
}

static void final_clear_trigger_draw(struct task *task)
{
    struct task_work *work = task->work;

    marker_draw(&work->pos, (work->scl.x + 1.0F) * REACH);
}

static void final_clear_trigger_touch(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, (work->scl.x + 1.0F) * REACH) == 1) {
        stage_clear();
        work->mode = CLEAR_DONE;
    }
}
