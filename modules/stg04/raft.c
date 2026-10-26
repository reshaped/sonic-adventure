/* HIGH RAFT A and HIGH RAFT C platforms, O TUNAGI solid joining block, and
 * O HELIP L heliport light that is only drawn */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/raft.h"
#include "system/task.h"

extern NJS_OBJECT highway_raft_a_object;
extern NJS_OBJECT highway_raft_c_object;
extern NJS_OBJECT highway_raft_joint_object;
extern NJS_OBJECT heliport_light_object;

/* the radius of the joining block's collision record. its box's corners
 * lie 12.25 from its position */
#define JOINT_REACH 14.0F

static struct collision_shape highway_raft_joint_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 5.0F, 0.0F},
    5.0F,
    5.0F,
    5.0F,
    {0},
    0,
    NULL,
};

void highway_raft_a_step(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &highway_raft_a_object;
    model_start_world(task);
}

void highway_raft_c_step(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &highway_raft_c_object;
    model_start_world(task);
}

void highway_raft_joint_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &highway_raft_joint_object;
    if (!placement_editing(task)) {
        collision_init(task, &highway_raft_joint_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = JOINT_REACH;
    }
    model_start_solid(task);
}

void heliport_light_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &heliport_light_object;
    model_start(task);
}
