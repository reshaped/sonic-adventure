/* O IWA, a rock that bobs under player 0 and tips toward them */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg01/bobbing_rock.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL beach_bobbing_rock_model;
/* the object of beach_bobbing_rock_model */
/* @hack type: declared an array of one, not the one NJS_OBJECT it is, so SHC
 * pools its address and loads model 4 bytes past it, MOV.L @(4,R3),R2 at
 * 0x0c9023ce; as an object, 10 of beach_bobbing_rock_add_world's 49 words
 * and 1 pool word differ */
extern NJS_OBJECT beach_bobbing_rock_object[1];

struct bobbing_rock_vars {
    NJS_OBJECT *world; /* the rock's object in the collision world */
    Uint8 unk_04[4];
    Float unk_08;
};

/* the bob, in the task's extra. bob_slot names its floats. BOB_PHASE is in
 * degrees. BOB_DAMPING is what each step takes from BOB_DEPTH. BOB_HOME_Y is
 * the height the rock was placed at */
/* @hack type: the bob as an array that bob_slot indexes, which reaches the
 * floats through copies of extra in R4 and R12 (0x0c902598, 0x0c9025b0); as
 * members, it reaches them through R13, and 43 of beach_bobbing_rock_run's
 * 277 words differ */
struct bobbing_rock_extra {
    Float bob[4];
};

enum bob_slot { BOB_PHASE, BOB_DEPTH, BOB_DAMPING, BOB_HOME_Y };

/* the rock's cylinder reaches INSIDE_RADIUS from its pos across, and
 * INSIDE_RADIUS sin 50 degrees above and below it */
#define INSIDE_RADIUS 23.0F
/* the bob's phase gains PHASE_STEP degrees each step that player 0 is on
 * the ground in the cylinder. while player 0 is outside, the rock rises
 * RISE_SPEED a step, up to its home height. the bob meanwhile holds
 * START_DEPTH and START_DAMPING */
#define PHASE_STEP 1.7F
#define RISE_SPEED 0.2F
#define START_DEPTH 15.0F
#define START_DAMPING 0.08F
/* the rock turns toward its tilt at most TILT_STEP a step, 65536 a turn */
#define TILT_STEP 15
/* the world tests the rock only while a player is within its model's radius
 * plus WORLD_NEAR of it */
#define WORLD_NEAR 20.0F
/* the radius of the sphere round the rock that must be in view to draw it */
#define ROCK_VIEW 20.0F
/* while player 0 stays inside, work->timer gains TIMER_GAIN / (tick + 1) a
 * step until that falls below 1, and unk_08 holds UNK_08_START until then */
#define TIMER_GAIN 200
#define UNK_08_START (-0.035F)

#define VARS(work) ((struct bobbing_rock_vars *)&(work)->vars)

enum bobbing_rock_mode {
    BOBBING_ROCK_START = 0,
    BOBBING_ROCK_RUN = 1,
    BOBBING_ROCK_IDLE = 2
};

static void beach_bobbing_rock_destroy(struct task *task);
static void beach_bobbing_rock_add_world(struct task *task);
static void beach_bobbing_rock_run(struct task *task);
static Sint8 beach_bobbing_rock_player_inside(struct task_work *work);
static void beach_bobbing_rock_draw(struct task *task);

void beach_bobbing_rock_step(struct task *task)
{
    struct task_work *work = task->work;
    struct bobbing_rock_extra *extra = task->extra;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BOBBING_ROCK_START:
        task->destructor = beach_bobbing_rock_destroy;
        task->draw = beach_bobbing_rock_draw;
        extra->bob[BOB_HOME_Y] = work->pos.y;
        beach_bobbing_rock_add_world(task);
        work->mode = BOBBING_ROCK_RUN;
        break;
    case BOBBING_ROCK_RUN:
        if (!placement_editing(task)) {
            beach_bobbing_rock_run(task);
        }
        break;
    case BOBBING_ROCK_IDLE:
        break;
    }
    beach_bobbing_rock_draw(task);
}

static void beach_bobbing_rock_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->world);
    world_object_free(VARS(work)->world);
}

/* put the rock in the collision world, as a medium object whose mesh is
 * made again each step. the run poses the object each step after */
