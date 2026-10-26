/* O SnowNewA, O SnowMountA to O SnowMountD, O SnowRockA to O SnowRockD and
 * O SnowStoneA to O SnowStoneF: snowy mounds, rocks and stones in the
 * collision world, each scaled by its set entry */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg08/snow_rocks.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT snow_new_a_object;
extern NJS_OBJECT snow_mount_a_object;
extern NJS_OBJECT snow_mount_b_object;
extern NJS_OBJECT snow_mount_c_object;
extern NJS_OBJECT snow_mount_d_object;
extern NJS_OBJECT snow_rock_a_object;
extern NJS_OBJECT snow_rock_b_object;
extern NJS_OBJECT snow_rock_c_object;
extern NJS_OBJECT snow_rock_d_object;
extern NJS_OBJECT snow_stone_a_object;
extern NJS_OBJECT snow_stone_b_object;
extern NJS_OBJECT snow_stone_c_object;
extern NJS_OBJECT snow_stone_d_object;
extern NJS_OBJECT snow_stone_e_object;
extern NJS_OBJECT snow_stone_f_object;

#define ROCK_WORLD (WORLD_LARGE | WORLD_SOLID)

/* a rock keeps WORLD_TESTED while player 0 or 1 is within its model's radius
 * plus ROCK_NEAR. without the bit, world_collect_near_players passes over the
 * rock's object */
#define ROCK_NEAR 20.0F

/* the kinds, in icecap_object_list's order */
enum snow_rock_kind {
    SNOW_NEW_A,
    SNOW_MOUNT_A,
    SNOW_MOUNT_B,
    SNOW_MOUNT_C,
    SNOW_MOUNT_D,
    SNOW_ROCK_A,
    SNOW_ROCK_B,
    SNOW_ROCK_C,
    SNOW_ROCK_D,
    SNOW_STONE_A,
    SNOW_STONE_B,
    SNOW_STONE_C,
    SNOW_STONE_D,
    SNOW_STONE_E,
    SNOW_STONE_F
};

struct snow_rock_vars {
    NJS_OBJECT *object; /* the copy of the kind's object in the world */
};

#define VARS(work) ((struct snow_rock_vars *)&(work)->vars)

static void snow_rock_destroy(struct task *task)
{
    NJS_OBJECT *object = VARS(task->work)->object;

    world_remove(task, object);
    world_object_free(object);
}

/* the rock's step, which is its draw too */
static void snow_rock_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &work->pos;
    NJS_VECTOR *scl = &work->scl;
    struct angles *ang = &work->ang;
    Angle turn;
    NJS_OBJECT *object = VARS(work)->object;

    if (!placement_try_leave(task)) {
        if (player_in_range(&work->pos, object->model->r + ROCK_NEAR)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        TURN_VIA(njRotateZ, ang->z, turn);
        TURN_VIA(njRotateX, ang->x, turn);
        TURN_VIA(njRotateY, ang->y, turn);
        njScaleV(NULL, scl);
        texture_set_stage();
        draw_model(object->model);
        njPopMatrix(1);
    }
}

