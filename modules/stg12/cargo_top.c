/* O CARGOTOP, the car at the head of the cargo train, which loops an action */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "stg12/cargo_top.h"
#include "stg12/cargo_train.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: the action declared as an array, so SHC pools its address, at
 * 0x0c912f8c, and cargo_top_play reads its motion through it at 0x0c912e9c.
 * declared as one, SHC pools the motion field's address in its place. then 3
 * of cargo_top_play's 22 words differ, as does 1 of the 40 words of the pool
 * and pad inside cargo_top_on_train */
extern NJS_ACTION cargo_top_action[];
extern NJS_OBJECT cargo_top_world_object;

/* the steps a placed car plays its action at double speed once work->tick
 * turns nonzero */
#define FAST_STEPS 60

/* in work->submode: a car an act's object list placed, or one
 * cargo_train_start made */
enum cargo_top_kind { CARGO_PLACED, CARGO_ON_TRAIN };

/* the modes of a placed car. TOP_FAST starts once work->tick turns nonzero,
 * which nothing in this file does */
enum cargo_top_mode { TOP_PLAY, TOP_FAST };

struct cargo_top_vars {
    /* the car's object in the collision world, or NULL when init ran in the
     * set editor */
    NJS_OBJECT *world;
    Float frame;
    Sint32 unk_08; /* init sets it to 5, and nothing here reads it */
};

#define VARS(work) ((struct cargo_top_vars *)&(work)->vars)

static void cargo_top_init(struct task *task);
static void cargo_top_play(struct task_work *work, Float speed);
static void cargo_top_placed(struct task *task);
static void cargo_top_on_train(struct task *task);
static void cargo_top_draw(struct task *task);
static void cargo_top_destroy(struct task *task);

void cargo_top_step(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = CARGO_PLACED;
    cargo_top_init(task);
}

void cargo_top_train_step(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = CARGO_ON_TRAIN;
    cargo_top_init(task);
}

/* a placed car in the set editor keeps cargo_top_step as its step, so init
 * runs again each step */
static void cargo_top_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = TOP_PLAY;
    work->tick = 0;
    work->timer = 0;
    VARS(work)->frame = 0.0F;
    VARS(work)->unk_08 = 5;
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
        world->model = cargo_top_world_object.model;
        world->child = NULL;
        world->sibling = NULL;
        world_add(WORLD_LARGE | WORLD_MOVES, task, world);
        work->flags |= WORLD_TESTED;
    }
    if (work->submode == CARGO_PLACED) {
        if (!placement_editing(task)) {
            task->step = cargo_top_placed;
        }
    } else {
        task->step = cargo_top_on_train;
    }
    task->draw = cargo_top_draw;
    task->destructor = cargo_top_destroy;
}

/* advance the action by speed frames. from its last frame on, it wraps back
 * to its first */
static void cargo_top_play(struct task_work *work, Float speed)
{
    Float frame = VARS(work)->frame;
    Float last;

    frame += speed;
    last = cargo_top_action->motion->nbFrame - 1;
    if (frame >= last) {
        frame -= last;
    }
    VARS(work)->frame = frame;
}

static void cargo_top_placed(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case TOP_PLAY:
                if (work->tick) {
                    work->tick = 0;
                    work->mode = TOP_FAST;
                }
                cargo_top_play(work, 1.0F);
                break;
            case TOP_FAST:
                cargo_top_play(work, 2.0F);
                TASK_TIMER(work)++;
                if (TASK_TIMER(work) > FAST_STEPS) {
                    work->timer = 0;
                    work->mode = TOP_PLAY;
                }
                break;
            }
        }
        cargo_top_draw(task);
    }
}

/* move the car along z by cargo_train_speed, and its object in the collision
 * world with it. its force for player 0 moves a player 0 standing on the car by
 * the same distance. the action plays at double speed while the train runs
 * forward. it holds while the train backs up, and plays at its own speed while
 * the train stands */
static void cargo_top_on_train(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    Float speed = cargo_train_speed;
    Float z;

    if (speed != 0.0F) {
        z = work->pos.z + speed;
        work->pos.z = z;
        VARS(work)->world->pos[2] = z;
        force->speed.z = speed;
        if (speed > 0.0F) {
            cargo_top_play(work, 2.0F);
        }
    } else {
        force->speed.z = 0.0F;
        cargo_top_play(work, 1.0F);
    }
    cargo_top_draw(task);
}

static void cargo_top_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter4_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(cargo_top_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void cargo_top_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}
