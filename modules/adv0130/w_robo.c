/* the walking robot the object list names O W ROBO, which paces back and
 * forth and looks round at each end */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/w_robo.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/models.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the walk action's object, which the robot draws alone as it looks round,
 * its root turned by the look */
extern NJS_OBJECT w_robo_object;
extern NJS_ACTION w_robo_walk_action;

/* the robot's kind, W_ROBO_PLAIN or W_ROBO_SHADOWED, which casts a shadow */
#define W_ROBO_KIND(work) ((work)->tick)
#define W_ROBO_SHADOWED 6
#define W_ROBO_PLAIN 5

/* the walk action's frame wraps to 0 when it reaches WALK_FRAMES */
#define WALK_FRAMES 13.99F

struct w_robo_vars {
    Float frame;
    Float walked; /* the distance walked since the robot last turned */
};

#define VARS(work) ((struct w_robo_vars *)&(work)->vars)

/* the walk, in the task's motion. LENGTH is the distance the robot walks each
 * way, and STRIDE the distance it walks a step. the motion's speed is the
 * step itself, STRIDE long, along the way the robot faces. LOOK is its turn
 * as it looks round, and TILT the lean it turns about with */
#define W_ROBO_LENGTH(motion) ((motion)->slip)
#define W_ROBO_STRIDE(motion) ((motion)->gravity)
#define W_ROBO_LOOK(motion) ((motion)->aim.y)
#define W_ROBO_TILT(motion) ((motion)->aim.z)

/* the steps the robot spends stopping, looking round, turning about and
 * waiting before it walks again */
#define STOP_STEPS 8
#define LOOK_STEPS 57
#define TURN_STEPS 8
#define WAIT_STEPS 5
/* the turn about each step, a sixteenth of a full one, so TURN_STEPS make
 * half a turn */
#define TURN_SPEED 0x1000

enum w_robo_mode {
    W_ROBO_WALK,
    W_ROBO_STOP,
    W_ROBO_LOOK_ROUND,
    W_ROBO_TURN,
    W_ROBO_WAIT
};

static struct collision_shape w_robo_shape[] = {
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 5.0F, 0.0F},
     5.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the robot's turn each step as it looks round. it turns 0x1000 one way in 8
 * steps and holds for 16. it then turns 0x2000 the other way in 16 and holds
 * for 16. last, it turns back to the middle in one */
static Angle w_robo_looks[LOOK_STEPS] = {
#include "adv0130/w_robo/w_robo_looks.inc"
};

static void w_robo_init(struct task *task);
static void w_robo_walk(struct task *task);
static void w_robo_edit(struct task *task);
static void w_robo_draw(struct task *task);
static void w_robo_destroy(struct task *task);

void egg_carrier_w_robo_shadowed_step(struct task *task)
{
    W_ROBO_KIND(task->work) = W_ROBO_SHADOWED;
    w_robo_init(task);
}

void egg_carrier_w_robo_step(struct task *task)
{
    W_ROBO_KIND(task->work) = W_ROBO_PLAIN;
    w_robo_init(task);
}

/* the set entry's scale x is the distance walked each way, scale y the
 * stride, and scale z the walk action's frames a step */
static void w_robo_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_POINT3 step;
    Angle ang;

    if (!placement_editing(task)) {
        collision_init(task, w_robo_shape, COLLISION_OBJECT, 1, 1);
    }
    work->mode = W_ROBO_WALK;
    work->timer = 0;
    VARS(work)->frame = 0.0F;
    VARS(work)->walked = 0.0F;
    W_ROBO_TILT(motion) = 0xf400;
    W_ROBO_LENGTH(motion) = work->scl.x;
    W_ROBO_STRIDE(motion) = work->scl.y;
    step.x = 0.0F;
    step.y = 0.0F;
    step.z = work->scl.y;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &step, &motion->speed);
    njPopMatrix(1);
    if (placement_editing(task)) {
        task->step = w_robo_edit;
    } else {
        task->step = w_robo_walk;
    }
    task->draw = w_robo_draw;
    task->destructor = w_robo_destroy;
}

