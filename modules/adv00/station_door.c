/* O STDOOR station's front door, locked until the story of player 0's
 * character opens it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/station_door.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the two leaves are its children */
extern NJS_OBJECT station_door_object;

/* the cylinder in which player 0 opens the door */
#define DOOR_REACH 15.0F
#define DOOR_HEIGHT 20.0F
/* the change of open at each step as the door closes. station_door_swing
 * scales it as the door opens */
#define DOOR_SPEED 0.02F
/* the open past which station_door_swing slows the opening door */
#define SWING_SLOWS 0.4F
#define DOOR_TURN NJM_DEG_ANG(120) /* a leaf's turn wide open */
/* the steps the door stays wide open before it can close */
#define DOOR_WAIT 30
/* each leaf hangs on a hinge DOOR_HINGE to its side of the door's center.
 * its shape's center lies LEAF_HALF from the hinge */
#define DOOR_HINGE 16.0F
#define LEAF_HALF 8.0F

#define SE_DOOR 530

/* a quarter turn, and the bit that is set in an angle from a half turn to a
 * whole one */
#define QUARTER_TURN 0x4000
#define HALF_TURN 0x8000

enum station_door_mode {
    DOOR_SHUT,
    DOOR_OPENING,
    DOOR_CLOSING,
    DOOR_OPEN,
    DOOR_HELD, /* open until player 0 leaves the cylinder */
    DOOR_STUCK, /* this file never sets it */
    DOOR_LOCKED
};

enum station_door_shape {
    SHAPE_FRONT, /* the whole front, on only while shut or locked */
    SHAPE_POST_RIGHT,
    SHAPE_POST_LEFT,
    SHAPE_LEAF_RIGHT,
    SHAPE_LEAF_LEFT,
    SHAPE_WALL_RIGHT,
    SHAPE_WALL_LEFT,
    SHAPE_WALL_ABOVE,
    SHAPES
};

/* work->timer counts down DOOR_OPEN's steps */
struct station_door_vars {
    Float open; /* from -1, open one way, through 0, shut, to 1 */
    Uint8 unk_04[4];
    Float speed; /* DOOR_SPEED, signed by the way the door opens */
};

#define VARS(work) ((struct station_door_vars *)&(work)->vars)

static void station_door_step(struct task *task);
static void station_door_draw(struct task *task);

/* clang-format off */
static struct collision_shape station_door_shapes[SHAPES] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 25.5F, 0.0F},
     55.0F, 25.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {55.0F, 25.5F, 0.0F},
     5.0F, 25.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-55.0F, 25.5F, 0.0F},
     5.0F, 25.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {8.0F, 15.0F, 0.0F},
     8.0F, 15.0F, 0.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-8.0F, 15.0F, 0.0F},
     8.0F, 15.0F, 0.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {35.5F, 15.5F, 0.0F},
     19.0F, 15.0F, 0.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-35.5F, 15.5F, 0.0F},
     19.0F, 15.0F, 0.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 40.5F, 0.0F},
     55.0F, 10.5F, 0.5F, {0}, 0, NULL},
};
/* clang-format on */

/* nonzero once the story of player 0's character has opened the station's
 * front */
static int station_door_unlocked(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_STATION_FRONT];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_STATION_FRONT];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_STATION_FRONT];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_SS_STATION_FRONT];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_SS_STATION_FRONT];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_SS_STATION_FRONT];
    }
    return 0;
}

/* the box behind the front that station_door_behind tests: BEHIND_HALF_WIDTH to
 * each side of the door's center, and BEHIND_DEPTH deep */
#define BEHIND_HALF_WIDTH 60.0F
#define BEHIND_DEPTH 66.0F

/* 1 while player 0 stands in the box behind the front, else 0 */
static int station_door_behind(struct task_work *work)
{
    Float dx = player_work[0]->pos.x - work->pos.x;
    Float dz = player_work[0]->pos.z - work->pos.z;
    Float cos = njCos(-work->ang.y);
    Float sin = njSin(-work->ang.y);
    Float x = (dx * cos) + (dz * sin);
    Float z = (dz * cos) - (dx * sin);

    if (x > -BEHIND_HALF_WIDTH && x < BEHIND_HALF_WIDTH && z < 0.0F &&
        z > -BEHIND_DEPTH) {
        return 1;
    }
    return 0;
}

/* make the whole front solid while the door is shut or locked, and else its
 * posts, walls and leaves, each leaf turned by open. then register for this
 * step's collision checks */
