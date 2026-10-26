/* O HATAKI, an arm whose rockets swing it over its hinge. it hurts players
 * as it comes down on the far side, and swings back after a wait */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/flame.h"
#include "object/placement.h"
#include "stg05/swatter.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_MODEL swatter_arm_model;
extern NJS_MODEL swatter_hinge_model;

enum swatter_mode {
    SWATTER_REST,
    SWATTER_BURST,
    SWATTER_SWING,
    SWATTER_OVER,
    SWATTER_BACK
};

/* in work->submode during a burst: whether the rockets fire or pause */
#define ROCKETS_FIRE 5
#define ROCKETS_PAUSE 6

/* TASK_TIMER(work) counts the steps of each rest, burst and wait.
 * task->motion->speed holds the center of the arm's box, ARM_REACH along x,
 * before the box's angles turn it */
struct swatter_vars {
    Uint8 unk_00[4];
    Float tilt; /* about the hinge, in angle units, 0 when level */
    Float swing; /* the tilt's change each step */
};

#define VARS(work) ((struct swatter_vars *)&(work)->vars)
#define BURSTS(work) ((work)->tick)
/* the set entry's scales. PUSH is the rockets' push on the swing each step.
 * BACK_SPEED is the tilt the arm takes back each step after its wait. WEIGHT is
 * its weight against the push */
#define PUSH(work) ((work)->scl.x)
#define BACK_SPEED(work) ((work)->scl.y)
#define WEIGHT(work) ((work)->scl.z)

/* the rest ends once TASK_TIMER(work) passes REST_STEPS, and the wait on
 * the far side once it passes OVER_STEPS */
#define REST_STEPS 10
#define OVER_STEPS 50

#define TILT_UPRIGHT 16384.0F /* a quarter turn: the arm stands upright */
#define TILT_HURTS 27306.7F /* 150 degrees, past which the arm's box hurts */
#define TILT_OVER 32768.0F /* half a turn: the arm lies on its far side */

/* the center of the arm's box, along x before the box's angles turn it */
#define ARM_REACH 100.0F

/* O HATAKI destroys itself once neither the camera nor player 0 is within
 * 1000 of it. the range is squared, as placement_try_leave_by takes it */
#define LEAVE_RANGE (1000.0F * 1000.0F)
#define DRAW_RADIUS 500.0F

/* the rockets' nozzles, and their flames' speeds in a burst and in the
 * swing. all three are in the arm's space */
static NJS_POINT3 swatter_nozzles[2] = {
    {30.0F, -10.0F, -175.0F},
    {-30.0F, -10.0F, -175.0F},
};
static NJS_VECTOR swatter_burst_speed = {0.0F, -2.0F, 0.0F};
static NJS_VECTOR swatter_swing_speed = {0.0F, -0.5F, 0.0F};

/* the bursts: the rockets fire, pause, then fire again. a burst ends once
 * TASK_TIMER(work) passes its entry. no step reads the fourth */
static Uint8 swatter_bursts[4] = {10, 20, 2, 10};

/* the arm's box, which hurts players while its attack level is set, and the
 * hinge's drum */
static struct collision_shape swatter_shapes[2] = {
    {
        SHAPE_RECTANGLE2 | SHAPE_HURTABLE,
        SHAPE_HURTS_PLAYERS,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        70.0F,
        2.0F,
        30.0F,
        {0, 0, 0},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        12.0F,
        32.0F,
        0.0F,
        {0, 0, 0x4000},
        0,
        NULL,
    },
};

static void swatter_run(struct task *task);
static void swatter_draw(struct task *task);
static void swatter_destroy(struct task *task);
static void swatter_flames(struct task_work *work, NJS_VECTOR *speed,
                           Float scale);

/* start the arm level and resting. unless the set editor holds the arm, give it
 * its box and the hinge's drum. the box turns a quarter turn about y, then
 * about z by the tilt. that puts it where the arm draws */
void swatter_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_POINT3 center;
    Angle ang;

    work->ang.x = 0;
    work->ang.z = 0;
    TASK_TIMER(work) = 0;
    BURSTS(work) = 0;
    VARS(work)->tilt = 0.0F;
    VARS(work)->swing = 0.0F;
    work->mode = SWATTER_REST;
    motion->speed.x = ARM_REACH;
    motion->speed.y = 0.0F;
    motion->speed.z = 0.0F;
    if (!placement_editing(task)) {
        collision_init(task, swatter_shapes, COLLISION_OBJECT, 1, 2);
        work->collision->shapes->ang.y = 0x4000;
        work->collision->shapes->ang.z = (Angle)VARS(work)->tilt;
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN_VIA(njRotateY, work->collision->shapes->ang.y, ang);
        TURN_VIA(njRotateZ, work->collision->shapes->ang.z, ang);
        njCalcPoint(NULL, &motion->speed, &center);
        njPopMatrix(1);
        work->collision->shapes->center = center;
    }
    task->step = swatter_run;
    task->draw = swatter_draw;
    task->destructor = swatter_destroy;
}

