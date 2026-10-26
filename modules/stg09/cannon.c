/* HoudaiL and HoudaiR: cannons on the tables, which fire confetti */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/model.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/cannon.h"
#include "stg09/casino.h"
#include "stg09/pinball.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL cannon_model;
/* a piece of confetti's two faces. each step, CONFETTI_FACE in its random
 * spin picks the face it shows */
extern NJS_MODEL confetti_model_0;
extern NJS_MODEL confetti_model_1;

/* the sides cannon_fire takes. cannon_left_step and cannon_right_step keep
 * the side in work->mode too, which nothing reads */
#define CANNON_LEFT 0
#define CANNON_RIGHT 1

/* in the cannon's work->flags. the paused draw sets it, and the step clears
 * it. the confetti holds still while it is set */
#define CANNON_HELD 0x100

#define CANNON_SHOTS 50 /* the pieces of confetti each firing makes */
#define SE_CANNON 28

/* work->tick counts a piece of confetti's steps. on ticks 0 to 9 it waits
 * where it was fired. it then flies along work->scl at its speed. from tick
 * CONFETTI_FALL, it falls at its speed scaled by CONFETTI_DRAG. as it falls,
 * it drifts 1 a step toward its spin */
#define CONFETTI_FALL 20
#define CONFETTI_DRAG 0.05F
/* the height a falling piece is killed at, whatever the cannon's */
#define CONFETTI_FLOOR (-10.0F)
#define CONFETTI_FACE 4 /* the spin's bit that picks a piece's second face */
/* the height above the cannon a piece is fired from, and the range of its
 * random speed */
#define CONFETTI_HEIGHT 5.0F
#define CONFETTI_SLOWEST 8.0F
#define CONFETTI_FASTEST 28.0F

struct confetti_vars {
    Float speed;
    Angle spin; /* a random angle each step: the piece's turn, face and drift */
};

#define VARS(work) ((struct confetti_vars *)&(work)->vars)

static void cannon_destroy(struct task *task)
{
}

static void confetti_draw(struct task_work *work)
{
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, VARS(work)->spin, ang);
    if (!(VARS(work)->spin & CONFETTI_FACE)) {
        draw_model(&confetti_model_0);
    } else {
        draw_model(&confetti_model_1);
    }
    njPopMatrix(1);
}

static void confetti_run(struct task *task)
{
    struct task_work *cannon = task->parent->work;
    struct task_work *work = task->work;

    if (!(cannon->flags & CANNON_HELD)) {
        VARS(work)->spin = RANDOM_ANGLE();
        switch (work->tick) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            ++work->tick;
            break;
        case CONFETTI_FALL:
            if (work->pos.y > CONFETTI_FLOOR) {
                work->pos.x += njSin(VARS(work)->spin);
                work->pos.z += njCos(VARS(work)->spin);
                work->pos.y -= VARS(work)->speed;
            } else {
                task_kill(task);
            }
            break;
        default:
            work->pos.x += work->scl.x * VARS(work)->speed;
            work->pos.y += work->scl.y * VARS(work)->speed;
            work->pos.z += work->scl.z * VARS(work)->speed;
            if (++work->tick == CONFETTI_FALL) {
                VARS(work)->speed *= CONFETTI_DRAG;
            }
            break;
        }
    }
    confetti_draw(work);
}

/* fire a piece of confetti. a piece from CANNON_LEFT's side flies against the
 * way the cannon's angles face. one from CANNON_RIGHT's flies along it */
static void cannon_fire(struct task *task, int side)
{
    struct task_work *cannon = task->work;
    struct task_work *work;

    work = task_create_child(TASK_WORK, confetti_run, task)->work;
    work->pos.x = cannon->pos.x;
    work->pos.y = cannon->pos.y + CONFETTI_HEIGHT;
    work->pos.z = cannon->pos.z;
    if (side == CANNON_LEFT) {
        work->scl.x = -njSin(cannon->ang.y);
        work->scl.y = -njSin(cannon->ang.x);
        work->scl.z = -njCos(cannon->ang.y);
    } else {
        work->scl.x = njSin(cannon->ang.y);
        work->scl.y = njSin(cannon->ang.x);
        work->scl.z = njCos(cannon->ang.y);
    }
    VARS(work)->speed = RANDOM(CONFETTI_SLOWEST, CONFETTI_FASTEST);
    work->tick = 0;
}

static void cannon_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    draw_model(&cannon_model);
    njPopMatrix(1);
    task_step_children(task);
}

static void cannon_draw_held(struct task *task)
{
    task->work->flags |= CANNON_HELD;
    cannon_draw(task);
}

static void cannon_left_run(struct task *task)
{
    int i;

    task->work->flags &= ~CANNON_HELD;
    cannon_draw(task);
    if ((casino_flags & CASINO_HOUDAI_L_FIRES) &&
        (player_work[0]->flags & PLAYER_ON_LAND)) {
        for (i = 0; i < CANNON_SHOTS; ++i) {
            cannon_fire(task, CANNON_LEFT);
        }
        sound_play(SE_CANNON, 0, 0, 0);
        casino_flags &= ~CASINO_HOUDAI_L_FIRES;
    }
}

static void cannon_right_run(struct task *task)
{
    int i;

    task->work->flags &= ~CANNON_HELD;
    cannon_draw(task);
    if ((casino_flags & CASINO_HOUDAI_R_FIRES) &&
        (player_work[0]->flags & PLAYER_ON_LAND)) {
        for (i = 0; i < CANNON_SHOTS; ++i) {
            cannon_fire(task, CANNON_RIGHT);
        }
        sound_play(SE_CANNON, 0, 0, 0);
        casino_flags &= ~CASINO_HOUDAI_R_FIRES;
    }
}

void cannon_left_step(struct task *task)
{
    task->work->mode = CANNON_LEFT;
    task->step = cannon_left_run;
    task->draw = cannon_draw_held;
    task->destructor = cannon_destroy;
}

void cannon_right_step(struct task *task)
{
    task->work->mode = CANNON_RIGHT;
    task->step = cannon_right_run;
    task->draw = cannon_draw_held;
    task->destructor = cannon_destroy;
}
