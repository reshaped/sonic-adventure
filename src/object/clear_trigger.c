/* the clear trigger, O CLROBJ in the object lists. player 0's touch clears
 * the stage. in STAGE_MOUNTAIN, STAGE_RUIN and STAGE_FINAL, once Sonic has
 * cleared the stage, a capsule stands in the trigger's place instead */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/capsule.h"
#include "object/clear_trigger.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"

/* the trigger's reach is REACH * (work->scl.x + 1) */
#define REACH 14.0F

/* the trigger's work->mode. it has no 1, and clear_trigger_step no case
 * for one */
enum clear_trigger_mode {
    CLEAR_INIT = 0,
    CLEAR_WAIT = 2, /* waits for player 0's touch, or for the flag */
    CLEAR_CAPSULE = 3, /* makes the capsule */
    CLEAR_PLACED = 4, /* the capsule stands in the trigger's place */
    CLEAR_DONE = 5 /* touched */
};

struct clear_trigger_vars {
    /* the event flag that puts a capsule in the trigger's place: Sonic's
     * clear of STAGE_MOUNTAIN, STAGE_RUIN or STAGE_FINAL. in any other
     * stage it stays 0, as task_create zeroes the work */
    Sint32 flag;
};

#define VARS(work) ((struct clear_trigger_vars *)&(work)->vars)

static void clear_trigger_draw(struct task *task);
static void clear_trigger_touch(struct task *task);

void clear_trigger_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *capsule;
    Uint16 stage;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CLEAR_INIT:
        work->mode = CLEAR_WAIT;
        stage = (Uint8)(stage_get_stage_act() >> 8);
        switch (stage) {
        case STAGE_MOUNTAIN:
            VARS(work)->flag = FLAG_SONIC_CLEAR_MOUNTAIN;
            break;
        case STAGE_RUIN:
            VARS(work)->flag = FLAG_SONIC_CLEAR_RUIN;
            break;
        case STAGE_FINAL:
            VARS(work)->flag = FLAG_SONIC_CLEAR_FINAL_EGG;
            break;
        }
        break;
    case CLEAR_WAIT:
        if (event_flag_check(VARS(work)->flag)) {
            work->mode = CLEAR_CAPSULE;
        } else {
            if (!placement_editing(task)) {
                clear_trigger_touch(task);
            }
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- SCALE");
            }
            if (placement_editing(task) || edit_mode == 1) {
                clear_trigger_draw(task);
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

/* draw marker_object at the trigger for the set editor: a ball of radius
 * 10 * (work->scl.x + 1) */
static void clear_trigger_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: the scale as the x of a vector, which SHC stores at @R15
     * and loads back into FR15 after njTranslate (0x8c0a8c44, 0x8c0a8c5c).
     * as a Float, SHC holds it in FR15 across both calls, and 40 of 41
     * words differ */
    NJS_VECTOR scale;

    scale.x = work->scl.x + 1.0F;
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njScale(NULL, scale.x, scale.x, scale.x);
    draw_tree(&marker_object);
    njPopMatrix(1);
}

/* clear the stage once player 0 is within the trigger's reach.
 * player_in_range gives 1 for player 0 */
static void clear_trigger_touch(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, (work->scl.x + 1.0F) * REACH) == 1) {
        stage_clear();
        work->mode = CLEAR_DONE;
    }
}
