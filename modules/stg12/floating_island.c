/* O UKIJIMA: a floating platform that bobs and rocks under player 0 */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg12/floating_island.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT floating_island_object;
extern NJS_OBJECT floating_island_world_object;

#define BODY_WORLD (WORLD_SMALL | WORLD_MOVES)

/* the collision world tests the island only while player 0 or 1 is within
 * NEAR of it */
#define NEAR 25.0F

/* player 0 landing sets the island's tip speeds about x and z. each is
 * TIP_SHARE of the angle whose tangent is player 0's offset along the other
 * axis over TIP_DEPTH */
#define TIP_DEPTH 140.0F
#define TIP_SHARE 0.25F

/* the island settles at rest once its speed and its distance from rest are
 * both under BOB_STOP. it settles level once its tip speed is under TIP_STOP */
#define BOB_STOP 0.001F
#define TIP_STOP 0.25F

enum floating_island_mode { ISLAND_STILL, ISLAND_RIDDEN, ISLAND_WAIT };
/* in work->submode: an island the act's set file places, or one another task
 * carries */
enum floating_island_kind { ISLAND_PLACED, ISLAND_CARRIED };

struct floating_island_motion {
    Uint8 unk_00[4];
    NJS_VECTOR tip; /* speed about x and z */
    Uint8 unk_10[36];
    Float start; /* a carried island's rest height when it started */
};

#define VARS(work) FLOATING_ISLAND(work)

static void floating_island_init(struct task *task);
static void floating_island_run(struct task *task);
static void floating_island_draw(struct task *task);
static void floating_island_destroy(struct task *task);

void floating_island_step(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = ISLAND_PLACED;
    floating_island_init(task);
}

void floating_island_carried_step(struct task *task)
{
    struct task_work *work = task->work;
    struct floating_island_motion *motion =
        (struct floating_island_motion *)task->motion;

    work->submode = ISLAND_CARRIED;
    floating_island_init(task);
    work->mode = ISLAND_WAIT;
    motion->start = VARS(work)->rest;
}

static void floating_island_init(struct task *task)
{
    struct task_work *work = task->work;
    struct floating_island_motion *motion =
        (struct floating_island_motion *)task->motion;
    struct task_force *force = task->force;
    NJS_OBJECT *body;

    work->ang.y = 0;
    work->timer = 0;
    work->mode = ISLAND_STILL;
    VARS(work)->rest = work->pos.y;
    VARS(work)->speed = 0.0F;
    motion->tip.x = 0.0F;
    motion->tip.z = 0.0F;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        VARS(work)->body = body = world_object_alloc();
        body->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
        body->scl[0] = 1.0F;
        body->scl[1] = 1.0F;
        body->scl[2] = 1.0F;
        body->ang[0] = 0;
        body->ang[1] = 0;
        body->ang[2] = 0;
        body->pos[0] = work->pos.x;
        body->pos[1] = work->pos.y;
        body->pos[2] = work->pos.z;
        body->model = floating_island_world_object.model;
        body->child = NULL;
        body->sibling = NULL;
        world_add(BODY_WORLD, task, body);
        work->flags |= WORLD_TESTED;
    }
    task->step = floating_island_run;
    task->draw = floating_island_draw;
    task->destructor = floating_island_destroy;
}

/* player 0 landing on the island pushes it down and sets it rocking. a damped
 * spring brings it back to its rest height, and another back level */
static void floating_island_run(struct task *task)
{
    struct task_work *work = task->work;
    struct floating_island_motion *motion =
        (struct floating_island_motion *)task->motion;
    Float speed;
    Float height;
    Float rest;
    Float below;
    Float tip;
    Float ang;
    Float back;
    Float damping;
    Float spring;

    if (work->submode == ISLAND_PLACED) {
        if (placement_try_leave(task)) {
            return;
        }
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ISLAND_WAIT:
            if (motion->start != VARS(work)->rest) {
                work->mode = ISLAND_STILL;
            }
            break;
        case ISLAND_STILL:
            if (player_stands_on(0, task)) {
                NJS_POINT3 player;
                Float x;
                Float z;
                Angle tip_x;
                Angle tip_z;

                work->mode = ISLAND_RIDDEN;
                if (VARS(work)->speed >= -1.0F) {
                    VARS(work)->speed -= 0.5F;
                }
                player_get_history(0, 0, &player, NULL);
                x = player.x - work->pos.x;
                z = player.z - work->pos.z;
                if (z != 0.0F) {
                    tip_x = (Angle)(atan2f(z, TIP_DEPTH) * ANGLE_PER_RADIAN);
                } else {
                    tip_x = 0;
                }
                if (x != 0.0F) {
                    tip_z = (Angle)(atan2f(-x, TIP_DEPTH) * ANGLE_PER_RADIAN);
                } else {
                    tip_z = 0;
                }
                motion->tip.x = tip_x * TIP_SHARE;
                motion->tip.z = tip_z * TIP_SHARE;
            }
            break;
        case ISLAND_RIDDEN:
            if (!player_stands_on(0, task)) {
                work->mode = ISLAND_STILL;
            }
            break;
        }
        speed = VARS(work)->speed;
        height = work->pos.y;
        /* @hack spelling: rest set inside the difference, which keeps the task
         * in R11 and its motion in R12 (0x0c911272, 0x0c91127c); set before it,
         * both move up a register and 31 of 258 words differ */
        below = (rest = VARS(work)->rest) - height;
        if (njAbs(speed) < BOB_STOP && njAbs(below) < BOB_STOP) {
            work->pos.y = rest;
            VARS(work)->body->pos[1] = work->pos.y;
            speed = 0.0F;
        } else {
            height += speed;
            speed += below * 0.005F;
            speed *= 0.94F;
            work->pos.y = height;
            VARS(work)->body->pos[1] = height;
        }
        VARS(work)->speed = speed;
        tip = motion->tip.x;
        damping = 0.95F;
        spring = 0.01F;
        if (njAbs(tip) < TIP_STOP) {
            tip = 0.0F;
            work->ang.x = 0;
            VARS(work)->body->ang[0] = 0;
        } else {
            ang = (Float)work->ang.x;
            back = 0.0F - ang;
            ang += tip;
            tip += back * spring;
            tip *= damping;
            work->ang.x = (Angle)ang;
            VARS(work)->body->ang[0] = (Angle)ang;
        }
        motion->tip.x = tip;
        tip = motion->tip.z;
        if (njAbs(tip) < TIP_STOP) {
            tip = 0.0F;
            work->ang.z = 0;
            VARS(work)->body->ang[2] = 0;
        } else {
            ang = (Float)work->ang.z;
            back = 0.0F - ang;
            ang += tip;
            tip += back * spring;
            tip *= damping;
            work->ang.z = (Angle)ang;
            VARS(work)->body->ang[2] = (Angle)ang;
        }
        motion->tip.z = tip;
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    floating_island_draw(task);
}

static void floating_island_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_tree(&floating_island_object);
        njPopMatrix(1);
    }
}

static void floating_island_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->body != NULL) {
        world_remove(task, VARS(work)->body);
        world_object_free(VARS(work)->body);
    }
}
