/* O GATESIDE: two solid posts, one at each side of a gate */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "stg12/gate_side.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT gate_side_object;

static void gate_side_run(struct task *task);
static void gate_side_draw(struct task *task);

static struct collision_shape gate_side_shapes[2] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {10.0F, 0.0F, -2.0F},
     8.0F,
     46.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-10.0F, 0.0F, -2.0F},
     8.0F,
     46.0F,
     0.0F,
     {0},
     0,
     NULL},
};

void gate_side_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, gate_side_shapes, COLLISION_OBJECT, 1, 2);
        work->collision->radius = 70.0F;
    }
    task->step = gate_side_run;
    task->draw = gate_side_draw;
}

static void gate_side_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_register(work);
    }
    gate_side_draw(task);
}

static void gate_side_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, -12.5F, -54.0F, 40.0F);
    draw_tree(&gate_side_object);
    njPopMatrix(1);
}
