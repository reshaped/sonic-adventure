/* O WASI: an eagle that flies between random points around where it was
 * placed */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/helpers.h"
#include "object/helpers.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg02/eagle.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION windy_eagle_action;

/* work's vars: the point the eagle steers toward, which follows the
 * target */
struct eagle_vars {
    Float steer_y;
    Float steer_z;
    Float steer_x;
};

/* TASK_MOTION's record, laid out the eagle's own way */
struct eagle_motion {
    Uint8 unk_00[4];
    /* never written, so 0. windy_eagle_fly takes it, and never reads it */
    Float unk_04;
    Uint8 unk_08[4];
    /* how fast the eagle flaps and flies. DOUBLE_FRAME gains the pace each
     * step, and the eagle's speed follows it */
    Float pace;
    NJS_POINT3 home; /* where the eagle was placed */
    Uint8 unk_1c[24];
    NJS_POINT3 target; /* the point the current leg heads for */
};

/* the eagle's record at task->extra */
struct eagle_extra {
    NJS_POINT3 from; /* where the current leg toward the target starts */
};

#define VARS(work) ((struct eagle_vars *)&(work)->vars)
#define EAGLE_MOTION(task) ((struct eagle_motion *)(task)->motion)
#define EXTRA(task) ((struct eagle_extra *)(task)->extra)
/* twice the frame of the action drawn. it goes back to 0 at the action's last
 * frame. the eagle plays only the action's first half */
#define DOUBLE_FRAME(work) ((work)->scl.x)

/* the eagle's modes, 0, 3 and 5 as the image stores them. in EAGLE_END its
 * step does nothing, and no step here sets it */
enum eagle_mode { EAGLE_START = 0, EAGLE_FLY = 3, EAGLE_END = 5 };

/* each target lies this far plus scl.y from home in x and z. it lies up to
 * this far above home */
#define TARGET_REACH 80.0F

/* each new target sets the pace to PACE_START, and each step takes
 * PACE_DRAG off it, down to PACE_MIN */
#define PACE_START 2.0F
#define PACE_DRAG 0.01F
#define PACE_MIN 0.5F
/* the eagle's speed is its pace held between SPEED_MIN and SPEED_MAX,
 * times PLAYER_BOOST within PLAYER_NEAR of player 0. each step it flies
 * twice its speed toward the steer point */
#define SPEED_MAX 0.3F
#define SPEED_MIN 0.09F
#define PLAYER_NEAR 30.0F
#define PLAYER_BOOST 2.2F
/* work->tick counts the steps at PACE_MIN up to this, then starts again
 * from 0. no code here reads it */
#define SLOW_STEPS 10

/* the steer point moves this far toward the target each step */
#define STEER_SPEED 0.7F

static void windy_eagle_draw(struct task *task);
static void windy_eagle_fly(struct task *task, Float unused);
static void windy_eagle_choose(struct task *task);
static void windy_eagle_steer(struct task *task);

/* in the set editor, the eagle only draws */
void windy_eagle_step(struct task *task)
{
    struct task_work *work = task->work;
    struct eagle_motion *motion = EAGLE_MOTION(task);
    struct eagle_extra *extra = EXTRA(task);

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case EAGLE_START:
            work->mode = EAGLE_FLY;
            motion->home = work->pos;
            extra->from.x = work->pos.x;
            extra->from.y = work->pos.y;
            extra->from.z = work->pos.z;
            windy_eagle_choose(task);
            /* @bug logic: the first steer point is half the vector from
             * home to the first target (FSUB and FMUL at 0x0c90c3c2 and
             * 0x0c90c3c4 for x), where home plus that half belongs. it lies
             * near the world's origin, not between home and the target */
            VARS(work)->steer_x = (motion->target.x - extra->from.x) * 0.5F;
            VARS(work)->steer_y = (motion->target.y - extra->from.y) * 0.5F;
            VARS(work)->steer_z = (motion->target.z - extra->from.z) * 0.5F;
            break;
        case EAGLE_FLY:
            if (!placement_editing(task)) {
                windy_eagle_fly(task, motion->unk_04);
                windy_eagle_steer(task);
            }
            windy_eagle_draw(task);
            break;
        case EAGLE_END:
            break;
        }
    }
}

