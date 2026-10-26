/* O HAMMER hammer that swings on its mount and flings the players it hits */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg10/hammer.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT hammer_mount_object;
extern NJS_MODEL hammer_model;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* the collision record's radius: 100 squared, though the checks take it as
 * a plain distance */
#define REACH (100.0F * 100.0F)

/* how far the hammer swings each way from hanging straight down, as angle x:
 * about 81.6 degrees */
#define SWING 14848.0F
/* a fling's speed is scale x times FLING times the cosine of the swing's
 * phase. the cosine peaks at the bottom of the swing, where the head moves
 * fastest */
#define FLING 0.014466831F
/* the steps after a fling before the hammer can fling again */
#define HIT_WAIT 30

struct hammer_vars {
    Uint8 unk_00[4];
    Sint16 wait; /* steps left of HIT_WAIT */
};

#define VARS(work) ((struct hammer_vars *)&(work)->vars)
/* the swing's phase, which gains scale x each step. angle x is SWING times
 * its sine */
#define PHASE(work) TASK_TIMER(work)

enum hammer_mode { HAMMER_INIT, HAMMER_RUN };

static void hammer_init(struct task *task);
static void hammer_reset(struct task *task);
static void hammer_swing(struct task *task);
static void hammer_draw(struct task *task);
static void hammer_destroy(struct task *task);

/* the head, then the handle */
static struct collision_shape hammer_shapes[2] = {
    {SHAPE_CAPSULE | SHAPE_TURN_YZX | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | ATTACK(2),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -69.0F, 0.0F},
     17.0F,
     16.5F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE | SHAPE_TURN_YZX,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -26.5F, 0.0F},
     4.8F,
     21.7F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the fling along z, before it is turned by the hammer's facing and swing */
static NJS_VECTOR hammer_fling = {0.0F, 0.0F, 0.0F};

void hammer_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case HAMMER_INIT:
            hammer_init(task);
            break;
        case HAMMER_RUN:
            hammer_draw(task);
            hammer_swing(task);
            break;
        }
    } else {
        switch (work->mode) {
        case HAMMER_INIT:
            hammer_init(task);
            break;
        case HAMMER_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ROTATION SPEED");
            hammer_draw(task);
            hammer_swing(task);
            break;
        }
    }
}

static void hammer_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = hammer_draw;
    task->destructor = hammer_destroy;
    hammer_reset(task);
    work->mode = HAMMER_RUN;
}

static void hammer_reset(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, hammer_shapes, COLLISION_OBJECT, 1, 2);
    work->collision->radius = REACH;
    PHASE(work) = 0;
    work->ang.x = 0;
    VARS(work)->wait = 0;
}

static void hammer_swing(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct task_work *player;
    NJS_VECTOR speed;
    Angle ang;
    int i;

    if ((collision->flags & COLLISION_HIT) &&
        collision->hit->kind == COLLISION_PLAYER && VARS(work)->wait == 0) {
        VARS(work)->wait = HIT_WAIT;
        hammer_fling.z = -(work->scl.x * njCos(PHASE(work)) * FLING);
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        njCalcVector(NULL, &hammer_fling, &speed);
        player = collision->hit->owner->work;
        for (i = 0; i < PLAYER_COUNT; i++) {
            if (player_work[i] == player) {
                player_set_speed(i, speed.x, speed.y, speed.z);
            }
        }
        njPopMatrix(1);
    } else if (VARS(work)->wait) {
        VARS(work)->wait--;
    }
    PHASE(work) += work->scl.x;
    work->ang.x = njSin(PHASE(work)) * SWING;
    collision_register(work);
}

static void hammer_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&hammer_mount_object);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_model(&hammer_model);
        njPopMatrix(1);
    }
}

static void hammer_destroy(struct task *task)
{
}
