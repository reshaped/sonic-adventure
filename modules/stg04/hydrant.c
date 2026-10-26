/* O HYDBASS, a hydrant that a hit knocks off its base while player 0 runs
 * fast. the base then sprays water */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/splash.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg04/hydrant.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL hydrant_base_model;
extern NJS_OBJECT hydrant_object; /* its child's model is the base */

#define HYDRANT_REACH 11.0F

/* a hit knocks the hydrant off while player 0's ground speed is over
 * KNOCK_SPEED. player 0 must also be less than KNOCK_BELOW below the
 * hydrant. the hydrant slides off at player 0's ground speed, held to
 * KNOCK_SPEED_MOST */
#define KNOCK_SPEED 0.65F
#define KNOCK_BELOW 5.0F
#define KNOCK_SPEED_MOST 2.0F

#define SE_KNOCK 120
#define SE_SPRAY 121

#define SPRAY_SCALE 0.2F

/* the standing hydrant's work->mode once it is knocked off, in which only
 * the base draws */
#define HYDRANT_SPRAYING 2

/* the knocked hydrant's modes, in its work->mode. no code sets BODY_GONE, so
 * the knocked hydrant stays in BODY_SLIDE */
enum body_mode { BODY_INIT = 0, BODY_SLIDE = 1, BODY_GONE = 3 };

/* the knocked hydrant counts its steps in TASK_TIMER. it tips
 * BODY_TIP_DEGREES more each step, until it lies BODY_TIPPED over, a quarter
 * turn. its slide slows each step by BODY_SLOWING times the count squared.
 * it draws BODY_LIFT above its position */
#define BODY_TIP_DEGREES 8
#define BODY_TIPPED 0x4000
#define BODY_SLOWING 0.002F
#define BODY_LIFT 1.0F
/* the knocked hydrant draws turned about y by BODY_TURN, not by its heading.
 * BODY_TURN stays 0 until the count passes BODY_TURN_STEPS, then is
 * BODY_TURN_DEGREES over the count */
#define BODY_TURN_STEPS 10
#define BODY_TURN_DEGREES 12.0F

struct body_vars {
    Float tipped; /* 1 once the knocked hydrant lies on its side, else 0 */
};

#define BODY(work) ((struct body_vars *)&(work)->vars)
#define BODY_SPEED(work) ((work)->scl.x)
#define BODY_TURN(work) ((work)->scl.y) /* in degrees */

static void hydrant_stand(struct task *task);
static void hydrant_spray(struct task *task);
static void hydrant_draw(struct task *task);
static void hydrant_destroy(struct task *task);
static void hydrant_body_step(struct task *task);
static void hydrant_body_slide(struct task_work *work);
static void hydrant_body_draw(struct task_work *work);

static struct collision_shape hydrant_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    2.5F,
    10.0F,
    0.0F,
    {0},
    0,
    NULL,
};

void hydrant_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        MODEL(work)->object = &hydrant_object;
        if (!placement_editing(task)) {
            collision_init(task, &hydrant_shape, COLLISION_OBJECT, 1, 1);
            work->collision->radius = HYDRANT_REACH;
        }
        task->destructor = hydrant_destroy;
        task->draw = hydrant_draw;
        task->step = hydrant_stand;
    }
}

/* the standing hydrant's step. a hit while player 0 runs fast knocks the
 * hydrant off its base, away from player 0. the base then sprays */