static void windy_eagle_draw(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: the motion read through the action's address, MOV.L
     * @(4,R3),R2 at 0x0c90c44c, so that the action's address is the one pool
     * word; without it, SHC pools the motion field's address for the read, and
     * 8 of 71 words differ */
    if (DOUBLE_FRAME(work) >=
        (Float)((&windy_eagle_action)->motion->nbFrame - 1)) {
        DOUBLE_FRAME(work) = 0.0F;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y - 0x4000);
    njScale(NULL, 1.5F, 1.5F, 1.5F);
    draw_action(&windy_eagle_action, DOUBLE_FRAME(work) * 0.5F);
    njPopMatrix(1);
}

/* advance the action by the pace, slow the pace, and fly toward the steer
 * point. vector_aim_x gives the angles that point x from the steer point to
 * the eagle. ang.z takes the z angle at once. ang.y turns toward the y angle by
 * at most a degree a step */
static void windy_eagle_fly(struct task *task, Float unused)
{
    struct task_work *work = task->work;
    struct eagle_motion *motion = EAGLE_MOTION(task);
    NJS_POINT3 steer;
    NJS_VECTOR away = {0.0F, 0.0F, 0.0F};
    Angle heading;
    Float speed;

    DOUBLE_FRAME(work) += motion->pace;
    /* @hack redundant: an add of 0 to the pace, FLDI0 at 0x0c90c54a and FADD
     * FR3,FR15 at 0x0c90c556; without it, 116 of 137 words differ */
    speed = motion->pace + 0.0F;
    if (speed > SPEED_MAX) {
        speed = SPEED_MAX;
    }
    if (SPEED_MIN > speed) {
        speed = SPEED_MIN;
    }
    motion->pace -= PACE_DRAG;
    if (PACE_MIN > motion->pace) {
        motion->pace = PACE_MIN;
        work->tick++;
        if (work->tick > SLOW_STEPS) {
            work->tick = 0;
        }
    }
    steer.x = VARS(work)->steer_x;
    steer.y = VARS(work)->steer_y;
    steer.z = VARS(work)->steer_z;
    if (njDistanceP2P(&work->pos, &player_work[0]->pos) < PLAYER_NEAR) {
        speed *= PLAYER_BOOST;
    }
    point_toward(&work->pos, &steer, speed * 2.0F, &work->pos);
    away.x = work->pos.x - steer.x;
    away.y = work->pos.y - steer.y;
    away.z = work->pos.z - steer.z;
    vector_aim_x(&away, &heading, &work->ang.z);
    work->ang.y = angle_approach(work->ang.y, heading, NJM_DEG_ANG(1));
}

static void windy_eagle_choose(struct task *task)
{
    struct task_work *work = task->work;
    struct eagle_motion *motion = EAGLE_MOTION(task);
    struct eagle_extra *extra = EXTRA(task);
    Angle ang;
    /* @hack redundant: leg, computed and never read, which keeps its FSUBs
     * and stores, from 0x0c90c6f6 to 0x0c90c71a; without it, 86 of 90 words
     * differ */
    NJS_VECTOR leg;
    /* @hack redundant: never read or written. it fills the 12 bytes below leg
     * in the 24-byte frame that ADD #-24,R15 at 0x0c90c68c makes; without it,
     * 35 of 90 words differ */
    NJS_VECTOR unused;

    /* a random angle in a turn, as RANDOM_ANGLE gives. the scale is not 2
     * but the float just under it, 0x3fffffff at 0x0c90c884. 0.000030517578
     * is just under 1/32768, and SHC folds 65536 times it to that float */
    ang = rand() * (65536.0F * 0.000030517578F);
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

/* move the steer point STEER_SPEED toward the target. a leg ends once the
 * eagle's distance from its start passes half the steer point's distance from
 * there. the next leg starts at the steer point, toward a new target */
static void windy_eagle_steer(struct task *task)
{
    struct task_work *work = task->work;
    struct eagle_motion *motion = EAGLE_MOTION(task);
    struct eagle_extra *extra = EXTRA(task);
    NJS_POINT3 target;
    NJS_POINT3 steer;
    /* @hack redundant: leg, computed and never read, which keeps its FSUBs
     * and stores, from 0x0c90c760 to 0x0c90c784; without it, 147 of 158 words
     * differ */
    NJS_VECTOR leg;
    NJS_POINT3 next;
    /* @hack redundant: up, copied from its template and never read, which
     * keeps the copy from 0x0c90c74c to 0x0c90c75e; without it, 147 of 158
     * words differ */
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
        windy_eagle_choose(task);
    }
}
