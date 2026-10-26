/* act 0's floating bridge, whose planks ride the sea */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg01/bridge.h"
#include "stg01/orca_control.h"
#include "stg01/sea.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT beach_bridge_end_0_object;
extern NJS_OBJECT beach_bridge_end_1_object;
extern NJS_OBJECT beach_bridge_plank_object;
/* drawn between each two planks while the set editor holds the bridge */
extern NJS_OBJECT beach_bridge_joint_object;
extern NJS_OBJECT beach_bridge_end_world_object;
extern NJS_OBJECT beach_bridge_plank_world_object;

/* the bridge's held sound. the step holds it while the sea is off
 * SEA_LEVEL at any plank's far edge */
#define SE_BRIDGE 41

/* the bridge runs along x: an end, PLANKS planks of PLANK_LENGTH, and an
 * end. END_EDGE is the length from an end's middle to its edge, where the
 * planks meet it */
#define PLANKS 25
#define PLANK_LENGTH 40.0F
#define END_EDGE 75.0F
/* from the first end's middle to the last's */
#define LENGTH (END_EDGE + (PLANKS * PLANK_LENGTH) + END_EDGE)
/* a plank rides PLANK_RISE above the bridge's y, plus the sea's height */
#define PLANK_RISE 9.5F
/* the radius of an end's collision record */
#define END_REACH 80.0F
/* the step runs the pieces only while the camera is within 1500 of the
 * bridge's middle. the world tests a piece only while player 0 or 1 is
 * within 95 of it. a piece beyond 600 of the camera hides. each range is
 * squared */
#define STEP_RANGE (1500.0F * 1500.0F)
#define TEST_RANGE (95.0F * 95.0F)
#define DRAW_RANGE (600.0F * 600.0F)

/* a piece's submode, which the bridge's step sets. a hidden piece isn't
 * drawn */
enum bridge_piece_mode { PIECE_SHOWN, PIECE_HIDDEN };

struct bridge_vars {
    Float unk_00;
    Bool sounding; /* SE_BRIDGE is held for the bridge */
};

struct bridge_end_vars {
    NJS_OBJECT *world; /* the end's object in the collision world */
    Uint8 unk_04[4];
    NJS_OBJECT *object; /* what the end draws */
};

struct bridge_plank_vars {
    NJS_OBJECT *world; /* the plank's object in the collision world */
    Uint8 unk_04[4];
    Float unk_08;
};

#define BRIDGE_VARS(work) ((struct bridge_vars *)&(work)->vars)
#define END_VARS(work) ((struct bridge_end_vars *)&(work)->vars)
#define PLANK_VARS(work) ((struct bridge_plank_vars *)&(work)->vars)

static void beach_bridge_init(struct task *task);
static void beach_bridge_step(struct task *task);
static void beach_bridge_draw(struct task *task);
static void beach_bridge_destroy(struct task *task);
static void beach_bridge_end_init(struct task *task);
static void beach_bridge_end_step(struct task *task);
static void beach_bridge_end_draw(struct task *task);
static void beach_bridge_end_destroy(struct task *task);
static void beach_bridge_plank_init(struct task *task);
static void beach_bridge_plank_step(struct task *task);
static void beach_bridge_plank_draw(struct task *task);
static void beach_bridge_plank_destroy(struct task *task);

/* rails along an end's sides, and two across its edge toward the planks.
 * the two leave a gap 40 wide between them, where the planks meet the end */
