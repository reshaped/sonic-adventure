/* Kareki01, Kareki02 and Kareki03: solid dead trees */
#include <ninja.h>

#include "collision/collision.h"
#include "object/common.h"
#include "stg08/dead_trees.h"
#include "system/task.h"

extern NJS_OBJECT dead_tree_1_object;
extern NJS_OBJECT dead_tree_2_object;
extern NJS_OBJECT dead_tree_3_object;

#define TRUNK_SHAPE                                                            \
    {SHAPE_CAPSULE,                                                            \
     0,                                                                        \
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,                                           \
     {0.0F, 40.0F, 0.0F},                                                      \
     10.0F,                                                                    \
     30.0F,                                                                    \
     0.0F,                                                                     \
     {0},                                                                      \
     0,                                                                        \
     NULL}
/* a shape for each tree, all alike, though every tree takes the first */
static struct collision_shape dead_tree_1_shape = TRUNK_SHAPE;
static struct collision_shape dead_tree_2_shape = TRUNK_SHAPE;
static struct collision_shape dead_tree_3_shape = TRUNK_SHAPE;

void dead_tree_1_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &dead_tree_1_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &dead_tree_1_object;
    model_start_solid(task);
}

void dead_tree_2_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &dead_tree_1_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &dead_tree_2_object;
    model_start_solid(task);
}

void dead_tree_3_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &dead_tree_1_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &dead_tree_3_object;
    model_start_solid(task);
}
