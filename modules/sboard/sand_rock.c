/* SNAKE STATUE, ROCK and ROCKY HEAD: solid models at the size their scale
 * gives */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "object/placement.h"
#include "object/simple.h"
#include "sboard/sand_rock.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"

/* a rock's record, in task->motion */
struct sand_rock_record {
    Sint8 mode;
    Float reach; /* the world test's radius at size 1: twice the model's */
    NJS_OBJECT *world; /* the rock's world object */
    NJS_OBJECT *object;
};

enum sand_rock_mode { SAND_ROCK_STAND };

extern NJS_OBJECT sand_snake_statue_object;
extern NJS_OBJECT sand_rock_object;
extern NJS_OBJECT sand_rocky_head_object;

static void sand_rock_init(struct task *task, void *arg);
static void sand_rock_run(struct task *task);
static void sand_rock_draw(struct task *task);
static void sand_rock_destroy(struct task *task);

static const struct task_desc sand_rock_desc = {sizeof(struct sand_rock_record),
                                                TASK_WORK | TASK_FORCE,
                                                sand_rock_init,
                                                sand_rock_run,
                                                sand_rock_draw,
                                                sand_rock_destroy};
static NJS_OBJECT *const sand_snake_statue = &sand_snake_statue_object;
static NJS_OBJECT *const sand_rock = &sand_rock_object;
static NJS_OBJECT *const sand_rocky_head = &sand_rocky_head_object;

void sand_snake_statue_step(struct task *task)
{
    task_apply_desc(task, &sand_rock_desc, (void *)&sand_snake_statue);
}

void sand_rock_step(struct task *task)
{
    task_apply_desc(task, &sand_rock_desc, (void *)&sand_rock);
}

void sand_rocky_head_step(struct task *task)
{
    task_apply_desc(task, &sand_rock_desc, (void *)&sand_rocky_head);
}

/* a rock's size: z times scale, or 1 for a z of 0 or more. each call
 * passes scl.z as z */
/* @hack spelling: the size in a helper, which SHC expands at each of its
 * three calls, as from 0x0c9049d2 to 0x0c9049ea in sand_rock_init. written in
 * place, 109 of the 152 words of sand_rock_init, sand_rock_run and
 * sand_rock_draw differ */
/* @hack spelling: the scale through a parameter, which each call passes as
 * -0.1F and SHC loads into FR5 ahead of the test (FMOV.S @R0,FR5 at
 * 0x0c9049d4); as a constant in sand_rock_size, 111 of the 152 words of
 * sand_rock_init, sand_rock_run and sand_rock_draw differ */
static Float sand_rock_size(Float z, Float scale)
{
    /* @hack spelling: the size through a local, which keeps the task in R12
     * in sand_rock_init (MOV R4,R12 at 0x0c9049a4); returned directly, the
     * task and its record trade R12 and R13, and 11 of those 152 words
     * differ */
    Float size = z >= 0.0F ? 1.0F : z * scale;

    return size;
}

static void sand_rock_init(struct task *task, void *arg)
{
    NJS_OBJECT *object = *(NJS_OBJECT *const *)arg;
    NJS_OBJECT *world = object_world_copy(object);
    struct sand_rock_record *record = (struct sand_rock_record *)task->motion;
    struct task_work *work;
    NJS_POINT3 *scl;
    Float size;

    record->world = world;
    if (world == NULL) {
        task_kill(task);
    } else {
        work = task->work;
        scl = &work->scl;
        size = sand_rock_size(scl->z, -0.1F);
        object_world_place(world, work, size);
        object_world_add_sized(task, world, WORLD_SOLID, size);
        object_force_clear(task->force);
        record->mode = SAND_ROCK_STAND;
        record->reach = world->model->r * 2.0F;
        record->object = object;
    }
}

/* draw the rock. unless the set editor holds it, test its world object
 * while player 0 or 1 is within reach. the reach scales with the rock's
 * size */
static void sand_rock_run(struct task *task)
{
    struct task_work *work;
    struct sand_rock_record *record;
    NJS_POINT3 *scl;
    Float size;

    if (placement_try_leave(task)) {
        return;
    }
    sand_rock_draw(task);
    work = task->work;
    record = (struct sand_rock_record *)task->motion;
    if (placement_editing(task)) {
        return;
    }
    scl = &work->scl;
    size = sand_rock_size(scl->z, -0.1F);
    object_world_test_radius(work, record->reach * size);
}

/* on a frame's first step, draw the rock at its size */
static void sand_rock_draw(struct task *task)
{
    struct task_work *work;
    struct sand_rock_record *record;
    Float size;

    if (frame_step == 0) {
        work = task->work;
        record = (struct sand_rock_record *)task->motion;
        size = sand_rock_size(work->scl.z, -0.1F);
        object_draw_tree_zyx(record->object, work, size, NULL);
    }
}

static void sand_rock_destroy(struct task *task)
{
    struct sand_rock_record *record = (struct sand_rock_record *)task->motion;

    object_world_free(task, record->world);
}
