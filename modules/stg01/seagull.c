/* O KAMOME and O KAMOMEL seagulls that fly between random points above a
 * circle about where they were placed */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/helpers.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg01/seagull.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION beach_seagull_action;
extern NJS_MODEL beach_seagull_l_body_model;
extern NJS_OBJECT beach_seagull_l_wing_minus_x_object;
extern NJS_OBJECT beach_seagull_l_wing_plus_x_object;

/* the steer point. the seagull flies toward it, and it moves on toward the
 * target */
struct seagull_vars {
    Float steer_y;
    Float steer_z;
    Float steer_x;
};

struct seagull_motion {
    Uint8 unk_00[4];
    Float unk_04;
    Uint8 unk_08[4];
    Float pace; /* the action's frames a step, and the speed before its clamp */
    NJS_POINT3 home; /* where the seagull was placed */
    Uint8 unk_1c[24];
    NJS_POINT3 target; /* the point the steer point moves toward */
};

/* where the current leg toward the target starts */
struct seagull_extra {
    NJS_POINT3 from;
};

#define VARS(work) ((struct seagull_vars *)&(work)->vars)
#define SEAGULL_MOTION(task) ((struct seagull_motion *)(task)->motion)
#define EXTRA(task) ((struct seagull_extra *)(task)->extra)

/* in SEAGULL_IDLE, the step neither flies nor draws. nothing here enters
 * it */
enum seagull_mode { SEAGULL_START = 0, SEAGULL_FLY = 3, SEAGULL_IDLE = 5 };

/* the targets lie this far plus scl.y out from home, and up to this far
 * above it */
#define TARGET_REACH 80.0F

/* each new target sets the pace to PACE_START. the pace then falls by
 * PACE_DRAG a step, down to PACE_LEAST */
#define PACE_START 2.0F
#define PACE_DRAG 0.01F
#define PACE_LEAST 0.5F
/* the seagull always flies SPEED_MOST a step, or PLAYER_BOOST times that
 * within PLAYER_NEAR of player 0. its speed is its pace held from
 * SPEED_LEAST to SPEED_MOST. the pace never falls below PACE_LEAST, which is
 * above SPEED_MOST */
#define SPEED_MOST 0.3F
#define SPEED_LEAST 0.09F
#define PLAYER_NEAR 30.0F
#define PLAYER_BOOST 2.2F

/* the steer point moves this far toward the target each step */
#define STEER_SPEED 0.7F

/* O KAMOMEL's wings swing this many degrees times the pace each way */
#define WING_SWING 10.0F

/* both seagulls draw at this scale */
#define SEAGULL_SCALE 1.5F

static void seagull_draw(struct task *task);
static void seagull_l_draw(struct task *task);
static void seagull_fly(struct task *task, Float unused);
static void seagull_choose(struct task *task);
static void seagull_steer(struct task *task);

/* the object list's step for O KAMOME */
void beach_seagull_step(struct task *task)
{
    struct task_work *work = task->work;
    struct seagull_motion *motion = SEAGULL_MOTION(task);
    struct seagull_extra *extra = EXTRA(task);

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SEAGULL_START:
        work->mode = SEAGULL_FLY;
        motion->home = work->pos;
        extra->from.x = work->pos.x;
        extra->from.y = work->pos.y;
        extra->from.z = work->pos.z;
        seagull_choose(task);
        /* @bug logic: the first steer point is half the vector from home
         * to the first target, with home never added (FSUB, then FMUL by 0.5,
         * at 0x0c904482 and 0x0c904484). it lies near the world's origin, not
         * halfway to the target */
        VARS(work)->steer_x = (motion->target.x - extra->from.x) * 0.5F;
        VARS(work)->steer_y = (motion->target.y - extra->from.y) * 0.5F;
        VARS(work)->steer_z = (motion->target.z - extra->from.z) * 0.5F;
        break;
    case SEAGULL_FLY:
        if (!placement_editing(task)) {
            seagull_fly(task, motion->unk_04);
            seagull_steer(task);
        }
        seagull_draw(task);
        break;
    case SEAGULL_IDLE:
        break;
    }
}

/* draw the action at frame scl.x, which the step advances. a frame at the
 * motion's last or past it goes back to 0 */