static void station_door_collide(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;
    Angle ang;
    Float x;
    Float z;

    if (work->mode == DOOR_LOCKED || work->mode == DOOR_SHUT) {
        shapes[SHAPE_FRONT].flags &= ~SHAPE_OFF;
        shapes[SHAPE_POST_RIGHT].flags &= ~SHAPE_OFF;
        shapes[SHAPE_POST_LEFT].flags &= ~SHAPE_OFF;
        shapes[SHAPE_LEAF_RIGHT].flags |= SHAPE_OFF;
        shapes[SHAPE_LEAF_LEFT].flags |= SHAPE_OFF;
        shapes[SHAPE_WALL_RIGHT].flags |= SHAPE_OFF;
        shapes[SHAPE_WALL_LEFT].flags |= SHAPE_OFF;
        shapes[SHAPE_WALL_ABOVE].flags |= SHAPE_OFF;
    } else {
        shapes[SHAPE_FRONT].flags |= SHAPE_OFF;
        shapes[SHAPE_POST_RIGHT].flags &= ~SHAPE_OFF;
        shapes[SHAPE_POST_LEFT].flags &= ~SHAPE_OFF;
        shapes[SHAPE_LEAF_RIGHT].flags &= ~SHAPE_OFF;
        shapes[SHAPE_LEAF_LEFT].flags &= ~SHAPE_OFF;
        shapes[SHAPE_WALL_RIGHT].flags &= ~SHAPE_OFF;
        shapes[SHAPE_WALL_LEFT].flags &= ~SHAPE_OFF;
        shapes[SHAPE_WALL_ABOVE].flags &= ~SHAPE_OFF;
        ang = (Angle)(VARS(work)->open * DOOR_TURN);
        x = -DOOR_HINGE + (LEAF_HALF * njCos(ang));
        z = LEAF_HALF * njSin(ang);
        shapes[SHAPE_LEAF_RIGHT].center.x = -x;
        shapes[SHAPE_LEAF_LEFT].center.x = x;
        shapes[SHAPE_LEAF_RIGHT].center.z = shapes[SHAPE_LEAF_LEFT].center.z =
            z;
        shapes[SHAPE_LEAF_RIGHT].ang.y = ang;
        shapes[SHAPE_LEAF_LEFT].ang.y = -ang;
    }
    collision_register(work);
}

/* swing open: open moves by twice speed until the door is SWING_SLOWS open, and
 * by less the further it opens after that */
static void station_door_swing(struct task_work *work)
{
    Float open = _builtin_fabsf(VARS(work)->open);

    /* @hack spelling: the two sums written out, which load 2.0F and 0.8F
     * with MOVA (0x0c906cf2, 0x0c906d02). as +=, SHC loads them through R1
     * and FPUL, the function runs 6 bytes long and 24 of 29 words differ */
    if (open < SWING_SLOWS) {
        VARS(work)->open = VARS(work)->open + (VARS(work)->speed * 2.0F);
    } else {
        VARS(work)->open = VARS(work)->open + (VARS(work)->speed * 0.8F / open);
    }
}

void station_door_init(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    work->ang.x = 0;
    work->ang.z = 0;
    work->submode = 0;
    if (!placement_editing(task)) {
        collision_init(task, station_door_shapes, COLLISION_OBJECT, 1, SHAPES);
    }
    if (station_door_unlocked()) {
        work->mode = DOOR_SHUT;
    } else {
        work->mode = DOOR_LOCKED;
    }
    task->step = station_door_step;
    task->draw = station_door_draw;
}

