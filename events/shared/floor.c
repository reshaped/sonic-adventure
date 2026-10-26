/* an event's floor: a solid triangle in the collision world. it follows a
 * task's x and z at a height of its own */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "shared/floor.h"
#include "system/task.h"

#define FLOOR_FLAGS (WORLD_SMALL | WORLD_MOVES | WORLD_SOLID)

/* the floor's height is in the work's pos.y */
struct floor_vars {
    NJS_OBJECT *object; /* the world's object it borrows for the triangle */
    struct task *target; /* the task it stays under */
};

#define VARS(work) ((struct floor_vars *)&(work)->vars)

static NJS_MATERIAL event_floor_materials[1] = {
#include "shared/floor/event_floor_materials.inc"
};

static Sint16 event_floor_meshes[3] = {
#include "shared/floor/event_floor_meshes.inc"
};

static NJS_MESHSET event_floor_meshsets[1] = {
#include "shared/floor/event_floor_meshsets.inc"
};

static NJS_POINT3 event_floor_points[3] = {
#include "shared/floor/event_floor_points.inc"
};

static NJS_VECTOR event_floor_normals[3] = {
#include "shared/floor/event_floor_normals.inc"
};

static NJS_MODEL event_floor_model = {
#include "shared/floor/event_floor_model.inc"
};

static NJS_OBJECT event_floor_object = {
#include "shared/floor/event_floor_object.inc"
};

static void event_floor_destroy(struct task *task)
{
    NJS_OBJECT *object = VARS(task->work)->object;

    world_remove(task, object);
    world_object_free(object);
}

static void event_floor_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    struct task *target = VARS(work)->target;

    object->pos[0] = target->work->pos.x;
    object->pos[2] = target->work->pos.z;
}

static void event_floor_init(struct task *task)
{
    NJS_OBJECT *floor;
    struct task_work *work;
    NJS_OBJECT *object;
    struct task_force *force;

    task->step = event_floor_step;
    task->destructor = event_floor_destroy;
    floor = &event_floor_object;
    work = task->work;
    object = world_object_alloc();
    VARS(work)->object = object;
    object->evalflags = floor->evalflags;
    event_floor_step(task);
    object->pos[1] = work->pos.y;
    object->ang[0] = 0;
    object->ang[1] = 0;
    object->ang[2] = 0;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->model = floor->model;
    object->child = floor->child;
    object->sibling = floor->sibling;
    world_add(FLOOR_FLAGS, task, object);
    work->flags |= WORLD_TESTED;
    force = task->force;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
}

struct task *event_floor_create(struct task *target, Float y)
{
    struct task *task =
        task_create(TASK_WORK | TASK_FORCE, 3, event_floor_init);

    task->work->pos.y = y;
    VARS(task->work)->target = target;
    return task;
}
