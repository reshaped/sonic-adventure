/* O GRASA to O GRASD: tufts of grass, each a model that only draws. O GRASB
 * alone draws scaled, by one more than its set entry's scale */
#include <ninja.h>

#include "object/common.h"
#include "stg01/grass.h"
#include "system/task.h"

extern NJS_OBJECT beach_grass_a_object;
extern NJS_OBJECT beach_grass_b_object;
extern NJS_OBJECT beach_grass_c_object;
extern NJS_OBJECT beach_grass_d_object;

void beach_grass_a_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &beach_grass_a_object;
    model_start(task);
}

void beach_grass_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &beach_grass_b_object;
    model_start_scaled(task);
}

void beach_grass_c_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &beach_grass_c_object;
    model_start(task);
}

void beach_grass_d_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &beach_grass_d_object;
    model_start(task);
}