static void hydrant_stand(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    struct task *body;
    struct task_work *body_work;
    Float speed;
    Float above;

    if (placement_try_leave(task)) {
        return;
    }
    player = player_work[0];
    if (player != NULL && (work->collision->flags & COLLISION_HIT)) {
        work->collision->flags &= ~COLLISION_HIT;
        above = work->pos.y - player->pos.y;
        player_ground_speed(0, &speed);
        if (speed > KNOCK_SPEED && above < KNOCK_BELOW) {
            if ((body = task_create_child(TASK_WORK, hydrant_body_step,
                                          task)) != NULL) {
                body_work = body->work;
                body_work->ang.y = (Angle)(atan2f(work->pos.x - player->pos.x,
                                                  work->pos.z - player->pos.z) *
                                           ANGLE_PER_RADIAN);
                if (speed > KNOCK_SPEED_MOST) {
                    speed = KNOCK_SPEED_MOST;
                }
                BODY_SPEED(body_work) = speed;
                body_work->pos = work->pos;
                task->step = hydrant_spray;
                work->mode = HYDRANT_SPRAYING;
                sound_play(SE_KNOCK, 0, 0, 0);
            }
        }
    }
    if (!placement_editing(task)) {
        collision_register(work);
    }
    hydrant_draw(task);
}

/* step the knocked hydrant, and spray water straight up from the base with
 * its sound */
static void hydrant_spray(struct task *task)
{
    struct task_work *work = task->work;

    /* @bug logic: work->mode is HYDRANT_SPRAYING here, never 1, so the
     * test always holds (CMP/EQ #1,R0 at 0x0c906e62). the knocked hydrant
     * steps every time */
    if (work->mode != 1) {
        task_step_children(task);
    }
    {
        NJS_VECTOR speed = {0.0F, 1.0F, 0.0F};

        splash_create(&work->pos, &speed, SPRAY_SCALE);
    }
    hydrant_draw(task);
    sound_play_at_for(SE_SPRAY, (int)work, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 2);
}

/* on a frame's first step, draw the hydrant, or once it is knocked off, the
 * base alone */
static void hydrant_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    if (work->mode == HYDRANT_SPRAYING) {
        draw_model(&hydrant_base_model);
    } else {
        draw_tree(&hydrant_object);
    }
    njPopMatrix(1);
}

static void hydrant_destroy(struct task *task)
{
}

static void hydrant_body_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BODY_INIT:
        work->mode = BODY_SLIDE;
        hydrant_body_slide(work);
        work->timer = 0;
        break;
    case BODY_SLIDE:
        hydrant_body_slide(work);
        break;
    case BODY_GONE:
        task_kill(task);
        break;
    default:
        work->mode = BODY_INIT;
        break;
    }
    hydrant_body_draw(work);
}

/* slide the knocked hydrant along its heading on the ground, tipping it
 * over and slowing it */
static void hydrant_body_slide(struct task_work *work)
{
    struct angles slope;
    Float ground;

    ground = ground_below(work->pos.x, work->pos.y - 4.0F, work->pos.z, &slope);
    /* tilt about x with the slope, and an angle unit more each step */
    work->ang.x = slope.x + TASK_TIMER(work);
    if (BODY(work)->tipped == 0.0F) {
        if (work->ang.z >= slope.z + BODY_TIPPED) {
            work->ang.z = slope.z + BODY_TIPPED;
            ++BODY(work)->tipped;
        } else {
            work->ang.z =
                slope.z +
                (Angle)(TASK_TIMER(work) * BODY_TIP_DEGREES * ANGLE_PER_DEGREE);
        }
    }
    work->pos.y = ground;
    work->pos.x += BODY_SPEED(work) * njSin(work->ang.y);
    work->pos.z += BODY_SPEED(work) * njCos(work->ang.y);
    if (TASK_TIMER(work) > BODY_TURN_STEPS) {
        BODY_TURN(work) = BODY_TURN_DEGREES / TASK_TIMER(work);
    }
    BODY_SPEED(work) -= TASK_TIMER(work) * TASK_TIMER(work) * BODY_SLOWING;
    if (BODY_SPEED(work) <= 0.0F) {
        BODY_SPEED(work) = 0.0F;
    }
    TASK_TIMER(work)++;
}

/* on a frame's first step, draw the knocked hydrant whole, its base
 * included */
static void hydrant_body_draw(struct task_work *work)
{
    if (frame_step) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + BODY_LIFT, work->pos.z);
    TURN(njRotateY, (Angle)(BODY_TURN(work) * ANGLE_PER_DEGREE));
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    draw_tree(&hydrant_object);
    njPopMatrix(1);
}
