/* O TREEM: a tree that plays its action over and over */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "stg02/tree.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION windy_tree_action;

/* in TREE_STOP and TREE_END each step does nothing, and no step here sets
 * either */
enum tree_mode { TREE_START, TREE_RUN, TREE_STOP, TREE_END };

struct tree_vars {
    /* the motion's frame. windy_tree_draw sets it back to 0 at the motion's
     * end */
    Float frame;
};

#define VARS(work) ((struct tree_vars *)&(work)->vars)

static void windy_tree_draw(struct task *task);
static void windy_tree_destroy(struct task *task);

void windy_tree_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case TREE_START:
            task->destructor = windy_tree_destroy;
            task->draw = windy_tree_draw;
            work->mode = TREE_RUN;
            break;
        case TREE_RUN:
            if (!placement_editing(task)) {
                VARS(work)->frame += work->scl.x;
            }
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- MOTION_SPEED");
            }
            windy_tree_draw(task);
            break;
        case TREE_STOP:
        case TREE_END:
            break;
        }
    }
}

static void windy_tree_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action = &windy_tree_action;
    Angle ang;

    if (VARS(work)->frame >= action->motion->nbFrame) {
        VARS(work)->frame = 0.0F;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(action, VARS(work)->frame);
    njPopMatrix(1);
}

static void windy_tree_destroy(struct task *task)
{
}
