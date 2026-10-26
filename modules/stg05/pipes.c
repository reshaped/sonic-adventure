/* O PIPEI, O PIPEBOX and O PIPEL: a straight pipe, a box of pipes and a bent
 * pipe. each is a solid model, drawn and shaped at one more than the set
 * entry's scale */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg05/pipes.h"
#include "system/task.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT pipe_i_object;
extern NJS_OBJECT pipe_box_object;
extern NJS_OBJECT pipe_l_object;

static struct collision_shape pipe_box_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    10.0F,
    21.0F,
    6.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape pipe_i_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 18.0F, 0.0F},
    3.0F,
    18.0F,
    0.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape pipe_l_shapes[2] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     3.0F,
     5.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {5.0F, 17.0F, 0.0F},
     2.5F,
     2.5F,
     2.5F,
     {0},
     0,
     NULL},
};

void pipe_i_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &pipe_i_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, &pipe_i_shape, COLLISION_OBJECT, 1, 1);
        object_scale_shapes(work);
    }
    model_start_solid_scaled(task);
}

void pipe_box_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &pipe_box_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, &pipe_box_shape, COLLISION_OBJECT, 1, 1);
        object_scale_shapes(work);
    }
    model_start_solid_scaled(task);
}

void pipe_l_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &pipe_l_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, pipe_l_shapes, COLLISION_OBJECT, 1, 2);
        object_scale_shapes(work);
    }
    model_start_solid_scaled(task);
}
