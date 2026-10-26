/* O CARGOCONTAINER, a container, A or B, that rides a cargo car */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg12/cargo_container.h"
#include "stg12/cargo_train.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cargo_container_a_object;
extern NJS_OBJECT cargo_container_b_object;
extern NJS_OBJECT cargo_container_a_world_object;

/* the containers, A and B */
#define CONTAINERS 2

/* how far work->collision reaches from the task. init sets it in place of
 * the reach collision_init measures from the shapes */
#define CONTAINER_REACH 60.0F
/* how far above the container its object in the collision world sits */
#define WORLD_LIFT 0.5F
/* the collision world tests a container on the train only while a player is
 * within 100 units */
#define NEAR 100.0F

/* in work->submode: a container an act's object list placed, or one
 * cargo_train_start made */
enum cargo_container_kind { CARGO_PLACED, CARGO_ON_TRAIN };

struct cargo_container_vars {
    /* the container's object in the collision world, or NULL when init ran in
     * the set editor */
    NJS_OBJECT *world;
};

#define VARS(work) ((struct cargo_container_vars *)&(work)->vars)
/* which container the task is, 0 for A or 1 for B, as scale x gives it */
#define CONTAINER(work) ((work)->tick)

static void cargo_container_init(struct task *task);
static void cargo_container_placed(struct task *task);
static void cargo_container_on_train(struct task *task);
static void cargo_container_draw(struct task *task);
static void cargo_container_destroy(struct task *task);

static NJS_OBJECT *cargo_container_objects[CONTAINERS] = {
    &cargo_container_a_object,
    &cargo_container_b_object,
};
/* each container as the collision world holds it. B's is the object it draws */
static NJS_OBJECT *cargo_container_worlds[CONTAINERS] = {
    &cargo_container_a_world_object,
    &cargo_container_b_object,
};

static struct collision_shape cargo_container_shapes[CONTAINERS] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 40.75F, 0.0F},
        30.0F,
        23.75F,
        54.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 30.0F, 0.0F},
        30.0F,
        13.0F,
        54.0F,
        {0},
        0,
        NULL,
    },
};

void cargo_container_step(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = CARGO_PLACED;
    cargo_container_init(task);
}

void cargo_container_train_step(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = CARGO_ON_TRAIN;
    cargo_container_init(task);
}

static void cargo_container_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    work->ang.x = 0;
    work->ang.z = 0;
    CONTAINER(work) = (Uint8)njRoundOff(work->scl.x) % CONTAINERS;
    if (!placement_editing(task)) {
        VARS(work)->world = world = world_object_alloc();
        world->evalflags = NJD_EVAL_BREAK;
        world->scl[0] = 1.0F;
        world->scl[1] = 1.0F;
        world->scl[2] = 1.0F;
        world->ang[0] = 0;
        world->ang[1] = work->ang.y;
        world->ang[2] = 0;
        world->pos[0] = work->pos.x;
        /* @hack redundant: the same store in both arms of a test of the
         * container. SHC merges the arms but keeps the load of CONTAINER(work)
         * and its TST, at 0x0c913110; as one store, 56 of 114 words differ */
        if (CONTAINER(work)) {
            world->pos[1] = work->pos.y + WORLD_LIFT;
        } else {
            world->pos[1] = work->pos.y + WORLD_LIFT;
        }
        world->pos[2] = work->pos.z;
        world->model = cargo_container_worlds[CONTAINER(work)]->model;
        world->child = NULL;
        world->sibling = NULL;
        world_add(WORLD_LARGE | WORLD_MOVES, task, world);
        work->flags |= WORLD_TESTED;
        collision_init(task, &cargo_container_shapes[CONTAINER(work)],
                       COLLISION_OBJECT, 1, 1);
        work->collision->radius = CONTAINER_REACH;
    }
    if (work->submode == CARGO_PLACED) {
        task->step = cargo_container_placed;
    } else {
        task->step = cargo_container_on_train;
    }
    task->draw = cargo_container_draw;
    task->destructor = cargo_container_destroy;
}

static void cargo_container_placed(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (placement_editing(task)) {
            CONTAINER(work) = (Uint8)njRoundOff(work->scl.x) % CONTAINERS;
        } else {
            collision_register(work);
        }
        cargo_container_draw(task);
    }
}

/* move the container along z by cargo_train_speed, and its object in the
 * collision world with it. its force for player 0 moves a player 0 standing on
 * the container by the same distance */
static void cargo_container_on_train(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    Float speed;
    Float z;

    cargo_container_draw(task);
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    speed = cargo_train_speed;
    if (speed != 0.0F) {
        z = work->pos.z + speed;
        work->pos.z = z;
        VARS(work)->world->pos[2] = z;
        force->speed.z = speed;
    } else {
        force->speed.z = 0.0F;
    }
    collision_register(work);
}

static void cargo_container_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter4_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(cargo_container_objects[CONTAINER(work)]);
        njPopMatrix(1);
    }
}

static void cargo_container_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}
