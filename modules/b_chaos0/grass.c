/* O GRAS0 to O GRAS3, Chaos 0's grass. each kind's step puts its tuft's
 * model in the work's model_vars, as a model object does. it then gives the
 * task a step and a draw that do nothing */
#include <ninja.h>

#include "b_chaos0/grass.h"
#include "b_chaos0/textures.h"
#include "object/common.h"
#include "system/task.h"

extern NJS_OBJECT chaos0_grass_1_object;
extern NJS_OBJECT chaos0_grass_2_object;
extern NJS_OBJECT chaos0_grass_3_object;

static void grass_idle(struct task *task)
{
}

void chaos0_grass_0_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &chaos0_grass_0_object;
    task->step = grass_idle;
    task->draw = grass_idle;
}

void chaos0_grass_1_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &chaos0_grass_1_object;
    task->step = grass_idle;
    task->draw = grass_idle;
}

void chaos0_grass_2_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &chaos0_grass_2_object;
    task->step = grass_idle;
    task->draw = grass_idle;
}

void chaos0_grass_3_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &chaos0_grass_3_object;
    task->step = grass_idle;
    task->draw = grass_idle;
}
