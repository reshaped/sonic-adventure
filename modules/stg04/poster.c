/* O POSTER0, O POSTER1 and O POSTER2 scaled poster models */
#include <ninja.h>

#include "object/common.h"
#include "stg04/poster.h"
#include "system/task.h"

extern NJS_OBJECT highway_poster_0_object;
extern NJS_OBJECT highway_poster_1_object;
extern NJS_OBJECT highway_poster_2_object;

void highway_poster_0_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &highway_poster_0_object;
    model_start_scaled(task);
}

void highway_poster_1_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &highway_poster_1_object;
    model_start_scaled(task);
}

void highway_poster_2_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &highway_poster_2_object;
    model_start_scaled(task);
}
