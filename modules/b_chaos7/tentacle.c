/* O TENTACLE: the Perfect Chaos fight's tentacle, which rises from the sea,
 * lashes and sinks */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/tentacle.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION tentacle_action;
extern NJS_MOTION tentacle_shape_motion;

/* a tentacle destroys itself once neither the camera nor player 0 is within
 * 4010. the range is squared, as placement_try_leave_by takes it */
#define TENTACLE_RANGE (4010.0F * 4010.0F)

/* a waiting tentacle fades in once player 0 comes within TENTACLE_NEAR. its
 * first collision record reaches TENTACLE_REACH */
#define TENTACLE_NEAR 1000.0F
#define TENTACLE_REACH 150.0F

/* the lash's motion plays TENTACLE_PACE frames a step. the tentacle's alpha
 * moves by TENTACLE_FADE a step as it fades in and out */
#define TENTACLE_PACE 0.1F
#define TENTACLE_FADE 0.1F

struct tentacle_vars {
    NJS_ACTION *action;
    Float frame;
    NJS_MOTION *shape;
};

#define VARS(work) ((struct tentacle_vars *)&(work)->vars)
/* in work->scl: the lash's pace, which TENTACLE_INIT sets to TENTACLE_PACE,
 * and the constant material's alpha, an offset. the alpha rises from -1, which
 * clears the tentacle, to 1 as it fades in. it falls back to -1 as it fades
 * out */
#define TENTACLE_PACE_OF(work) ((work)->scl.x)
#define TENTACLE_ALPHA(work) ((work)->scl.y)

/* nothing sets TENTACLE_IDLE */
enum tentacle_mode {
    TENTACLE_INIT,
    TENTACLE_WAITING,
    TENTACLE_FADING_IN,
    TENTACLE_LASHING,
    TENTACLE_FADING_OUT,
    TENTACLE_IDLE
};

static const NJS_ARGB tentacle_color = {1.0F, 1.0F, 1.0F, 1.0F};

/* the lash's two capsules, with no size or place. each collision record takes
 * its own copy, which tentacle_follow sizes and places as the lash plays */
static struct collision_shape tentacle_shapes[2] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     0.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     0.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* size and place the record's two capsules, the lower and then the upper, for
 * the lash's motion frame. work->submode counts the whole frames passed, at
 * most one a step. a field a case doesn't set keeps an earlier case's value */