static void beach_bobbing_rock_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    object = world_object_alloc();
    VARS(work)->world = object;
    object->evalflags = NJD_EVAL_BREAK;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = work->ang.x;
    object->ang[1] = 0;
    object->ang[2] = work->ang.z;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = beach_bobbing_rock_object[0].model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(WORLD_MEDIUM | WORLD_MOVES, task, object);
    /* @hack redundant: the return, which gives task a frame word, ADD #-4 and
     * MOV.L R4,@R15 at 0x0c902386 and 0x0c90238c; without it, SHC pushes task,
     * and 47 of 49 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* while player 0 is inside the rock's cylinder, the rock tips toward them.
 * in x and in z, it tips a sixth of their angle from straight up. while they
 * are also on the ground, it sinks |depth sin phase| below its home height.
 * force->speed.y takes each such step's change in height. the depth loses
 * the damping each such step, down to 0. while player 0 is outside, the rock
 * levels and rises back to its home height. the extra starts zeroed, so the
 * rock sinks only once player 0 has been outside */
static void beach_bobbing_rock_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    struct bobbing_rock_extra *extra = task->extra;
    Angle tilt_z = 0;
    Angle tilt_x = 0;
    struct task *player;
    struct task_work *player_work;
    Float dx;
    Float dz;
    Float y;
    Float tilt_scale;
    NJS_OBJECT *object;
    int step;

    if (!(player = player_get_task(0))) {
        return;
    }
    player_work = player->work;
    if (beach_bobbing_rock_player_inside(work) == 1) {
        /* nothing reads unk_08 or work->timer, which these lines set */
        if (work->tick == 0) {
            VARS(work)->unk_08 = UNK_08_START;
        }
        step = TIMER_GAIN / (work->tick + 1);
        if (step < 1) {
            VARS(work)->unk_08 = 0.0F;
        } else {
            work->tick++;
        }
        work->timer += step;
        dx = player_work->pos.x - work->pos.x;
        tilt_scale = (1.0F / 12.0F);
        /* @hack redundant: never-run stores, which put 0 in FR14, dx in FR12,
         * the scale in FR13 and the radian factor in FR15, at 0x0c902478 to
         * 0x0c9024d2; without it, 25 of 277 words differ */
        if (0) {
            VARS(work)->unk_08 = ANGLE_PER_RADIAN;
            VARS(work)->unk_08 = ANGLE_PER_RADIAN;
            VARS(work)->unk_08 = 0.0F;
            VARS(work)->unk_08 = tilt_scale;
            VARS(work)->unk_08 = tilt_scale;
            VARS(work)->unk_08 = ANGLE_PER_RADIAN;
            VARS(work)->unk_08 = 0.0F;
        }
        if (dx > 0.0F) {
            tilt_z = -(Angle)(atan2f(dx, player_work->pos.y - work->pos.y) *
                              ANGLE_PER_RADIAN);
            /* @hack redundant: a never-run call, which keeps the angle in R11,
             * a register calls preserve: FMOV.S FR0,FR3 at 0x0c9024e0 and STS
             * FPUL,R11 at 0x0c9024e6; without it, 95 of 277 words differ */
            if (0) {
                texture_set_stage();
            }
        } else {
            tilt_z = (Angle)(atan2f(_builtin_fabsf(dx),
                                    player_work->pos.y - work->pos.y) *
                             ANGLE_PER_RADIAN);
        }
        /* @hack redundant: a constant select, which keeps tilt_z's converted
         * copy, FLOAT FPUL,FR3 and FMOV FR3,FR2 at 0x0c90253c; without it, 95
         * of 277 words differ */
        tilt_z = (Angle)((1 ? (Float)tilt_z : 0.0F) * tilt_scale);
        /* @hack redundant: a never-run test of dx, which copies it to FR4 ahead
         * of the FABS, at 0x0c902522 and 0x0c90252c; without it, 2 of 277 words
         * differ */
        if (0) {
            if (dx) {
            }
        }
        /* @hack redundant: a never-run store of tilt_x, which keeps atan2f's
         * address in R9 and tilt_x in R10, from 0x0c9024b8; without it, 56 of
         * 277 words differ */
        if (0) {
            work->ang.x = tilt_x;
        }
        /* @hack redundant: a never-run test of tilt_z, one of two. either one
         * alone keeps dx in FR12 and the scale in FR13 (0x0c9024c0,
         * 0x0c9024c6), which otherwise trade. with the second, this one keeps
         * 0 in R9, tilt_x in R10 and tilt_z in R11 from 0x0c90245a; without
         * it, 14 of 277 words differ */
        if (0) {
            if (tilt_z) {
            }
        }
        /* @hack redundant: the second never-run test of tilt_z. with the
         * first, it keeps 0 in R9, tilt_x in R10 and tilt_z in R11 from
         * 0x0c90245a; without it, 14 of 277 words differ */
        if (0) {
            if (tilt_z) {
            }
        }
        dz = player_work->pos.z - work->pos.z;
        if (dz > 0.0F) {
            tilt_x = (Angle)(atan2f(dz, player_work->pos.y - work->pos.y) *
                             ANGLE_PER_RADIAN);
            /* @hack redundant: a never-run call, which, as for tilt_z, keeps
             * the angle in R10, a register calls preserve: FMOV.S FR0,FR3 at
             * 0x0c90255c and STS FPUL,R10 at 0x0c902564; without it, 70 of 277
             * words differ */
            if (0) {
                texture_set_stage();
            }
        } else {
            tilt_x = -(Angle)(atan2f(_builtin_fabsf(dz),
                                     player_work->pos.y - work->pos.y) *
                              ANGLE_PER_RADIAN);
        }
        /* @hack redundant: a never-run fabs of tilt_x, which has its FLOAT
         * write FR3, copied to FR2, at 0x0c902582 and 0x0c902586; without it, 4
         * of 277 words differ */
        if (0) {
            (void)_builtin_fabsf((Float)tilt_x);
        }
        /* @hack redundant: a constant select, which keeps tilt_x's converted
         * copy, FLOAT FPUL,FR3 and FMOV FR3,FR2 at 0x0c902582 and 0x0c902586;
         * without it, 146 of 277 words differ */
        tilt_x = (Angle)((1 ? (Float)tilt_x : 0.0F) * tilt_scale);
        if (player_work->flags & PLAYER_ON_GROUND) {
            extra->bob[BOB_PHASE] += PHASE_STEP;
            extra->bob[BOB_DEPTH] -= extra->bob[BOB_DAMPING];
            if (extra->bob[BOB_DEPTH] < 0.0F) {
                extra->bob[BOB_DEPTH] = 0.0F;
            }
            y = extra->bob[BOB_HOME_Y] -
                _builtin_fabsf(
                    extra->bob[BOB_DEPTH] *
                    njSin((Angle)(extra->bob[BOB_PHASE] * ANGLE_PER_DEGREE)));
            force->speed.y = y - work->pos.y;
            work->pos.y = y;
        }
    } else {
        work->tick = 0;
        work->timer = 0;
        extra->bob[BOB_DEPTH] = START_DEPTH;
        extra->bob[BOB_PHASE] = 0.0F;
        extra->bob[BOB_DAMPING] = START_DAMPING;
        work->pos.y += RISE_SPEED;
        if (extra->bob[BOB_HOME_Y] < work->pos.y) {
            work->pos.y = extra->bob[BOB_HOME_Y];
        }
    }
    work->ang.x = angle_approach(work->ang.x, tilt_x * 2, TILT_STEP);
    work->ang.z = angle_approach(work->ang.z, tilt_z * 2, TILT_STEP);
    object = VARS(work)->world;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    if (player_in_range(&work->pos, object->model->r + WORLD_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* 1 while player 0 is inside the rock's cylinder, else 0 */
static Sint8 beach_bobbing_rock_player_inside(struct task_work *work)
{
    struct task *player;
    struct task_work *player_work;
    Float distance;
    Float height;
    Float limit;

    if ((player = player_get_task(0)) != NULL) {
        player_work = player->work;
        /* @hack redundant: the jump to the next statement, which puts the
         * player's task in R5, not R4, at 0x0c902712; without it, 2 of 57
         * words differ */
        goto measure;
    measure:
        distance = ((player_work->pos.x - work->pos.x) *
                    (player_work->pos.x - work->pos.x)) +
                   ((player_work->pos.z - work->pos.z) *
                    (player_work->pos.z - work->pos.z));
        height = _builtin_fabsf(player_work->pos.y - work->pos.y);
        limit = njSin(NJM_DEG_ANG(50)) * INSIDE_RADIUS;
        /* @hack spelling: the square of INSIDE_RADIUS first, which loads the
         * distance ahead of it at 0x0c90274a; as distance < INSIDE_RADIUS *
         * INSIDE_RADIUS, 5 of 57 words differ */
        if ((INSIDE_RADIUS * INSIDE_RADIUS) > distance && height < limit) {
            return 1;
        }
    }
    return 0;
}

static void beach_bobbing_rock_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, ROCK_VIEW)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateX, work->ang.x);
        draw_model(&beach_bobbing_rock_model);
        njPopMatrix(1);
    }
}
