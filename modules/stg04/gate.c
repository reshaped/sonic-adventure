/* O GG solid arch over the road, and O FF solid fence */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/gate.h"
#include "system/task.h"

extern NJS_OBJECT highway_gate_object;
extern NJS_OBJECT fence_ff_object;

static struct collision_shape fence_ff_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 5.0F, 0.0F},
    10.0F,
    5.0F,
    2.0F,
    {0},
    0,
    NULL,
};

static struct collision_shape highway_gate_shapes[3] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {34.0F, 25.0F, 0.0F},
     2.5F,
     27.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-34.0F, 25.0F, 0.0F},
     2.5F,
     27.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 50.0F, 0.0F},
     35.0F,
     10.0F,
     2.0F,
     {0},
     0,
     NULL},
};

void highway_gate_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &highway_gate_object;
    if (!placement_editing(task)) {
        collision_init(task, highway_gate_shapes, COLLISION_OBJECT, 1, 3);
    }
    model_start_solid(task);
}

void fence_ff_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &fence_ff_object;
    if (!placement_editing(task)) {
        collision_init(task, &fence_ff_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}