static void w_robo_walk(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    if (placement_try_leave(task)) {
        return;
    }
    if (event_in_control()) {
        return;
    }
    switch (work->mode) {
    case W_ROBO_WALK:
        if (VARS(work)->walked < W_ROBO_LENGTH(motion)) {
            work->pos.x += motion->speed.x;
            work->pos.y += motion->speed.y;
            work->pos.z += motion->speed.z;
            VARS(work)->frame += work->scl.z;
            if (VARS(work)->frame >= WALK_FRAMES) {
                VARS(work)->frame = 0.0F;
            }
            VARS(work)->walked += W_ROBO_STRIDE(motion);
        } else {
            VARS(work)->walked = 0.0F;
            work->timer = 0;
            work->mode = W_ROBO_STOP;
        }
        break;
    case W_ROBO_STOP:
        ++work->timer;
        if (TASK_TIMER(work) >= STOP_STEPS) {
            work->timer = 0;
            W_ROBO_LOOK(motion) = 0;
            work->mode = W_ROBO_LOOK_ROUND;
        }
        break;
    case W_ROBO_LOOK_ROUND:
        if (TASK_TIMER(work) < LOOK_STEPS) {
            W_ROBO_LOOK(motion) += w_robo_looks[TASK_TIMER(work)];
            ++work->timer;
        } else {
            work->timer = 0;
            work->mode = W_ROBO_TURN;
        }
        break;
    case W_ROBO_TURN:
        if (TASK_TIMER(work) < TURN_STEPS) {
            work->ang.y += TURN_SPEED;
            ++work->timer;
        } else {
            work->timer = 0;
            work->mode = W_ROBO_WAIT;
            motion->speed.x = -motion->speed.x;
            motion->speed.y = -motion->speed.y;
            motion->speed.z = -motion->speed.z;
        }
        break;
    case W_ROBO_WAIT:
        ++work->timer;
        if (TASK_TIMER(work) >= WAIT_STEPS) {
            work->timer = 0;
            work->mode = W_ROBO_WALK;
        }
        break;
    }
    collision_register(work);
    w_robo_draw(task);
}

static void w_robo_edit(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame += work->scl.z;
    if (VARS(work)->frame >= WALK_FRAMES) {
        VARS(work)->frame = 0.0F;
    }
    njPrintC(NJM_LOCATION(17, 19), "-> MOVE RANGE");
    njPrintC(NJM_LOCATION(17, 20), "-> MOVE SPEED");
    njPrintC(NJM_LOCATION(17, 21), "-> ANIME SPEED");
    w_robo_draw(task);
}

static void w_robo_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct angles slope;
    Angle ang;
    Float ground;
    Float scale;

    texture_set_stage();
    switch (work->mode) {
    case W_ROBO_LOOK_ROUND:
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        /* @hack spelling: the two writes of evalflags written out as x = x
         * & y and x = x | y, which keep motion in R10 and w_robo_object's
         * address in R11 (0x0c927580, 0x0c9275d2); as &= or |=, either one,
         * the two trade and 15 of 230 words differ */
        w_robo_object.evalflags = w_robo_object.evalflags & ~NJD_EVAL_UNIT_ANG;
        w_robo_object.ang[1] = W_ROBO_LOOK(motion);
        draw_tree(&w_robo_object);
        w_robo_object.evalflags = w_robo_object.evalflags | NJD_EVAL_UNIT_ANG;
        w_robo_object.ang[1] = 0;
        njPopMatrix(1);
        break;
    case W_ROBO_TURN:
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, W_ROBO_TILT(motion), ang);
        draw_action(&w_robo_walk_action, 0.0F);
        njPopMatrix(1);
        break;
    default:
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(&w_robo_walk_action, VARS(work)->frame);
        njPopMatrix(1);
        break;
    }
    if (W_ROBO_KIND(work) == W_ROBO_SHADOWED) {
        ground =
            ground_below(work->pos.x, work->pos.y, work->pos.z, &slope) + 0.1F;
        scale = 1.2F / (1.0F + (0.03F * _builtin_fabsf(ground - work->pos.y)));
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, ground, work->pos.z);
        TURN_VIA(njRotateZ, slope.z, ang);
        TURN_VIA(njRotateX, slope.x, ang);
        njScale(NULL, scale, scale, scale);
        draw_tree(shadow_object);
        njPopMatrix(1);
    }
}

static void w_robo_destroy(struct task *task)
{
}
