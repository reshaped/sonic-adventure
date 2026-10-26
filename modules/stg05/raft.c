/* O HAKA, a tomb, and O CHAIR, a chair. O MASIBA, a raft that rises with the
 * lava and tips toward player 0 while it is near */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg05/mountain.h"
#include "stg05/raft.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT tomb_object;
extern NJS_OBJECT chair_object;
extern NJS_MODEL raft_model;
extern NJS_OBJECT raft_object;

static struct collision_shape tomb_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 5.0F, 0.0F},
    7.0F,
    10.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

static struct collision_shape chair_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 5.0F, 0.0F},
    3.0F,
    8.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

struct raft_vars {
    NJS_OBJECT *world; /* the raft's object in the world */
    Uint8 unk_04[4];
    /* set to RAFT_SINK on player 0's first step near the raft, and to 0 once
     * tick reaches NEAR_STEPS. no code reads it */
    Float sink;
};

#define VARS(work) ((struct raft_vars *)&(work)->vars)

enum raft_mode { RAFT_INIT, RAFT_FLOAT, RAFT_IDLE };

/* player 0 is near the raft while within RAFT_NEAR of it across x and z. it
 * must also be less than RAFT_NEAR times the sine of RAFT_NEAR_ANGLE above
 * or below it */
#define RAFT_NEAR 50.0F
#define RAFT_NEAR_ANGLE 0x238e /* 50 degrees */
/* the most the raft's tilt turns each step */
#define TIP_STEP 15
/* the world tests the raft's object only while a player is within this */
#define RAFT_REACH 90.0F
#define DRAW_RADIUS 50.0F
/* tick counts player 0's steps near the raft since it came near, up to
 * NEAR_STEPS. sink takes RAFT_SINK on the first */
#define NEAR_STEPS 200
#define RAFT_SINK (-0.035F)

static void raft_destroy(struct task *task);
static void raft_add_world(struct task *task);
static void raft_float(struct task *task);
static Sint8 raft_player_near(struct task_work *work);
static void raft_draw(struct task *task);

void tomb_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &tomb_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, &tomb_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void chair_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &chair_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, &chair_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

/* unless the set editor holds the raft, it floats. it starts at the lava's
 * level when mountain_work is set */
void raft_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case RAFT_INIT:
            task->destructor = raft_destroy;
            task->draw = raft_draw;
            raft_add_world(task);
            work->mode = RAFT_FLOAT;
            if (mountain_work && !placement_editing(task)) {
                work->pos.y = mountain_work->pos.y;
            }
            break;
        case RAFT_FLOAT:
            if (!placement_editing(task)) {
                raft_float(task);
            }
            break;
        case RAFT_IDLE:
            break;
        }
        raft_draw(task);
    }
}

static void raft_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->world);
    world_object_free(VARS(work)->world);
}

/* put a new object with raft_object's model in the world. it stands at the
 * raft's place, turned as the raft is about x and z. work->scl.y keeps the
 * raft's height, which no code reads */
static void raft_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    VARS(work)->world = world = world_object_alloc();
    work->scl.y = work->pos.y;
    world->evalflags = NJD_EVAL_BREAK;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = 0;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    /* @hack spelling: the model through the object's address (0x0c905ef0); from
     * the object itself, the member's address takes the pool word and 5 of
     * 58 words differ */
    world->model = (&raft_object)->model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WORLD_MOVES, task, world);
    work->flags |= WORLD_TESTED;
}

/* move as far as the lava's level moved this step. while player 0 is near, tip
 * toward it by a sixth of its angle from the raft's vertical. the tilt about z
 * measures that angle across x, and the tilt about x across z. the tilt turns
 * TIP_STEP a step at most, and back to level once player 0 leaves. the raft's
 * object in the world is made again each step */