/* clang-format off */
static struct collision_shape beach_bridge_end_0_shapes[4] = {
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 16.0F, 48.0F},
     75.0F, 5.0F, 2.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 16.0F, -48.0F},
     75.0F, 5.0F, 2.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {73.0F, 16.0F, 35.0F},
     2.0F, 5.0F, 15.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {73.0F, 16.0F, -35.0F},
     2.0F, 5.0F, 15.0F, {0, 0, 0}, 0, NULL},
};
static struct collision_shape beach_bridge_end_1_shapes[4] = {
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 16.0F, 48.0F},
     75.0F, 5.0F, 2.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 16.0F, -48.0F},
     75.0F, 5.0F, 2.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {-73.0F, 16.0F, 35.0F},
     2.0F, 5.0F, 15.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {-73.0F, 16.0F, -35.0F},
     2.0F, 5.0F, 15.0F, {0, 0, 0}, 0, NULL},
};
/* rails along both sides of a plank */
static struct collision_shape beach_bridge_plank_shapes[2] = {
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 7.0F, 23.0F},
     20.0F, 5.0F, 2.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 7.0F, -23.0F},
     20.0F, 5.0F, 2.0F, {0, 0, 0}, 0, NULL},
};
/* clang-format on */

static struct task *beach_bridge_task = NULL;

/* the heights and angles the bridge's pieces ride at. the step samples the
 * sea's height at each edge where two pieces meet. a plank rides PLANK_RISE
 * above the bridge's y, plus the mean of the samples at its two edges. its
 * angle is the slope between those samples. an end rides at the sample at
 * its edge */
static Angle beach_bridge_plank_angles[PLANKS];
static Float beach_bridge_plank_heights[PLANKS];
static Float beach_bridge_end_heights[2];

void beach_bridge_create(void)
{
    struct task_work *work;

    beach_bridge_task = task_create(TASK_WORK, 3, beach_bridge_init);
    work = beach_bridge_task->work;
    work->pos.x = 2803.0F;
    work->pos.y = -1.0F;
    work->pos.z = 365.0F;
}

void beach_bridge_remove(void)
{
    if (beach_bridge_task != NULL) {
        task_kill(beach_bridge_task);
    }
}

/* create the bridge's two ends, then its planks from the first end out.
 * while the set editor holds the bridge, create none. each piece's tick
 * numbers it */
static void beach_bridge_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *child_work;
    struct task *child;
    Float x;
    Float unk;
    int i;

    work->ang.y = 0;
    BRIDGE_VARS(work)->unk_00 = 0.0F;
    BRIDGE_VARS(work)->sounding = FALSE;
    if (!placement_editing(task)) {
        child = task_create_child(TASK_WORK | TASK_FORCE, beach_bridge_end_init,
                                  task);
        child_work = child->work;
        child_work->pos.x = work->pos.x;
        child_work->pos.y = work->pos.y;
        child_work->pos.z = work->pos.z;
        child_work->tick = 0;
        END_VARS(child_work)->object = &beach_bridge_end_0_object;

        child = task_create_child(TASK_WORK | TASK_FORCE, beach_bridge_end_init,
                                  task);
        child_work = child->work;
        child_work->pos.x = work->pos.x + LENGTH;
        child_work->pos.y = work->pos.y;
        child_work->pos.z = work->pos.z;
        child_work->tick = 1;
        END_VARS(child_work)->object = &beach_bridge_end_1_object;

        x = work->pos.x + (END_EDGE + (PLANK_LENGTH / 2));
        unk = 0.0F;
        for (i = 0; i < PLANKS; i++) {
            child = task_create_child(TASK_WORK | TASK_FORCE,
                                      beach_bridge_plank_init, task);
            child_work = child->work;
            child_work->pos.x = x;
            child_work->pos.y = work->pos.y;
            child_work->pos.z = work->pos.z;
            child_work->tick = i;
            PLANK_VARS(child_work)->unk_08 = unk;
            x += PLANK_LENGTH;
            /* 5242.8794F spells the float the pool holds at 0x0c908e3c,
             * 0x45a3d709. the shorter 5242.88F rounds to 0x45a3d70a */
            unk += 5242.8794F;
        }
    }
    task->step = beach_bridge_step;
    task->destructor = beach_bridge_destroy;
}

/* run the bridge only while the camera's squared distance from its middle
 * is within STEP_RANGE. while the set editor holds it, draw it whole.
 * otherwise mark which pieces the world tests and which hide. then sample
 * the sea for the pieces and run them */
