/* workshop door that swings open a quarter turn as the nearest player comes
 * near */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/texlists.h"
#include "adv02/workshop_door.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL workshop_door_model;

/* where the door stands, and its y angle shut */
#define DOOR_X 1432.5F
#define DOOR_Y 201.0F
#define DOOR_Z 809.5F
#define SHUT_ANG (-6990)
/* outside the set editor, the step draws and runs the door only while the
 * camera is within RUN_RANGE of it */
#define RUN_RANGE 400.0F

/* the nearest player opens the door from within 30 units of it. OPEN_REACH is
 * 30 squared */
#define OPEN_REACH (30.0F * 30.0F)

/* how far the door turns out from shut: a quarter turn, in Ninja's angle
 * units */
#define OPEN_SWING 16384.0F
/* the moving door speeds up by SPEED_UP a step, up to TOP_SPEED. once it
 * brakes, it slows down by SLOW_DOWN a step, to no less than LOW_SPEED */
#define SPEED_UP 40.0F
#define TOP_SPEED 768.0F
#define SLOW_DOWN 30.0F
#define LOW_SPEED 112.0F
/* the opening door brakes once it is OPENING_BRAKE of OPEN_SWING out from shut,
 * and the shutting door once it is SHUTTING_BRAKE out */
#define OPENING_BRAKE 0.65F
#define SHUTTING_BRAKE 0.35F

enum workshop_door_mode { DOOR_INIT, DOOR_RUN };

struct workshop_door_vars {
    Float speed; /* the door's speed a step, below 0 while it shuts */
    Sint16 shut_ang; /* the door's y angle shut */
    Float swing; /* how far the door has turned out from shut */
};

#define VARS(work) ((struct workshop_door_vars *)&(work)->vars)
/* TRUE while the nearest player is within OPEN_REACH, so the door opens, else
 * FALSE */
#define OPENING(work) ((work)->submode)

static void workshop_door_init(struct task *task);
static void workshop_door_reset(struct task *task);
static void workshop_door_update(struct task *task);
static void workshop_door_draw(struct task *task);
static void workshop_door_destroy(struct task *task);

/* a box along the door out from its hinge, which nothing reads */
static struct collision_shape workshop_door_shape[1] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-7.5F, 10.0F, -2.5F},
     7.5F,
     10.0F,
     2.5F,
     {0},
     0,
     NULL},
};

void workshop_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case DOOR_INIT:
            workshop_door_init(task);
            break;
        case DOOR_RUN:
            if (njDistanceP2P(&camera_work->pos, &work->pos) <= RUN_RANGE) {
                workshop_door_draw(task);
                workshop_door_update(task);
            }
            break;
        }
    } else {
        switch (work->mode) {
        case DOOR_INIT:
            workshop_door_init(task);
            break;
        case DOOR_RUN:
            workshop_door_draw(task);
            workshop_door_update(task);
            break;
        }
    }
}

static void workshop_door_init(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x = DOOR_X;
    work->pos.y = DOOR_Y;
    work->pos.z = DOOR_Z;
    work->ang.y = SHUT_ANG;
    task->draw = workshop_door_draw;
    task->destructor = workshop_door_destroy;
    workshop_door_reset(task);
    work->mode = DOOR_RUN;
}

static void workshop_door_reset(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->shut_ang = work->ang.y;
    VARS(work)->swing = 0.0F;
    VARS(work)->speed = 0.0F;
}

static void workshop_door_update(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 player;
    Float x;
    Float y;
    Float z;

    player = player_work[player_nearest(&work->pos)]->pos;
    x = player.x - work->pos.x;
    y = player.y - work->pos.y;
    z = player.z - work->pos.z;
    if ((x * x) + (y * y) + (z * z) < OPEN_REACH) {
        OPENING(work) = TRUE;
    } else {
        OPENING(work) = FALSE;
    }
    if (OPENING(work)) {
        if (VARS(work)->swing < OPEN_SWING * OPENING_BRAKE) {
            VARS(work)->speed += SPEED_UP;
            if (VARS(work)->speed > TOP_SPEED) {
                VARS(work)->speed = TOP_SPEED;
            }
        } else {
            VARS(work)->speed -= SLOW_DOWN;
            if (VARS(work)->speed < LOW_SPEED) {
                VARS(work)->speed = LOW_SPEED;
            }
        }
    } else {
        if (VARS(work)->swing > OPEN_SWING * SHUTTING_BRAKE) {
            VARS(work)->speed -= SPEED_UP;
            if (VARS(work)->speed < -TOP_SPEED) {
                VARS(work)->speed = -TOP_SPEED;
            }
        } else {
            VARS(work)->speed += SLOW_DOWN;
            if (VARS(work)->speed > -LOW_SPEED) {
                VARS(work)->speed = -LOW_SPEED;
            }
        }
    }
    VARS(work)->swing += VARS(work)->speed;
    if (VARS(work)->swing < 0.0F) {
        VARS(work)->swing = 0.0F;
    } else if (VARS(work)->swing > OPEN_SWING) {
        VARS(work)->swing = OPEN_SWING;
    }
    work->ang.y = VARS(work)->shut_ang + VARS(work)->swing;
}

static void workshop_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&workshop_door_model);
    njPopMatrix(1);
}

static void workshop_door_destroy(struct task *task)
{
}
