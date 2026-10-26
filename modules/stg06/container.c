/* the containers the object list names O CON and O D CON. O CON slides along
 * z as gravity tilts, and carries whoever stands on it. O D CON stands still
 * until the collapse reaches it, then drifts away */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg06/collapse.h"
#include "stg06/container.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT sky_deck_container_object;
extern NJS_OBJECT container_d_object;
extern NJS_OBJECT container_world_object;

/* the square of the distance from the camera or player 0 within which a
 * container stays */
#define CONTAINER_RANGE (2010.0F * 2010.0F)
/* the world tests a container while a player is within CONTAINER_NEAR of
 * its position */
#define CONTAINER_NEAR 70.0F
#define SLIDING_FLAGS (WORLD_MEDIUM | WORLD_MOVES)
#define STILL_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

/* once the collapse reaches O D CON, it moves DRIFT_X along x and DRIFT_Y up
 * each step. it also turns DRIFT_TURN about z each step */
#define DRIFT_X 1.8F
#define DRIFT_Y 0.4F
#define DRIFT_TURN 32

struct container_vars {
    NJS_OBJECT *world;
    NJS_OBJECT *object; /* the model it draws, O CON's or O D CON's */
};

#define VARS(work) ((struct container_vars *)&(work)->vars)
/* O CON's z as it started, which its travel is measured from */
#define HOME(work) ((work)->scl.z)

enum container_mode { CONTAINER_INIT, CONTAINER_RUN };

/* one of O CON's travels along z. it runs from HOME + low to HOME + high, at
 * speed times gravity's z each step. the first step picks a travel by scl.x,
 * and work->submode holds its index in container_travels */
struct container_travel {
    Float low;
    Float high;
    Float speed;
};

static struct container_travel container_travels[3] = {
    {0.0F, 210.0F, 15.0F},
    {-210.0F, 0.0F, 5.0F},
    {-245.0F, 245.0F, 10.0F},
};

static void sky_deck_container_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

static void sky_deck_container_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}

/* put O CON's shape in the world where it stands, as one that moves */
static void container_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *shape = &container_world_object;
    NJS_OBJECT *world;

    VARS(work)->world = world = world_object_alloc();
    world->evalflags =
        shape->evalflags & ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = shape->model;
    world->child = shape->child;
    world->sibling = shape->sibling;
    world_add(SLIDING_FLAGS, task, world);
}

/* put O D CON's shape, the same as O CON's, in the world as a solid one */
static void container_d_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *shape = &container_world_object;
    NJS_OBJECT *world;

    VARS(work)->world = world = world_object_alloc();
    world->evalflags =
        shape->evalflags & ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = shape->model;
    world->child = shape->child;
    world->sibling = shape->sibling;
    world_add(STILL_FLAGS, task, world);
}

/* move the container's shape to its position and angles. the world tests
 * the shape while a player is within CONTAINER_NEAR of the container */
static void container_update_world(struct task_work *work)
{
    NJS_OBJECT *world = VARS(work)->world;

    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    if (player_in_range(&work->pos, CONTAINER_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* O CON's step. each step the container slides along z by its travel's
 * speed times gravity's z. it stops at either end of the travel. its force's
 * speed is the slide, which carries whoever stands on it */
void sky_deck_container_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;
    Float pos;
    Float home;
    Float speed;
    Sint8 travel;

    if (placement_try_leave_by(task, CONTAINER_RANGE)) {
        return;
    }
    object = &sky_deck_container_object;
    if (edit_mode) {
        if (VARS(work)->object == NULL) {
            VARS(work)->object = object;
        }
        sky_deck_container_draw(task);
    } else {
        switch (work->mode) {
        case CONTAINER_INIT:
            work->mode = CONTAINER_RUN;
            task->destructor = sky_deck_container_destroy;
            task->draw = sky_deck_container_draw;
            VARS(work)->object = object;
            container_add_world(task);
            if (work->scl.x < 10.0F) {
                work->submode = 0;
            } else if (work->scl.x < 20.0F) {
                work->submode = 1;
            } else {
                work->submode = 2;
            }
            HOME(work) = work->pos.z;
            break;
        case CONTAINER_RUN:
            pos = work->pos.z;
            home = HOME(work);
            travel = work->submode;
            /* @hack spelling: speed set to the travel's and then multiplied by
             * the tilt in a statement of its own, which keeps it in FR5 for
             * the FMUL at 0x0c90a6d6; as one product, 11 of 135 words
             * differ */
            speed = container_travels[travel].speed;
            speed *= world_gravity.z;
            pos += speed;
            if (pos < home + container_travels[travel].low) {
                pos = home + container_travels[travel].low;
                speed = 0.0F;
            } else if (pos > home + container_travels[travel].high) {
                pos = home + container_travels[travel].high;
                speed = 0.0F;
            }
            work->pos.z = pos;
            force->speed.z = speed;
            container_update_world(work);
            sky_deck_container_draw(task);
            break;
        }
    }
}

/* O D CON's step. once the collapse reaches the container, it drifts away
 * each step. a container that leaves range after that is gone for good: its
 * placement makes no more */
void sky_deck_container_d_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave_by(task, CONTAINER_RANGE)) {
        if (work->mode == CONTAINER_RUN) {
            if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
                placement_kill(task);
            }
        }
        return;
    }
    object = &container_d_object;
    if (edit_mode) {
        if (VARS(work)->object == NULL) {
            VARS(work)->object = object;
        }
        sky_deck_container_draw(task);
    } else {
        switch (work->mode) {
        case CONTAINER_INIT:
            work->mode = CONTAINER_RUN;
            task->destructor = sky_deck_container_destroy;
            task->draw = sky_deck_container_draw;
            VARS(work)->object = object;
            container_d_add_world(task);
            break;
        case CONTAINER_RUN:
            container_update_world(work);
            if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
                work->ang.z += DRIFT_TURN;
                work->pos.y += DRIFT_Y;
                work->pos.x += DRIFT_X;
            }
            sky_deck_container_draw(task);
            break;
        }
    }
}
