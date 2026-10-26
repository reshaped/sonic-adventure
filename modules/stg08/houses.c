/* O_HouseA to O_HouseD: houses that are only drawn */
#include <ninja.h>

#include "object/common.h"
#include "stg08/houses.h"
#include "system/task.h"

extern NJS_OBJECT house_a_object;
extern NJS_OBJECT house_b_object;
extern NJS_OBJECT house_c_object;
extern NJS_OBJECT house_d_object;

void house_a_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &house_a_object;
    model_start(task);
}

void house_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &house_b_object;
    model_start(task);
}

void house_c_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &house_c_object;
    model_start(task);
}

void house_d_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &house_d_object;
    model_start(task);
}