static void beach_bridge_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *child_work;
    NJS_POINT3 position;
    Float *at;
    Float camera_x;
    Float camera_y;
    Float camera_z;
    Float player0_x;
    Float player0_y;
    Float player0_z;
    Float player1_x;
    Float player1_y;
    Float player1_z;
    Float x;
    Float y;
    Float z;
    Float distance;
    Float dx;
    Float dy;
    Float dz;
    Float player0_distance;
    Float player1_distance;
    Float previous;
    Float height;
    Float base_y;
    Bool calm; /* the sea is at SEA_LEVEL at every far edge sampled */
    int i;
    Angle *angles;
    Float *heights;

    /* @hack spelling: the camera's pos and the bridge's read through a cursor,
     * which loads them with FMOV.S @R4+ from 0x0c908e86; without it, 264 of
     * 287 words differ */
    at = VECTOR_FLOATS(&camera_work->pos);
    /* @hack spelling: the camera's cursor stepped in statements of its own,
     * which leaves ~WORLD_TESTED in R7 and WORLD_TESTED in R6 for 0x0c908fae
     * and 0x0c908fb2; stepped as *at++, 125 of 287 words differ */
    camera_x = *at;
    at++;
    camera_y = *at;
    at++;
    camera_z = *at;
    at = VECTOR_FLOATS(&work->pos);
    dx = camera_x - *at++ - (LENGTH / 2);
    /* @hack spelling: dy assigned apart from its subtraction, which keeps
     * camera x in FR14 and z in FR15, loaded at 0x0c908e86 and 0x0c908e8c; as
     * one statement, 6 of 287 words differ */
    dy = camera_y;
    dy -= *at++;
    dz = camera_z - *at;
    distance = (dx * dx) + (dy * dy) + (dz * dz);
    if (distance > STEP_RANGE) {
        BRIDGE_VARS(work)->unk_00 += 300.0F;
        return;
    }
    if (placement_editing(task)) {
        beach_bridge_draw(task);
    } else {
        child = task->child;
        /* @bug uninitialized: with no player 0, player_get_history returns
         * FALSE without writing position (JSR at 0x0c908f0c), and the step
         * reads it all the same (0x0c908f14 to 0x0c908f28). with no player
         * 1, the second call writes nothing, and 0x0c908f34 to 0x0c908f4a
         * read what the first left */
        player_get_history(0, 0, &position, NULL);
        /* @hack spelling: player 0's position read through the cursor, which
         * loads it with FMOV.S @R4+ from 0x0c908f14; without it, 99 of 287
         * words differ */
        at = VECTOR_FLOATS(&position);
        player0_x = *at++;
        player0_y = *at++;
        player0_z = *at;
        player_get_history(1, 0, &position, NULL);
        /* @hack spelling: player 1's position read the same way, from
         * 0x0c908f34; without it, 81 of 287 words differ */
        at = VECTOR_FLOATS(&position);
        player1_x = *at++;
        player1_y = *at++;
        player1_z = *at;
        while (child != NULL) {
            child_work = child->work;
            /* @hack redundant: a never-run test of child, whose reads keep
             * child in R13 and 0 in R12, at 0x0c908f04 and 0x0c908f58; without
             * it, 55 of 287 words differ */
            if (0) {
                /* NOLINTNEXTLINE(misc-redundant-expression) */
                if (child && child && child) {
                }
            }
            x = child_work->pos.x;
            y = child_work->pos.y;
            z = child_work->pos.z;
            /* @hack redundant: a never-run test of x, which keeps the piece's x
             * in FR7, loaded at 0x0c908f60; without it, 55 of 287 words
             * differ */
            if (0) {
                if (x) {
                }
            }
            /* @hack spelling: dy ahead of dx, which keeps player 0's y in FR12,
             * loaded at 0x0c908f20; with dx first, 15 of 287 words differ */
            dy = player0_y - y;
            dx = player0_x - x;
            player0_distance = dx * dx;
            player0_distance += dy * dy;
            dz = player0_z - z;
            player0_distance += dz * dz;
            dx = player1_x - x;
            player1_distance = dx * dx;
            dy = player1_y - y;
            player1_distance += dy * dy;
            dz = player1_z - z;
            player1_distance += dz * dz;
            /* @hack redundant: a never-run test of player0_distance, whose
             * reads keep player 0's distance in FR4, from 0x0c908f6c; without
             * it, 72 of 287 words differ */
            if (0) {
                /* NOLINTNEXTLINE(misc-redundant-expression) */
                if (player0_distance && player0_distance) {
                }
            }
            /* @hack redundant: a never-run test against TEST_RANGE, which
             * keeps TEST_RANGE in FR6, loaded at 0x0c908f54; without it, 67 of
             * 287 words differ */
            if (0) {
                if (player0_distance > TEST_RANGE) {
                }
            }
            if (player0_distance > TEST_RANGE &&
                player1_distance > TEST_RANGE) {
                child_work->flags &= ~WORLD_TESTED;
            } else {
                child_work->flags |= WORLD_TESTED;
            }
            dx = camera_x - x;
            distance = dx * dx;
            /* @hack spelling: y, reused for its own delta, which keeps the
             * piece's y in FR5, loaded at 0x0c908f64; as dy, 70 of 287 words
             * differ */
            y = camera_y - y;
            distance += y * y;
            dz = camera_z - z;
            distance += dz * dz;
            if (distance > DRAW_RANGE) {
                child_work->submode = PIECE_HIDDEN;
            } else {
                child_work->submode = PIECE_SHOWN;
            }
            child = child->next;
        }
        /* @hack redundant: never-run stores of sounding, which keep work in
         * R14, from 0x0c908e7e; without it, 122 of 287 words differ */
        if (0) {
            BRIDGE_VARS(work)->sounding = FALSE;
            BRIDGE_VARS(work)->sounding = FALSE;
            BRIDGE_VARS(work)->sounding = FALSE;
            BRIDGE_VARS(work)->sounding = FALSE;
        }
        /* @hack redundant: a never-run test of work and task, whose reads keep
         * camera x in FR14 and y in FR13, loaded at 0x0c908e86 and 0x0c908e88;
         * without it, 60 of 287 words differ */
        if (0) {
            /* NOLINTNEXTLINE(misc-redundant-expression) */
            if (work && work && task) {
            }
        }
        x = work->pos.x + END_EDGE;
        z = work->pos.z;
        calm = TRUE;
        previous = beach_sea_height(x, z);
        beach_bridge_end_heights[0] = previous;
        base_y = work->pos.y + PLANK_RISE;
        angles = beach_bridge_plank_angles;
        heights = beach_bridge_plank_heights;
        for (i = 0; i < PLANKS; i++) {
            x += PLANK_LENGTH;
            height = beach_sea_height(x, z);
            if (height != SEA_LEVEL) {
                calm = FALSE;
            }
            /* @hack redundant: a never-run test of calm, which keeps the locals
             * in 44 bytes, ADD #-44,R15 at 0x0c908e78; without it, 123 of 287
             * words differ */
            if (0) {
                if (calm) {
                }
            }
            *heights++ = base_y + ((previous + height) * 0.5F);
            *angles++ = (Angle)(atan2f(height - previous, PLANK_LENGTH) *
                                ANGLE_PER_RADIAN);
            previous = height;
        }
        beach_bridge_end_heights[1] = height;
        if (BRIDGE_VARS(work)->sounding == TRUE && calm == TRUE) {
            sound_stop_id((int)work);
            BRIDGE_VARS(work)->sounding = FALSE;
        } else if (BRIDGE_VARS(work)->sounding == FALSE && calm == FALSE) {
            sound_play_held(SE_BRIDGE, (int)work, 1, 2000);
            BRIDGE_VARS(work)->sounding = TRUE;
        }
        task_step_children(task);
    }
}

