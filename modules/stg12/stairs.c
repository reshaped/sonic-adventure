/* O KAIDAN, stairs on wheels that players push along a run */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg12/stairs.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT stairs_object;
extern NJS_OBJECT stairs_world_object;

/* the flags of the stairs' body in the collision world */
#define BODY_WORLD (WORLD_SMALL | WORLD_MOVES | WORLD_SOLID)

/* the share of the players' push along the run that rolls the stairs */
#define PUSH_SHARE 0.4F
/* the wheels' turn for each unit the stairs roll */
#define WHEEL_TURN 5000.0F

/* the box takes part in the collision checks only while player 0 is within this
 * height of the stairs' foot */
#define PUSH_HEIGHT 1.0F

/* the stairs roll along a run on their heading. work->scl.x holds half the
 * run's length. work->scl.y holds the stairs' place along the run, measured
 * from the run's center. the set entry gives both */
struct stairs_vars {
    NJS_OBJECT *body; /* the stairs' object in the collision world */
    Float sine; /* the heading's sine */
    Float cosine; /* the heading's cosine */
};

/* task->motion as the stairs lay it out. carry_reach is the word struct
 * task_motion names drive, where carry_set_kind sets the reach */
struct stairs_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    NJS_VECTOR accel; /* the players' push this step */
    Angle wheels;
    Uint8 unk_20[0x14];
    Float carry_reach;
    Float roll; /* how far the stairs roll this step, cleared once used */
    Float gravity;
};

#define VARS(work) ((struct stairs_vars *)&(work)->vars)

static void stairs_run(struct task *task);
static void stairs_draw(struct task *task);
static void stairs_destroy(struct task *task);

/* the box a player pushes the stairs by */
static struct collision_shape stairs_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
    {-4.0F, 5.0F, 0.0F},
    10.0F,
    10.0F,
    15.0F,
    {0},
    0,
    NULL,
};

void stairs_step(struct task *task)
{
    struct task_work *work = task->work;
    struct stairs_motion *motion = (struct stairs_motion *)task->motion;
    struct task_force *force = task->force;
    Angle heading = work->ang.y;
    NJS_OBJECT *body;
    Float range;

    VARS(work)->sine = njSin(heading);
    VARS(work)->cosine = njCos(heading);
    range = work->scl.x;
    /* @bug logic: the second test reads the range, scl.x, where it means
     * the place, scl.y (FMOV.S @(R0,R14),FR3 with R0 44 at 0x0c90c1a4). for a
     * range of 0 or more it never holds, so a place below -range stays as the
     * set entry gives it */
    if (work->scl.y > range) {
        work->scl.y = range;
    } else if (work->scl.x < -range) {
        work->scl.y = -range;
    }
    motion->roll = 0.0F;
    motion->gravity = 0.0F;
    motion->speed.x = 0.0F;
    motion->speed.y = 0.0F;
    motion->speed.z = 0.0F;
    motion->accel.x = 0.0F;
    motion->accel.y = 0.0F;
    motion->accel.z = 0.0F;
    motion->wheels = 0;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        body = world_object_alloc();
        VARS(work)->body = body;
        body->evalflags = NJD_EVAL_BREAK;
        body->scl[0] = 1.0F;
        body->scl[1] = 1.0F;
        body->scl[2] = 1.0F;
        body->ang[0] = 0;
        body->ang[1] = work->ang.y;
        body->ang[2] = 0;
        body->pos[0] = work->pos.x + stairs_world_object.pos[0];
        body->pos[1] = work->pos.y + stairs_world_object.pos[1];
        body->pos[2] = work->pos.z + stairs_world_object.pos[2];
        body->model = stairs_world_object.model;
        body->child = NULL;
        body->sibling = NULL;
        world_add(BODY_WORLD, task, body);
        work->flags |= WORLD_TESTED;
        collision_init(task, &stairs_shape, COLLISION_OBJECT, 1, 1);
    }
    /* so players can push the stairs */
    carry_set_kind(work, (struct task_motion *)motion, CARRY_KIND_MEDIUM);
    task->step = stairs_run;
    task->draw = stairs_draw;
    task->destructor = stairs_destroy;
}