/* put a copy of kind's object in the world, scaled as the task is */
static void snow_rock_init(struct task *task, Sint8 kind)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    /* @hack redundant: a discarded address of work->scl, which keeps task in
     * R13 and work in R14 (0x0c90df46, 0x0c90df52); without it, the two trade
     * and 28 of 108 words differ */
    (void)&work->scl;
    switch (kind) {
    case SNOW_NEW_A:
        VARS(work)->object =
            object_add_world(&snow_new_a_object, task, ROCK_WORLD);
        break;
    case SNOW_MOUNT_A:
        VARS(work)->object =
            object_add_world(&snow_mount_a_object, task, ROCK_WORLD);
        break;
    case SNOW_MOUNT_B:
        VARS(work)->object =
            object_add_world(&snow_mount_b_object, task, ROCK_WORLD);
        break;
    case SNOW_MOUNT_C:
        VARS(work)->object =
            object_add_world(&snow_mount_c_object, task, ROCK_WORLD);
        break;
    case SNOW_MOUNT_D:
        VARS(work)->object =
            object_add_world(&snow_mount_d_object, task, ROCK_WORLD);
        break;
    case SNOW_ROCK_A:
        VARS(work)->object =
            object_add_world(&snow_rock_a_object, task, ROCK_WORLD);
        break;
    case SNOW_ROCK_B:
        VARS(work)->object =
            object_add_world(&snow_rock_b_object, task, ROCK_WORLD);
        break;
    case SNOW_ROCK_C:
        VARS(work)->object =
            object_add_world(&snow_rock_c_object, task, ROCK_WORLD);
        break;
    case SNOW_ROCK_D:
        VARS(work)->object =
            object_add_world(&snow_rock_d_object, task, ROCK_WORLD);
        break;
    case SNOW_STONE_A:
        VARS(work)->object =
            object_add_world(&snow_stone_a_object, task, ROCK_WORLD);
        break;
    case SNOW_STONE_B:
        VARS(work)->object =
            object_add_world(&snow_stone_b_object, task, ROCK_WORLD);
        break;
    case SNOW_STONE_C:
        VARS(work)->object =
            object_add_world(&snow_stone_c_object, task, ROCK_WORLD);
        break;
    case SNOW_STONE_D:
        VARS(work)->object =
            object_add_world(&snow_stone_d_object, task, ROCK_WORLD);
        break;
    case SNOW_STONE_E:
        VARS(work)->object =
            object_add_world(&snow_stone_e_object, task, ROCK_WORLD);
        break;
    case SNOW_STONE_F:
        VARS(work)->object =
            object_add_world(&snow_stone_f_object, task, ROCK_WORLD);
        break;
    }
    object = VARS(work)->object;
    /* @hack redundant: a discarded read of object->model, which keeps the world
     * flags in R11 and object_add_world's address in R12 (0x0c90df50,
     * 0x0c90df4e); without it, the two trade and 16 of 108 words differ */
    (void)object->model;
    object->scl[0] = work->scl.x;
    object->scl[1] = work->scl.y;
    object->scl[2] = work->scl.z;
    task->step = snow_rock_run;
    task->draw = task->step;
    task->destructor = snow_rock_destroy;
}

void snow_new_a_step(struct task *task)
{
    snow_rock_init(task, SNOW_NEW_A);
}

void snow_mount_a_step(struct task *task)
{
    snow_rock_init(task, SNOW_MOUNT_A);
}

void snow_mount_b_step(struct task *task)
{
    snow_rock_init(task, SNOW_MOUNT_B);
}

void snow_mount_c_step(struct task *task)
{
    snow_rock_init(task, SNOW_MOUNT_C);
}

void snow_mount_d_step(struct task *task)
{
    snow_rock_init(task, SNOW_MOUNT_D);
}

void snow_rock_a_step(struct task *task)
{
    snow_rock_init(task, SNOW_ROCK_A);
}

void snow_rock_b_step(struct task *task)
{
    snow_rock_init(task, SNOW_ROCK_B);
}

void snow_rock_c_step(struct task *task)
{
    snow_rock_init(task, SNOW_ROCK_C);
}

void snow_rock_d_step(struct task *task)
{
    snow_rock_init(task, SNOW_ROCK_D);
}

void snow_stone_a_step(struct task *task)
{
    snow_rock_init(task, SNOW_STONE_A);
}

void snow_stone_b_step(struct task *task)
{
    snow_rock_init(task, SNOW_STONE_B);
}

void snow_stone_c_step(struct task *task)
{
    snow_rock_init(task, SNOW_STONE_C);
}

void snow_stone_d_step(struct task *task)
{
    snow_rock_init(task, SNOW_STONE_D);
}

void snow_stone_e_step(struct task *task)
{
    snow_rock_init(task, SNOW_STONE_E);
}

void snow_stone_f_step(struct task *task)
{
    snow_rock_init(task, SNOW_STONE_F);
}
