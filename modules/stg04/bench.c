/* O Bench solid bench */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/bench.h"
#include "system/task.h"

extern NJS_OBJECT bench_object;

/* reach of the bench's collision record from its position */
#define BENCH_REACH 20.0F

static struct collision_shape bench_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     12.0F,
     5.0F,
     3.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 3.0F},
     12.0F,
     12.0F,
     1.0F,
     {0},
     0,
     NULL},
};

void bench_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &bench_object;
    if (!placement_editing(task)) {
        collision_init(task, bench_shapes, COLLISION_OBJECT, 1, 2);
        work->collision->radius = BENCH_REACH;
    }
    model_start_solid(task);
}
