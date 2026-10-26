/* O WELL, a solid box with the well drawn round it */
#include <ninja.h>

#include "adv03/well.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT well_object;
extern NJS_OBJECT well_shadow_object;

/* a box about the well, which pushes every group back and which none can
 * push */
static struct collision_shape well_shape[1] = {
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 20.0F, 0.0F},
     42.0F,
     21.0F,
     22.0F,
     {0},
     0,
     NULL},
};

enum well_mode { WELL_INIT, WELL_STANDING };

static void well_draw(struct task *task);

void well_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case WELL_INIT:
        collision_init(task, well_shape, COLLISION_OBJECT, 1, 1);
        task->draw = well_draw;
        work->mode = WELL_STANDING;
        break;
    case WELL_STANDING:
        break;
    }
    well_draw(task);
    collision_register(work);
}

static void well_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    texture_set_stage();
    draw_tree(&well_object);
    draw_tree(&well_shadow_object);
    njPopMatrix(1);
}