static void tentacle_follow(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = task->work->collision;
    struct collision_shape *shape = collision->shapes;

    switch (work->submode) {
    case 0:
        if (VARS(work)->frame >= 1.0F) {
            work->submode = 1;
        }
        break;
    case 1:
        if (VARS(work)->frame >= 2.0F) {
            work->submode = 2;
        }
        shape[0].radius = 2.5F;
        break;
    case 2:
        if (VARS(work)->frame >= 3.0F) {
            work->submode = 3;
        }
        shape[0].center.y = 5.0F;
        shape[0].size_y = 5.0F;
        shape[1].center.y = 10.0F;
        shape[1].radius = 5.0F;
        break;
    case 3:
        if (VARS(work)->frame >= 4.0F) {
            work->submode = 4;
        }
        shape[0].center.y = 10.0F;
        shape[0].radius = 5.0F;
        shape[0].size_y = 5.0F;
        shape[0].ang.x = 0xf700;
        shape[1].center.y = 25.0F;
        shape[1].center.z = -2.5F;
        shape[1].radius = 7.5F;
        break;
    case 4:
        if (VARS(work)->frame >= 5.0F) {
            work->submode = 5;
        }
        shape[0].center.y = 20.0F;
        shape[0].center.z = -2.5F;
        shape[0].radius = 5.0F;
        shape[0].size_y = 10.0F;
        shape[0].ang.x = 0xf000;
        shape[1].center.y = 40.0F;
        shape[1].center.z = -10.0F;
        shape[1].radius = 10.0F;
        break;
    case 5:
        if (VARS(work)->frame >= 6.0F) {
            work->submode = 6;
        }
        shape[0].center.y = 30.0F;
        shape[0].center.z = -5.0F;
        shape[0].size_y = 15.0F;
        shape[1].center.y = 55.0F;
        shape[1].center.z = -22.5F;
        shape[1].radius = 12.5F;
        break;
    case 6:
        if (VARS(work)->frame >= 7.0F) {
            work->submode = 7;
        }
        shape[0].center.y = 35.0F;
        shape[0].center.z = -7.5F;
        shape[0].size_y = 20.0F;
        shape[1].center.y = 65.0F;
        shape[1].center.z = -35.0F;
        shape[1].size_y = 12.5F;
        shape[1].ang.x = 0xd800;
        break;
    case 7:
        if (VARS(work)->frame >= 8.0F) {
            work->submode = 8;
        }
        shape[1].center.y = 72.5F;
        shape[1].center.z = -50.0F;
        shape[1].size_y = 17.5F;
        shape[1].ang.x = 0xd000;
        break;
    case 8:
        if (VARS(work)->frame >= 9.0F) {
            work->submode = 9;
        }
        shape[0].size_y = 25.0F;
        shape[1].center.y = 77.5F;
        shape[1].center.z = -57.5F;
        shape[1].size_y = 25.0F;
        shape[1].ang.x = 0xd000;
        break;
    case 9:
        if (VARS(work)->frame >= 10.0F) {
            work->submode = 10;
        }
        shape[0].size_y = 30.0F;
        shape[1].center.y = 80.0F;
        shape[1].center.z = -65.0F;
        shape[1].size_y = 30.0F;
        shape[1].ang.x = 0xcd00;
        break;
    case 10:
        if (VARS(work)->frame >= 11.0F) {
            work->submode = 11;
        }
        shape[0].size_y = 35.0F;
        shape[1].center.y = 85.0F;
        shape[1].center.z = -75.0F;
        shape[1].size_y = 40.0F;
        shape[1].ang.x = 0xca00;
        break;
    case 11:
        if (VARS(work)->frame >= 12.0F) {
            work->submode = 12;
        }
        shape[0].center.y = 40.0F;
        shape[0].size_y = 40.0F;
        shape[1].center.y = 90.0F;
        break;
    case 12:
        if (VARS(work)->frame >= 13.0F) {
            work->submode = 13;
        }
        shape[0].center.z = -5.0F;
        shape[0].ang.x = 0xf600;
        shape[1].center.y = 110.0F;
        shape[1].center.z = -70.0F;
        shape[1].size_y = 45.0F;
        shape[1].ang.x = 0xd100;
        break;
    case 13:
        if (VARS(work)->frame >= 14.0F) {
            work->submode = 14;
        }
        shape[0].center.y = 55.0F;
        shape[0].center.z = 2.5F;
        shape[0].size_y = 50.0F;
        shape[0].ang.x = 0;
        shape[1].center.y = 140.0F;
        shape[1].center.z = -20.0F;
        shape[1].radius = 15.0F;
        shape[1].size_y = 30.0F;
        shape[1].ang.x = 0xf000;
        break;
    case 14:
        if (VARS(work)->frame >= 15.0F) {
            work->submode = 15;
        }
        shape[0].center.z = 15.0F;
        shape[0].size_y = 50.0F;
        shape[0].ang.x = 0x1000;
        shape[1].center.y = 140.0F;
        shape[1].center.z = 75.0F;
        shape[1].radius = 15.0F;
        shape[1].size_y = 30.0F;
        shape[1].ang.x = 0x2300;
        break;
    case 15:
        if (VARS(work)->frame >= 16.0F) {
            work->submode = 16;
        }
        shape[0].center.y = 35.0F;
        shape[0].size_y = 40.0F;
        shape[0].ang.x = 0x1a00;
        shape[1].center.y = 65.0F;
        shape[1].center.z = 90.0F;
        shape[1].radius = 10.0F;
        shape[1].size_y = 40.0F;
        shape[1].ang.x = 0x4200;
        break;
    case 16:
        if (VARS(work)->frame >= 17.0F) {
            work->submode = 17;
        }
        shape[0].center.y = 30.0F;
        shape[0].size_y = 30.0F;
        shape[0].ang.x = 0x2200;
        shape[1].center.y = 25.0F;
        shape[1].center.z = 90.0F;
        shape[1].radius = 7.5F;
        shape[1].size_y = 40.0F;
        shape[1].ang.x = 0x5a00;
        break;
    case 17:
        if (VARS(work)->frame >= 18.0F) {
            work->submode = 18;
        }
        shape[0].center.y = 20.0F;
        shape[0].size_y = 30.0F;
        shape[0].ang.x = 0x2800;
        shape[1].center.y = 20.0F;
        shape[1].center.z = 75.0F;
        shape[1].radius = 7.5F;
        shape[1].size_y = 25.0F;
        shape[1].ang.x = 0x5d00;
        break;
    case 18:
        if (VARS(work)->frame >= 19.0F) {
            work->submode = 19;
        }
        shape[0].center.y = 12.5F;
        shape[0].center.z = 10.0F;
        shape[0].size_y = 15.0F;
        shape[1].center.y = 15.0F;
        shape[1].center.z = 55.0F;
        shape[1].radius = 5.0F;
        shape[1].ang.x = 0x4e00;
        break;
    case 19:
        shape[0].center.y = 10.0F;
        shape[0].center.z = 5.0F;
        shape[0].size_y = 10.0F;
        shape[1].center.y = 10.0F;
        shape[1].center.z = 40.0F;
        shape[1].ang.x = 0x4b00;
        break;
    }
}

