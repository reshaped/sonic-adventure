/* O EGGSEAT seat that rolls along a rail between two fixed stops */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/egg_seat.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

/* @hack type: stage_get_entrance redeclared to return int, which SHC tests as
 * it comes back (TST R0,R0 at 0x0c9256ee); with stage.h's Sint8, SHC
 * sign-extends it first with EXTS.B, the function runs 2 bytes long and 44 of
 * 306 words differ */
#ifdef GCC
#include "stage/stage.h"
#else
extern int stage_get_entrance(void);
#endif

extern NJS_ACTION egg_seat_action;
extern NJS_OBJECT egg_seat_solid_object;
/* @hack type: the wheels as arrays, which has SHC pool each wheel's address
 * and store its angle 28 bytes past it (MOV.L R4,@(28,R2) at 0x0c925a40); as
 * plain objects, SHC pools the address of each angle, and 4 of 17 words
 * differ */
extern NJS_OBJECT egg_seat_wheel0_object[];
extern NJS_OBJECT egg_seat_wheel1_object[];
extern NJS_OBJECT egg_seat_wheel2_object[];
extern NJS_OBJECT egg_seat_wheel3_object[];

/* the surface's flags in the collision world */
#define SOLID_FLAGS (WORLD_MEDIUM | WORLD_MOVES | WORLD_SOLID)
/* player 0's distance within which the collision world tests the surface */
#define TEST_RANGE 40.0F

/* the seat's two stops along x, and its height. the seat stands at them
 * whatever the set entry's position */
#define SEAT_NEAR_STOP 0.0F
#define SEAT_FAR_STOP (-80.0F)
#define SEAT_HEIGHT 98.0F

/* the seat rolls once player 0 has stood on it more than this many steps */
#define SEAT_WAIT 30
#define SEAT_ACCEL 0.05F /* the speed the seat gains each step */
#define SEAT_SPEED 0.4F /* the farthest the seat rolls in a step */
/* the action's last frame, past which the roll loops back to frame 1 */
#define SEAT_LAST_FRAME 13.99F
/* the wheels' turn for each unit the seat rolls along x */
#define WHEEL_TURN 2048.0F

/* side_lift.c's sound for its lift moving, which the seat refreshes each
 * step it rolls */
#define SE_LIFT_MOVE 560

struct egg_seat_vars {
    NJS_OBJECT *solid; /* the surface's object in the collision world */
    NJS_OBJECT *from; /* the model solid copies */
    Float frame; /* the action's frame, which runs while the seat rolls */
};

#define VARS(work) ((struct egg_seat_vars *)&(work)->vars)
/* the stop the seat stands at, or rolls from */
#define STOP(work) ((work)->submode)

enum egg_seat_mode {
    SEAT_INIT,
    SEAT_TO_FAR,
    SEAT_TO_NEAR,
    SEAT_WAIT_RIDER,
    SEAT_STILL /* no code switches to it */
};
enum egg_seat_stop { STOP_FAR, STOP_NEAR };

static void egg_seat_destroy(struct task *task);
static void egg_seat_draw(struct task *task);
static void egg_seat_draw_work(struct task_work *work);
static void egg_seat_dust(struct task_work *work, Float speed);
static void egg_seat_turn_wheels(struct task_work *work);

