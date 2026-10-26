/* O CARGO, a flat car of the cargo train */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg12/cargo_car.h"
#include "stg12/cargo_train.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cargo_car_object;
extern NJS_OBJECT cargo_car_world_object;

/* a placed car is destroyed beyond 1000 units of the camera and player 0 */
#define RANGE (1000.0F * 1000.0F)
/* the collision world tests a car only while a player is within 100 units
 * of it */
#define NEAR 100.0F

/* in work->submode: a car an act's object list placed, or one
 * cargo_train_start made */
enum cargo_car_kind { CARGO_PLACED, CARGO_ON_TRAIN };

struct cargo_car_vars {
    /* the car's object in the collision world, or NULL when init ran in the set
     * editor */
    NJS_OBJECT *world;
};

#define VARS(work) ((struct cargo_car_vars *)&(work)->vars)

static void cargo_car_init(struct task *task);
static void cargo_car_placed(struct task *task);
static void cargo_car_on_train(struct task *task);
static void cargo_car_draw(struct task *task);
static void cargo_car_destroy(struct task *task);

static struct collision_shape cargo_car_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 7.0F, 0.0F},
    32.0F,
    7.0F,
    70.0F,
    {0},
    0,
    NULL,
};

void cargo_car_step(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = CARGO_PLACED;
    cargo_car_init(task);
}

void cargo_car_train_step(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = CARGO_ON_TRAIN;
    cargo_car_init(task);
}

static void cargo_car_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->ang.x = 0;
    work->ang.z = 0;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
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
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = cargo_car_world_object.model;
        world->child = NULL;
        world->sibling = NULL;
        world_add(WORLD_LARGE | WORLD_MOVES, task, world);
        work->flags |= WORLD_TESTED;
        collision_init(task, &cargo_car_shape, COLLISION_OBJECT, 1, 1);
    }
    if (work->submode == CARGO_PLACED) {
        task->step = cargo_car_placed;
    } else {
        task->step = cargo_car_on_train;
    }
    task->draw = cargo_car_draw;
    task->destructor = cargo_car_destroy;
}

static void cargo_car_placed(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            if (player_in_range(&work->pos, NEAR)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            collision_register(work);
        }
        cargo_car_draw(task);
    }
}

/* move the car along z by cargo_train_speed, and its object in the collision
 * world with it. its force for player 0 moves a player 0 standing on the car by
 * the same distance */
static void cargo_car_on_train(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    Float speed;
    Float z;

    cargo_car_draw(task);
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

static void cargo_car_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter4_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&cargo_car_object);
        njPopMatrix(1);
    }
}

static void cargo_car_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}