static void tentacle_destroy(struct task *task)
{
    task_kill(task);
}

static void tentacle_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    Angle ang;

    color = tentacle_color;
    texture_set_stage();
    color.a = TENTACLE_ALPHA(work);
    njSetConstantMaterial(&color);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_shape_motion(VARS(work)->action->object, VARS(work)->action->motion,
                      VARS(work)->shape, VARS(work)->frame);
    njPopMatrix(1);
    color.a = color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
}

/* wait until player 0 is within TENTACLE_NEAR, then fade in and lash. fade
 * out at the lash's last frame, then wait again */
void chaos7_tentacle_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    NJS_MOTION *shape;
    Float frames;

    if (placement_try_leave_by(task, TENTACLE_RANGE)) {
        return;
    }
    shape = &tentacle_shape_motion;
    action = &tentacle_action;
    if (edit_mode) {
        VARS(work)->action = action;
        VARS(work)->shape = shape;
        tentacle_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), ">");
        }
    } else {
        switch (work->mode) {
        case TENTACLE_INIT:
            /* @hack redundant: two of the three stores of TENTACLE_WAITING,
             * which SHC drops, but which load its 1 into R4 early (MOV #1,R4 in
             * the BT/S slot at 0x0c91d31c); with one store, 26 of 211 words
             * differ */
            work->mode = TENTACLE_WAITING;
            work->mode = TENTACLE_WAITING;
            work->mode = TENTACLE_WAITING;
            work->ang.x = work->ang.z = 0;
            TENTACLE_PACE_OF(work) = TENTACLE_PACE;
            TENTACLE_ALPHA(work) = work->scl.z = 0.0F;
            VARS(work)->action = action;
            VARS(work)->shape = shape;
            task->destructor = tentacle_destroy;
            task->draw = tentacle_draw;
            collision_init(task, tentacle_shapes, COLLISION_OBJECT, 1, 2);
            work->collision->radius = TENTACLE_REACH;
            break;
        case TENTACLE_WAITING:
            if (njDistanceP2P(&work->pos, &player_work[0]->pos) <
                TENTACLE_NEAR) {
                work->mode = TENTACLE_FADING_IN;
                TENTACLE_ALPHA(work) = -1.0F;
                VARS(work)->frame = 0.0F;
            }
            break;
        case TENTACLE_FADING_IN:
            if ((TENTACLE_ALPHA(work) += TENTACLE_FADE) > 1.0F) {
                work->mode = TENTACLE_LASHING;
                TENTACLE_ALPHA(work) = 1.0F;
            }
            tentacle_draw(task);
            break;
        case TENTACLE_LASHING:
            frames = VARS(work)->action->motion->nbFrame;
            if ((VARS(work)->frame += TENTACLE_PACE_OF(work)) >= frames) {
                work->mode = TENTACLE_FADING_OUT;
                VARS(work)->frame = frames;
            }
            tentacle_follow(task);
            tentacle_draw(task);
            collision_register(work);
            break;
        case TENTACLE_FADING_OUT:
            if ((TENTACLE_ALPHA(work) -= TENTACLE_FADE) < -1.0F) {
                TENTACLE_ALPHA(work) = -1.0F;
                work->mode = TENTACLE_WAITING;
                work->submode = 0;
                /* @bug memory: leaks the lash's record and its shapes. this
                 * second collision_init gives a work that has a record a new
                 * one, which collision.h's contract forbids, and nothing frees
                 * the old (JSR at 0x0c91d4c6) */
                /* @bug logic: from the second lash, the checks test another
                 * record only within 25 plus its radius. the new record's
                 * radius is 25, which collision_set_radius gives shapes of no
                 * size, and not TENTACLE_REACH: TENTACLE_INIT stores that
                 * after its call (FMOV.S FR3,@(R0,R3) at 0x0c91d37a), and
                 * nothing does after this one */
                collision_init(task, tentacle_shapes, COLLISION_OBJECT, 1, 2);
            }
            tentacle_draw(task);
            break;
        case TENTACLE_IDLE:
            break;
        }
    }
}