/* the whole bridge, at rest: the first end, the planks with a joint between
 * each two, and the last end */
static void beach_bridge_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    int i;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&beach_bridge_end_0_object);
    njTranslate(NULL, END_EDGE, PLANK_RISE, 0.0F);
    for (i = 0; i < PLANKS - 1; i++) {
        njTranslate(NULL, PLANK_LENGTH / 2, 0.0F, 0.0F);
        draw_tree(&beach_bridge_plank_object);
        njTranslate(NULL, PLANK_LENGTH / 2, 0.0F, 0.0F);
        draw_tree(&beach_bridge_joint_object);
    }
    njTranslate(NULL, PLANK_LENGTH / 2, 0.0F, 0.0F);
    draw_tree(&beach_bridge_plank_object);
    njTranslate(NULL, (PLANK_LENGTH / 2) + END_EDGE, 0.0F, 0.0F);
    draw_tree(&beach_bridge_end_1_object);
    njPopMatrix(1);
}

static void beach_bridge_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void beach_bridge_end_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;

    work->submode = PIECE_SHOWN;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    END_VARS(work)->world = object = world_object_alloc();
    object->evalflags = NJD_EVAL_BREAK;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = beach_bridge_end_world_object.model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(WORLD_SMALL | WORLD_MOVES | WORLD_SOLID, task, object);
    work->flags |= WORLD_TESTED;
    if (work->tick == 0) {
        collision_init(task, beach_bridge_end_0_shapes, COLLISION_OBJECT, 1, 4);
    } else {
        collision_init(task, beach_bridge_end_1_shapes, COLLISION_OBJECT, 1, 4);
    }
    work->collision->radius = END_REACH;
    task->step = beach_bridge_end_step;
    task->draw = beach_bridge_end_draw;
    task->destructor = beach_bridge_end_destroy;
}