static void raft_float(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    Angle tilt_z = 0;
    Angle tilt_x = 0;
    struct task *player;
    struct task_work *rider;
    Float dx;
    Float dz;
    Float scale;
    /* @hack spelling: 0 through a local, which keeps a zero in R8 for the
     * speed and the sink (MOV #0,R8 at 0x0c905f24); as the literal, R9 takes
     * it, the function is 2 bytes shorter and 89 of 219 words differ */
    Float zero;
    /* @hack spelling: the factor and atan2f through locals, which keep dx in
     * FR13 and the factor in FR14 (0x0c905fea, 0x0c905ff0); by name, the two
     * move up a register and 17 of 219 words differ */
    Float radians_to_angle;
    Float (*arctangent)(Float, Float);
    int share;

    if ((player = player_get_task(0)) != NULL) {
        rider = player->work;
        /* @hack redundant: a goto to the next statement, which puts
         * mountain_work's address in R2 (0x0c905f46); without it, R3 takes it
         * and 2 of 219 words differ */
        goto lava;
    lava:
        zero = 0.0F;
        if (mountain_work &&
            (mountain_work->scl.y != 0.0F ||
             MOUNTAIN_VARS(mountain_work)->lava_moved != 0.0F)) {
            force->speed.y = MOUNTAIN_VARS(mountain_work)->lava_moved;
        } else {
            force->speed.y = zero;
        }
        if (raft_player_near(work) == 1) {
            /* each step near adds share, NEAR_STEPS / (tick + 1), to timer,
             * which no code reads */
            if (work->tick == 0) {
                VARS(work)->sink = RAFT_SINK;
            }
            share = NEAR_STEPS / (work->tick + 1);
            if (share < 1) {
                VARS(work)->sink = zero;
            } else {
                work->tick++;
            }
            work->timer += share;
            /* @hack redundant: a never-run test of rider, which gives force R12
             * and rider R13 (0x0c905f36, 0x0c905f48), which otherwise trade;
             * without it, 11 of 219 words differ */
            if (0) {
                if (rider) {
                }
            }
            dx = rider->pos.x - work->pos.x;
            radians_to_angle = ANGLE_PER_RADIAN;
            arctangent = atan2f;
            scale = 1.0F / 12.0F;
            /* @hack redundant: a never-run store of scale, one of three that
             * each keep task in R11 and force in R12 (0x0c905f1c, 0x0c905f36),
             * and the radian factor in FR14 and the scale in FR15 (0x0c905ff0,
             * 0x0c905ffc), which otherwise trade; without it, 17 of 219 words
             * differ */
            if (0) {
                force->speed.y = scale;
            }
            /* @hack redundant: the same, a second store of scale; without it,
             * 17 of 219 words differ */
            if (0) {
                force->speed.y = scale;
            }
            /* @hack redundant: the same, a third store of scale; without it, 17
             * of 219 words differ */
            if (0) {
                force->speed.y = scale;
            }
            if (dx > 0.0F) {
                tilt_z = -(Angle)(arctangent(dx, rider->pos.y - work->pos.y) *
                                  radians_to_angle);
                /* @hack redundant: a never-run call, which keeps the angle in
                 * R9, a register calls preserve: FMOV.S FR0,FR3 at 0x0c90600a
                 * and STS FPUL,R9 at 0x0c906010, and the function 32 bytes
                 * longer; without it, 124 of 219 words differ */
                if (0) {
                    raft_add_world(task);
                }
            } else {
                tilt_z = (Angle)(arctangent(_builtin_fabsf(dx),
                                            rider->pos.y - work->pos.y) *
                                 radians_to_angle);
            }
            /* @hack redundant: a select whose condition is constant, which
             * keeps tilt_z in R9 (0x0c906054) and has its FLOAT write FR3,
             * copied to FR2 (0x0c90605c, 0x0c90605e); without it, 102 of 219
             * words differ */
            tilt_z = (Angle)((1 ? (Float)tilt_z : 0.0F) * scale);
            /* @hack redundant: a never-run test of dx, which keeps 0 in R8,
             * copied to R10 and R9 (0x0c905f24 to 0x0c905f2c), atan2f's address
             * in R8 (0x0c905fe2), and dx's copy to FR4 ahead of the FABS
             * (0x0c906042); without it, 21 of 219 words differ */
            if (0) {
                if (dx) {
                }
            }
            /* @hack redundant: a never-run store of tilt_x, which keeps
             * atan2f's address in R8 and tilt_z in R9 (0x0c905fe2, 0x0c906010);
             * without it, 66 of 219 words differ */
            if (0) {
                work->ang.x = tilt_x;
            }
            /* @hack redundant: a never-run test of tilt_z, which keeps 0 in R8,
             * copied to R10 and R9 (0x0c905f24 to 0x0c905f2c), and atan2f's
             * address in R8 (0x0c905fe2); without it, 19 of 219 words differ */
            if (0) {
                if (tilt_z) {
                }
            }
            dz = rider->pos.z - work->pos.z;
            if (dz > 0.0F) {
                tilt_x = (Angle)(arctangent(dz, rider->pos.y - work->pos.y) *
                                 radians_to_angle);
                /* @hack redundant: a never-run call, which, as for tilt_z,
                 * keeps the angle in R10, a register calls preserve: FMOV.S
                 * FR0,FR3 at 0x0c90607c and STS FPUL,R10 at 0x0c906084; without
                 * it, 71 of 219 words differ */
                if (0) {
                    raft_add_world(task);
                }
            } else {
                tilt_x = -(Angle)(arctangent(_builtin_fabsf(dz),
                                             rider->pos.y - work->pos.y) *
                                  radians_to_angle);
            }
            /* @hack redundant: a never-run fabs of tilt_x, which keeps 0 in R8,
             * copied to R10 and R9 (0x0c905f24 to 0x0c905f2c), atan2f's address
             * in R8 (0x0c905fe2), and has tilt_x's FLOAT write FR3, copied to
             * FR2 (0x0c9060a0, 0x0c9060a2); without it, 23 of 219 words
             * differ */
            if (0) {
                _builtin_fabsf((Float)tilt_x);
            }
            /* @hack redundant: a select whose condition is constant, which
             * keeps tilt_x in R10 (0x0c90609a), not in a frame slot; without
             * it, 168 of 219 words differ */
            tilt_x = (Angle)((1 ? (Float)tilt_x : 0.0F) * scale);
            /* @hack redundant: a never-run test of tilt_x, which keeps tilt_z
             * in R9 and tilt_x in R10 (0x0c906010, 0x0c9060aa), not in a frame
             * slot, and the function 2 bytes shorter; without it, 129 of 219
             * words differ */
            if (0) {
                if (tilt_x) {
                }
            }
        } else {
            work->tick = 0;
            work->timer = 0;
        }
        work->ang.x = angle_approach(work->ang.x, tilt_x * 2, TIP_STEP);
        work->ang.z = angle_approach(work->ang.z, tilt_z * 2, TIP_STEP);
        work->pos.y += force->speed.y;
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
        raft_add_world(task);
        if (player_in_range(&work->pos, RAFT_REACH)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

/* 1 when player 0 is near the raft, as RAFT_NEAR says, else 0 */
static Sint8 raft_player_near(struct task_work *work)
{
    struct task *player;
    struct task_work *rider;
    Float distance;
    Float height;
    Float limit;

    if ((player = player_get_task(0)) != NULL) {
        rider = player->work;
        /* @hack redundant: a label nothing needs, which puts player's work in
         * R5 (0x0c906172); without it, R4 takes it and 2 of 57 words differ */
    rider_ready: /* NOLINT(clang-diagnostic-unused-label) */
        distance = SQUARE(rider->pos.x - work->pos.x) +
                   SQUARE(rider->pos.z - work->pos.z);
        height = _builtin_fabsf(rider->pos.y - work->pos.y);
        limit = njSin(RAFT_NEAR_ANGLE) * RAFT_NEAR;
        if (SQUARE(RAFT_NEAR) > distance && height < limit) {
            return 1;
        }
    }
    return 0;
}

static void raft_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&raft_model);
        njPopMatrix(1);
    }
}