static void station_door_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    Angle ang;
    int near;
    /* @hack spelling: DOOR_SHUT held in shut, set before the switch, which
     * keeps 0 in R12 for DOOR_CLOSING's and DOOR_LOCKED's stores (MOV #0,R12
     * at 0x0c906da6). with DOOR_SHUT in each store, 90 of 227 words differ */
    Sint8 shut;

    if (placement_try_leave(task)) {
        return;
    }
    shut = DOOR_SHUT;
    switch (work->mode) {
    case DOOR_SHUT:
        near = player_in_cylinder(&work->pos, DOOR_REACH, DOOR_HEIGHT);
        /* @hack redundant: a store of DOOR_REACH that never runs, which puts
         * player_in_cylinder's address ahead of DOOR_REACH in the pool
         * (0x0c906e08, 0x0c906e0c). without it, the two trade and 2 of 227
         * words differ */
        if (0) {
            VARS(work)->speed = DOOR_REACH;
        }
        if (!near) {
            break;
        }
        player_get_history(0, 0, &pos, NULL);
        ang = (Angle)(atan2f(pos.x - work->pos.x, pos.z - work->pos.z) *
                      ANGLE_PER_RADIAN) -
              work->ang.y + QUARTER_TURN;
        if (ang & HALF_TURN) {
            VARS(work)->speed = DOOR_SPEED;
        } else {
            VARS(work)->speed = -DOOR_SPEED;
        }
        work->mode = DOOR_OPENING;
        sound_play(SE_DOOR, 0, 0, 0);
        break;
    case DOOR_OPENING:
        station_door_swing(work);
        if (VARS(work)->open > 1.0F) {
            VARS(work)->open = 1.0F;
            work->mode = DOOR_OPEN;
            TASK_TIMER(work) = DOOR_WAIT;
        } else if (VARS(work)->open < -1.0F) {
            VARS(work)->open = -1.0F;
            work->mode = DOOR_OPEN;
            TASK_TIMER(work) = DOOR_WAIT;
        }
        /* @hack redundant: a store of DOOR_OPEN that never runs, which has
         * R4 hold DOOR_OPEN and R5 DOOR_WAIT (MOV #3,R4 at 0x0c906ea0, MOV
         * #30,R5 at 0x0c906e9a). without it, the two trade and 6 of 227 words
         * differ */
        if (0) {
            work->mode = DOOR_OPEN;
        }
        break;
    case DOOR_OPEN:
        if (--TASK_TIMER(work) == 0) {
            work->mode = DOOR_HELD;
        }
        break;
    case DOOR_HELD:
        near = player_in_cylinder(&work->pos, DOOR_REACH, DOOR_HEIGHT);
        /* @hack spelling: the empty branch for player 0 near, which keeps
         * DOOR_SHUT's call of sound_play and this case's apart (JSR at
         * 0x0c906e88 and 0x0c906f24). as if (!near), the two share this
         * case's, and 48 of 227 words differ */
        if (near) {
        } else {
            work->mode = DOOR_CLOSING;
            sound_play(SE_DOOR, 0, 0, 0);
            /* @hack redundant: the else's own break, which has DOOR_CLOSING
             * store DOOR_OPENING from R3 (MOV #1,R3 at 0x0c906f3c). without it,
             * R2 takes it and 2 of 227 words differ */
            break;
        }
        break;
    case DOOR_CLOSING:
        near = player_in_cylinder(&work->pos, DOOR_REACH, DOOR_HEIGHT);
        if (near) {
            work->mode = DOOR_OPENING;
            break;
        }
        /* @hack spelling: the subtraction written out, which reads speed at
         * an offset in R0 (FMOV.S @(R0,R14),FR3 at 0x0c906f82). as -=, SHC
         * reads it through @R1 after ADD R14,R1, the function runs 2 bytes
         * long and 70 of 227 words differ */
        VARS(work)->open = VARS(work)->open - VARS(work)->speed;
        if ((VARS(work)->speed < 0.0F && VARS(work)->open > 0.0F) ||
            (VARS(work)->speed > 0.0F && VARS(work)->open < 0.0F)) {
            VARS(work)->open = 0.0F;
            /* the 4 stored here is never read: DOOR_OPENING sets the timer
             * again before DOOR_OPEN counts it down */
            if (station_door_behind(work)) {
                TASK_TIMER(work) = 4;
            }
            work->mode = shut;
        }
        break;
    case DOOR_LOCKED:
        if (!station_door_unlocked()) {
            break;
        }
        work->mode = shut;
        break;
    case DOOR_STUCK:
        break;
    }
    if (!placement_editing(task)) {
        station_door_collide(work);
    }
    station_door_draw(task);
}

static void station_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle turn = -(Angle)(VARS(work)->open * DOOR_TURN);
    Angle ang;
    NJS_OBJECT *leaf;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(station_door_object.model);
    leaf = station_door_object.child;
    njPushMatrix(NULL);
    njTranslate(NULL, leaf->pos[0], leaf->pos[1], leaf->pos[2]);
    TURN(njRotateY, turn);
    draw_model(leaf->model);
    njPopMatrix(1);
    leaf = leaf->sibling;
    njTranslate(NULL, leaf->pos[0], leaf->pos[1], leaf->pos[2]);
    TURN_VIA(njRotateY, -turn, ang);
    draw_model(leaf->model);
    njPopMatrix(1);
}
