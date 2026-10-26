/* O_Ele, O_Fiw4m, O_Fiw5m and O_Fiw6m: solid rocks */
#include <ninja.h>

#include "collision/collision.h"
#include "object/common.h"
#include "stg08/rocks.h"
#include "system/task.h"

/* O_Ele is a low rock hung with icicles */
extern NJS_OBJECT icicle_rock_object;
extern NJS_OBJECT rock_4m_object;
extern NJS_OBJECT rock_5m_object;
extern NJS_OBJECT rock_6m_object;

/* a cylinder around each rock, centered below its position */
#define ROCK_SHAPE(y, radius, height)                                          \
    {SHAPE_CYLINDER,                                                           \
     0,                                                                        \
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,                                           \
     {0.0F, (y), 0.0F},                                                        \
     (radius),                                                                 \
     (height),                                                                 \
     0.0F,                                                                     \
     {0},                                                                      \
     0,                                                                        \
     NULL}
static struct collision_shape icicle_rock_shape =
    ROCK_SHAPE(-3.5F, 14.0F, 5.0F);
static struct collision_shape rock_4m_shape = ROCK_SHAPE(-10.0F, 24.0F, 10.0F);
static struct collision_shape rock_5m_shape = ROCK_SHAPE(-14.0F, 25.0F, 14.0F);
static struct collision_shape rock_6m_shape = ROCK_SHAPE(-16.0F, 31.0F, 16.0F);

void icicle_rock_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &icicle_rock_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &icicle_rock_object;
    model_start_solid(task);
}

void rock_4m_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &rock_4m_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &rock_4m_object;
    model_start_solid(task);
}

void rock_5m_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &rock_5m_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &rock_5m_object;
    model_start_solid(task);
}

void rock_6m_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &rock_6m_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &rock_6m_object;
    model_start_solid(task);
}