static void stairs_run(struct task *task)
{
    struct task_work *work = task->work;
    struct stairs_motion *motion = (struct stairs_motion *)task->motion;
    NJS_POINT3 player;
    Angle ang;
    Float push_x;
    Float push_z;
    Float sine;
    Float cosine;
    Float push;
    Float roll;
    Float along;
    Float range;
    Float x;
    Float z;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> MOVABLE RANGE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> START POSITION");
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njTranslate(NULL, 0.0F, 0.0F, -work->scl.y);
        njScale(NULL, 1.0F, 1.0F, (work->scl.x + 12.0F) * 0.2F);
        draw_tree(marker_cube_object);
        njPopMatrix(1);
    } else {
        push_x = motion->accel.x;
        push_z = motion->accel.z;
        if (push_x != 0.0F || push_z != 0.0F) {
            sine = VARS(work)->sine;
            cosine = VARS(work)->cosine;
            push = (push_x * sine) + (push_z * cosine);
            if (push != 0.0F) {
                push *= PUSH_SHARE;
                along = work->scl.y;
                range = work->scl.x;
                roll = motion->roll + push;
                along += roll;
                if (along >= 0.0F) {
                    if (along > range) {
                        roll -= along - range;
                        along = range;
                    }
                } else {
                    /* @hack spelling: the negation written as a multiply by
                     * -1.0F, which SHC loads from the pool and multiplies
                     * (0x0c90c3d0 to 0x0c90c3d4); as -range, it negates it
                     * with FNEG, and 103 of 197 words and 10 of 59 other
                     * words differ */
                    range *= -1.0F;
                    if (along < range) {
                        roll -= along - range;
                        along = range;
                    }
                }
                work->scl.y = along;
                motion->roll = roll;
                motion->wheels += (Angle)(roll * WHEEL_TURN);
                x = work->pos.x;
                z = work->pos.z;
                x += roll * sine;
                z += roll * cosine;
                work->pos.x = x;
                work->pos.z = z;
                VARS(work)->body->pos[0] = x + stairs_world_object.pos[0];
                VARS(work)->body->pos[2] = z + stairs_world_object.pos[2];
            }
        }
        motion->roll = 0.0F;
        motion->accel.x = 0.0F;
        motion->accel.z = 0.0F;
        player_get_history(0, 0, &player, NULL);
        /* @bug uninitialized: with no player 0, player_get_history returns
         * FALSE without writing player (JSR at 0x0c90c43c), and the test
         * reads player.y from the stack all the same (FMOV.S @(R0,R15),FR2
         * at 0x0c90c446) */
        if (njAbs(player.y - work->pos.y) < PUSH_HEIGHT) {
            collision_register(work);
        }
    }
    stairs_draw(task);
}

/* the model's first child is the stairs' structure, and the next two the
 * wheels */
static void stairs_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct stairs_motion *motion = (struct stairs_motion *)task->motion;
    NJS_OBJECT *node;
    Angle wheels;
    Angle ang;

    if (frame_step == 0) {
        wheels = motion->wheels;
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(stairs_object.model);
        node = stairs_object.child;
        njPushMatrix(NULL);
        njTranslate(NULL, node->pos[0], node->pos[1], node->pos[2]);
        draw_model(node->model);
        njPopMatrix(1);
        node = node->sibling;
        njPushMatrix(NULL);
        njTranslate(NULL, node->pos[0], node->pos[1], node->pos[2]);
        TURN(njRotateX, wheels);
        njScale(NULL, node->scl[0], node->scl[1], node->scl[2]);
        draw_model(node->model);
        njPopMatrix(1);
        node = node->sibling;
        njPushMatrix(NULL);
        njTranslate(NULL, node->pos[0], node->pos[1], node->pos[2]);
        TURN(njRotateX, wheels);
        njScale(NULL, node->scl[0], node->scl[1], node->scl[2]);
        draw_model(node->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void stairs_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->body != NULL) {
        world_remove(task, VARS(work)->body);
        world_object_free(VARS(work)->body);
    }
}