void egg_carrier_egg_seat_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *solid;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SEAT_INIT:
        work->pos.x = 0.0F;
        work->pos.y = SEAT_HEIGHT;
        work->pos.z = 0.0F;
        if (!placement_editing(task)) {
            VARS(work)->from = &egg_seat_solid_object;
            VARS(work)->solid = solid = world_object_alloc();
            solid->evalflags = NJD_EVAL_BREAK;
            solid->scl[0] = VARS(work)->from->scl[0];
            solid->scl[1] = VARS(work)->from->scl[1];
            solid->scl[2] = VARS(work)->from->scl[2];
            solid->ang[0] = 0;
            solid->ang[1] = 0;
            solid->ang[2] = 0;
            solid->pos[0] = work->pos.x;
            solid->pos[1] = work->pos.y;
            solid->pos[2] = work->pos.z;
            solid->model = VARS(work)->from->model;
            solid->child = NULL;
            solid->sibling = NULL;
            world_add(SOLID_FLAGS, task, solid);
        }
        if (!stage_get_entrance()) {
            STOP(work) = STOP_FAR;
            work->pos.x = SEAT_FAR_STOP;
        } else {
            STOP(work) = STOP_NEAR;
            work->pos.x = SEAT_NEAR_STOP;
        }
        task->destructor = egg_seat_destroy;
        task->draw = egg_seat_draw;
        work->mode = SEAT_WAIT_RIDER;
        break;
    case SEAT_WAIT_RIDER:
        if (player_stands_on(0, task) != 1) {
            TASK_TIMER(work) = 0;
            break;
        }
        if (TASK_TIMER(work)++ < SEAT_WAIT) {
            break;
        }
        work->mode = STOP(work) == STOP_FAR ? SEAT_TO_NEAR : SEAT_TO_FAR;
        break;
    case SEAT_TO_FAR:
        sound_play_for(SE_LIFT_MOVE, (int)work, 1, 0, 2);
        egg_seat_turn_wheels(work);
        egg_seat_dust(work, 0.3F);
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame > SEAT_LAST_FRAME) {
            VARS(work)->frame = 1.0F;
        }
        /* @hack spelling: -SEAT_ACCEL added, an FADD of -0.05 from the pool
         * (0x0c9257dc); with -=, an FSUB of 0.05, and 1 of 306 words
         * differ */
        force->speed.x += -SEAT_ACCEL;
        if (force->speed.x < -SEAT_SPEED) {
            force->speed.x = -SEAT_SPEED;
        }
        work->pos.x += force->speed.x;
        if (work->pos.x > SEAT_FAR_STOP) {
            break;
        }
        force->speed.x = 0.0F;
        work->pos.x = SEAT_FAR_STOP;
        work->mode = SEAT_WAIT_RIDER;
        STOP(work) = STOP_FAR;
        TASK_TIMER(work) = 0;
        break;
    case SEAT_TO_NEAR:
        sound_play_for(SE_LIFT_MOVE, (int)work, 1, 0, 2);
        egg_seat_turn_wheels(work);
        egg_seat_dust(work, 1.5F);
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame > SEAT_LAST_FRAME) {
            VARS(work)->frame = 1.0F;
        }
        force->speed.x += SEAT_ACCEL;
        if (force->speed.x > SEAT_SPEED) {
            force->speed.x = SEAT_SPEED;
        }
        work->pos.x += force->speed.x;
        if (work->pos.x < SEAT_NEAR_STOP) {
            break;
        }
        force->speed.x = 0.0F;
        work->pos.x = SEAT_NEAR_STOP;
        work->mode = SEAT_WAIT_RIDER;
        STOP(work) = STOP_NEAR;
        TASK_TIMER(work) = 0;
        break;
    case SEAT_STILL:
        break;
    }
    VARS(work)->solid->pos[0] = work->pos.x;
    work->flags &= ~WORLD_TESTED;
    if (njDistanceP2P(&work->pos, &player_work[0]->pos) < TEST_RANGE) {
        work->flags |= WORLD_TESTED;
    }
    egg_seat_draw_work(work);
}

static void egg_seat_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

static void egg_seat_draw(struct task *task)
{
    egg_seat_draw_work(task->work);
}

static void egg_seat_draw_work(struct task_work *work)
{
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    texture_set_stage();
    draw_action(&egg_seat_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* raise dust at both sides of the seat, drifting along x at speed */
static void egg_seat_dust(struct task_work *work, Float speed)
{
    NJS_POINT3 pos;
    NJS_VECTOR velocity;

    velocity.x = speed;
    velocity.y = 0.0F;
    velocity.z = 0.0F;
    pos.x = work->pos.x + 30.0F;
    pos.y = work->pos.y + 31.5F;
    pos.z = work->pos.z - 20.0F;
    dust_create(&pos, &velocity, 0.5F);
    pos.z = work->pos.z + 20.0F;
    dust_create(&pos, &velocity, 0.5F);
}

/* turn the wheels by the seat's place along x. the angle goes into the
 * wheels' shared objects, not into a copy */
static void egg_seat_turn_wheels(struct task_work *work)
{
    Angle ang = -(Angle)(work->pos.x * WHEEL_TURN);

    egg_seat_wheel0_object->ang[2] = ang;
    egg_seat_wheel1_object->ang[2] = ang;
    egg_seat_wheel2_object->ang[2] = ang;
    egg_seat_wheel3_object->ang[2] = ang;
}
