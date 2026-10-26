/* O CORA and O CORB, two corals. each draws scaled by one more than its set
 * entry's scale on each axis */
#include <ninja.h>

#include "object/common.h"
#include "stg01/coral.h"
#include "system/task.h"

extern NJS_OBJECT coral_a_object;
extern NJS_OBJECT coral_b_object;

void coral_a_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &coral_a_object;
    model_start_scaled(task);
}

void coral_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &coral_b_object;
    model_start_scaled(task);
}