/* in SWATTER_SWING, VARS(work)->swing grows each step by PUSH(work), less
 * WEIGHT(work) times the absolute cosine of the tilt */
static void swatter_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_POINT3 center;
    Angle ang;

    if (placement_try_leave_by(task, LEAVE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case SWATTER_REST:
        if (TASK_TIMER(work) > REST_STEPS) {
            work->mode = SWATTER_BURST;
            work->submode = ROCKETS_FIRE;
            TASK_TIMER(work) = 0;
            BURSTS(work) = 0;
        } else {
            ++TASK_TIMER(work);
        }
        break;
    case SWATTER_BURST:
        if (TASK_TIMER(work) > swatter_bursts[BURSTS(work)]) {
            TASK_TIMER(work) = 0;
            ++BURSTS(work);
            if (BURSTS(work) >= 3) {
                VARS(work)->swing = 0.0F;
                TASK_TIMER(work) = 0;
                BURSTS(work) = 0;
                work->mode = SWATTER_SWING;
                work->submode = 0;
            } else {
                work->submode = work->submode == ROCKETS_FIRE ? ROCKETS_PAUSE
                                                              : ROCKETS_FIRE;
            }
        } else {
            ++TASK_TIMER(work);
            if (work->submode == ROCKETS_FIRE) {
                swatter_flames(work, &swatter_burst_speed, 1.5F);
            }
        }
        break;
    case SWATTER_SWING:
        if (VARS(work)->tilt < TILT_UPRIGHT) {
            VARS(work)->swing +=
                PUSH(work) - (njCos((Angle)VARS(work)->tilt) * WEIGHT(work));
        } else {
            VARS(work)->swing +=
                PUSH(work) + (njCos((Angle)VARS(work)->tilt) * WEIGHT(work));
        }
        VARS(work)->tilt += VARS(work)->swing;
        if (VARS(work)->tilt > TILT_HURTS) {
            work->collision->shapes->levels =
                (work->collision->shapes->levels & ~SHAPE_ATTACK) | ATTACK(2);
            if (VARS(work)->tilt > TILT_OVER) {
                VARS(work)->tilt = TILT_OVER;
                work->mode = SWATTER_OVER;
                work->collision->shapes->levels =
                    (work->collision->shapes->levels & ~SHAPE_ATTACK) |
                    ATTACK(0);
            }
        }
        swatter_flames(work, &swatter_swing_speed, 1.5F);
        break;
    case SWATTER_OVER:
        if (TASK_TIMER(work) > OVER_STEPS) {
            TASK_TIMER(work) = 0;
            work->mode = SWATTER_BACK;
        } else {
            ++TASK_TIMER(work);
        }
        break;
    case SWATTER_BACK:
        VARS(work)->tilt -= BACK_SPEED(work);
        if (VARS(work)->tilt < 0.0F) {
            VARS(work)->tilt = 0.0F;
            work->mode = SWATTER_REST;
        }
        break;
    }
    if (!placement_editing(task)) {
        work->collision->shapes->ang.z = (Angle)VARS(work)->tilt;
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN_VIA(njRotateY, work->collision->shapes->ang.y, ang);
        TURN_VIA(njRotateZ, work->collision->shapes->ang.z, ang);
        njCalcPoint(NULL, &motion->speed, &center);
        njPopMatrix(1);
        work->collision->shapes->center = center;
        collision_register(work);
    }
    swatter_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ROCKET POWER");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> BACK ROT SPEED");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> HATAKI WEIGHT");
    }
}

static void swatter_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&swatter_hinge_model);
        TURN_VIA(njRotateX, (Angle)VARS(work)->tilt, ang);
        draw_model(&swatter_arm_model);
        njPopMatrix(1);
    }
}

static void swatter_destroy(struct task *task)
{
}

/* create a flame from each nozzle, moving at speed in the arm's space */
static void swatter_flames(struct task_work *work, NJS_VECTOR *speed,
                           Float scale)
{
    NJS_POINT3 pos;
    NJS_VECTOR flame_speed;
    Uint8 i;
    Angle ang;

    for (i = 0; i < 2; ++i) {
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, (Angle)VARS(work)->tilt, ang);
        njCalcPoint(NULL, &swatter_nozzles[i], &pos);
        njCalcPoint(NULL, speed, &flame_speed);
        pos.x += work->pos.x;
        pos.y += work->pos.y;
        pos.z += work->pos.z;
        njPopMatrix(1);
        flame_create(&pos, &flame_speed, scale);
    }
}