static void seagull_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y - NJM_DEG_ANG(90));
    njScale(NULL, SEAGULL_SCALE, SEAGULL_SCALE, SEAGULL_SCALE);
    draw_action(&beach_seagull_action, work->scl.x);
    njPopMatrix(1);
    /* @hack spelling: the motion reached through the action's address, so the
     * load at 0x0c90455a takes the action's own pool entry, which
     * draw_action's argument uses too. through the member, 5 of 68 words and
     * 4 pool words differ */
    if (work->scl.x >= (Float)((&beach_seagull_action)->motion->nbFrame - 1)) {
        work->scl.x = 0.0F;
    }
}

/* TASK_TIMER(work) counts these draws, and the wings beat once every 360 of
 * them */
static void seagull_l_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct seagull_motion *motion = SEAGULL_MOTION(task);
    Float swing;
    Angle ang;

    TASK_TIMER(work)++;
    swing = motion->pace * njSin((Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE)) *
            WING_SWING;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y - NJM_DEG_ANG(90));
    njScale(NULL, SEAGULL_SCALE, SEAGULL_SCALE, SEAGULL_SCALE);
    draw_model(&beach_seagull_l_body_model);
    njPushMatrix(NULL);
    TURN_VIA(njRotateZ, (Angle)(swing * ANGLE_PER_DEGREE), ang);
    /* the plus-x wing is the minus-x wing's sibling, so it draws twice:
     * here, turned with the minus-x wing, and below, turned the other way */
    draw_tree(&beach_seagull_l_wing_minus_x_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    TURN(njRotateZ, -ang);
    draw_tree(&beach_seagull_l_wing_plus_x_object);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* advance the action by the pace, slow the pace, and fly toward the steer
 * point. the seagull turns its x axis away from the point. ang.z turns at
 * once, and ang.y by at most a degree a step. work->tick counts the steps at
 * PACE_LEAST from 0 to 10 and round again. nothing reads it */
static void seagull_fly(struct task *task, Float unused)
{
    struct task_work *work = task->work;
    struct seagull_motion *motion = SEAGULL_MOTION(task);
    NJS_POINT3 steer;
    /* @hack redundant: an initializer, which the image copies from
     * 0x0c911428 (0x0c904748 to 0x0c90475e) before it sets each field again.
     * without it, 129 of 134 words differ, the functions after it move, and
     * the constants are 12 bytes fewer */
    NJS_VECTOR away = {0.0F, 0.0F, 0.0F};
    Angle heading;
    Float speed;

    work->scl.x += motion->pace;
    /* @hack redundant: + 0.0F, which keeps the image's FADD at 0x0c904776.
     * without it, 113 of 134 words differ */
    speed = motion->pace + 0.0F;
    if (speed > SPEED_MOST) {
        speed = SPEED_MOST;
    }
    if (speed < SPEED_LEAST) {
        speed = SPEED_LEAST;
    }
    motion->pace -= PACE_DRAG;
    if (motion->pace < PACE_LEAST) {
        motion->pace = PACE_LEAST;
        work->tick++;
        if (work->tick > 10) {
            work->tick = 0;
        }
    }
    steer.x = VARS(work)->steer_x;
    steer.y = VARS(work)->steer_y;
    steer.z = VARS(work)->steer_z;
    if (njDistanceP2P(&work->pos, &player_work[0]->pos) < PLAYER_NEAR) {
        speed *= PLAYER_BOOST;
    }
    point_toward(&work->pos, &steer, speed, &work->pos);
    away.x = work->pos.x - steer.x;
    away.y = work->pos.y - steer.y;
    away.z = work->pos.z - steer.z;
    vector_aim_x(&away, &heading, &work->ang.z);
    work->ang.y = angle_approach(work->ang.y, heading, NJM_DEG_ANG(1));
}

/* pick a target at random, and set the pace to PACE_START */
static void seagull_choose(struct task *task)
{
    struct task_work *work = task->work;
    struct seagull_motion *motion = SEAGULL_MOTION(task);
    struct seagull_extra *extra = EXTRA(task);
    /* @hack redundant: the leg, which the image stores (0x0c904920 to
     * 0x0c90493a) and never reads. without it, 86 of 90 words differ and the
     * functions after it move */
    NJS_VECTOR leg;
    /* @hack redundant: twelve bytes declared and never used, which keep the
     * frame at 24 bytes (ADD #-24,R15 at 0x0c9048ac). without them, 35 of 90
     * words differ and the functions after it move */
    Uint8 unused[12];
    /* rand() times just under 2: an angle from 0 up to a turn. 1.9999999F
     * spells the image's factor, the float 0x3fffffff */
    Angle ang = (Angle)(rand() * 1.9999999F);

    motion->target.x =
        motion->home.x + ((work->scl.y + TARGET_REACH) * njSin(ang));
    motion->target.y =
        motion->home.y + _builtin_fabsf(njSin(ang) * TARGET_REACH);
    motion->target.z =
        motion->home.z + ((work->scl.y + TARGET_REACH) * njCos(ang));
    leg.x = motion->target.x - extra->from.x;
    leg.y = motion->target.y - extra->from.y;
    leg.z = motion->target.z - extra->from.z;
    motion->pace = PACE_START;
}

/* move the steer point STEER_SPEED toward the target. the leg ends once the
 * seagull's distance from the leg's start passes half the steer point's.
 * the next leg starts at the steer point, toward a new target */
static void seagull_steer(struct task *task)
{
    struct task_work *work = task->work;
    struct seagull_motion *motion = SEAGULL_MOTION(task);
    struct seagull_extra *extra = EXTRA(task);
    NJS_POINT3 target;
    NJS_POINT3 steer;
    /* @hack redundant: the leg, which the image stores (0x0c904988 to
     * 0x0c9049a4) and never reads. without it, 147 of 158 words differ and
     * the functions after it move */
    NJS_VECTOR leg;
    NJS_POINT3 next;
    /* @hack redundant: up, whose initializer the image copies (0x0c904976
     * to 0x0c90497e) and never reads. without it, 147 of 158 words differ
     * and the constants are 12 bytes fewer */
    NJS_VECTOR up = {0.0F, 1.0F, 0.0F};
    NJS_VECTOR back;
    NJS_VECTOR flown;

    leg.x = motion->target.x - extra->from.x;
    leg.y = motion->target.y - extra->from.y;
    leg.z = motion->target.z - extra->from.z;
    steer.x = VARS(work)->steer_x;
    steer.y = VARS(work)->steer_y;
    steer.z = VARS(work)->steer_z;
    target.x = motion->target.x;
    target.y = motion->target.y;
    target.z = motion->target.z;
    point_toward(&steer, &target, STEER_SPEED, &next);
    VARS(work)->steer_x = next.x;
    VARS(work)->steer_y = next.y;
    VARS(work)->steer_z = next.z;
    back.x = extra->from.x - VARS(work)->steer_x;
    back.y = extra->from.y - VARS(work)->steer_y;
    back.z = extra->from.z - VARS(work)->steer_z;
    flown.x = work->pos.x - extra->from.x;
    flown.y = work->pos.y - extra->from.y;
    flown.z = work->pos.z - extra->from.z;
    if (njScalor(&back) * 0.5F < njScalor(&flown)) {
        extra->from.x = VARS(work)->steer_x;
        extra->from.y = VARS(work)->steer_y;
        extra->from.z = VARS(work)->steer_z;
        seagull_choose(task);
    }
}

/* the object list's step for O KAMOMEL. it runs as beach_seagull_step does,
 * but draws with seagull_l_draw */
void beach_seagull_l_step(struct task *task)
{
    struct task_work *work = task->work;
    struct seagull_motion *motion = SEAGULL_MOTION(task);
    struct seagull_extra *extra = EXTRA(task);

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SEAGULL_START:
        work->mode = SEAGULL_FLY;
        motion->home = work->pos;
        extra->from.x = work->pos.x;
        extra->from.y = work->pos.y;
        extra->from.z = work->pos.z;
        seagull_choose(task);
        /* @bug logic: as in beach_seagull_step, home is never added to
         * the first steer point (FSUB, then FMUL by 0.5, at 0x0c904b36 and
         * 0x0c904b38) */
        VARS(work)->steer_x = (motion->target.x - extra->from.x) * 0.5F;
        VARS(work)->steer_y = (motion->target.y - extra->from.y) * 0.5F;
        VARS(work)->steer_z = (motion->target.z - extra->from.z) * 0.5F;
        break;
    case SEAGULL_FLY:
        if (!placement_editing(task)) {
            seagull_fly(task, motion->unk_04);
            seagull_steer(task);
        }
        seagull_l_draw(task);
        break;
    case SEAGULL_IDLE:
        break;
    }
}