/* draw, then ride the sea at the end's edge */
static void beach_bridge_end_step(struct task *task)
{
    struct task_work *work = task->work;
    Float height;

    beach_bridge_end_draw(task);
    height = beach_bridge_end_heights[work->tick];
    work->pos.y = height;
    END_VARS(work)->world->pos[1] = height;
    collision_register(work);
}

static void beach_bridge_end_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    if (work->submode == PIECE_SHOWN) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(END_VARS(work)->object);
        njPopMatrix(1);
    }
}

static void beach_bridge_end_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (END_VARS(work)->world != NULL) {
        world_remove(task, END_VARS(work)->world);
        world_object_free(END_VARS(work)->world);
    }
}

static void beach_bridge_plank_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;

    work->submode = PIECE_SHOWN;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    PLANK_VARS(work)->world = object = world_object_alloc();
    object->evalflags = NJD_EVAL_BREAK;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = beach_bridge_plank_world_object.model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(WORLD_SMALL | WORLD_MOVES | WORLD_SOLID, task, object);
    work->flags |= WORLD_TESTED;
    collision_init(task, beach_bridge_plank_shapes, COLLISION_OBJECT, 1, 2);
    task->step = beach_bridge_plank_step;
    task->draw = beach_bridge_plank_draw;
    task->destructor = beach_bridge_plank_destroy;
}

/* draw, then ride the sea. the slope's angle turns the plank about z. it
 * turns the plank about x by the same angle too, as drawn and in the
 * collision world */
static void beach_bridge_plank_step(struct task *task)
{
    struct task_work *work = task->work;
    int plank;
    Float height;
    Angle angle;

    beach_bridge_plank_draw(task);
    plank = work->tick;
    height = beach_bridge_plank_heights[plank];
    angle = beach_bridge_plank_angles[plank];
    work->pos.y = height;
    work->ang.z = angle;
    PLANK_VARS(work)->world->pos[1] = height;
    PLANK_VARS(work)->world->ang[2] = angle;
    PLANK_VARS(work)->world->ang[0] = angle;
    collision_register(work);
}

static void beach_bridge_plank_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (work->submode == PIECE_SHOWN) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.z, ang);
        draw_tree(&beach_bridge_plank_object);
        njPopMatrix(1);
    }
}

static void beach_bridge_plank_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (PLANK_VARS(work)->world != NULL) {
        world_remove(task, PLANK_VARS(work)->world);
        world_object_free(PLANK_VARS(work)->world);
    }
}
