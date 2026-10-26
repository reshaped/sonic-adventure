/* O ARCHROCK, a rock arch standing on two big rocks. its top falls in
 * pieces once either rock starts to topple */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg01/arch_rock.h"
#include "stg01/big_rock.h"
#include "stg01/sea.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT arch_rock_object;
extern NJS_OBJECT arch_rock_piece_1_object;
extern NJS_OBJECT arch_rock_piece_2_object;
extern NJS_OBJECT arch_rock_piece_3_object;
extern NJS_OBJECT arch_rock_piece_4_object;
extern NJS_OBJECT arch_rock_piece_5_object;
extern NJS_OBJECT arch_rock_piece_6_object;
extern NJS_OBJECT arch_rock_piece_7_object;

/* the arch destroys itself once the camera and player 0 are both beyond
 * 1000 of it. it stays while player 1 is Sonic within 40 of it. the range is
 * squared, as placement_try_leave_by takes it */
#define LEAVE_RANGE (1000.0F * 1000.0F)
/* the radius of the arch's collision record, which holds the top's capsule */
#define ARCH_REACH 150.0F

/* where the parts stand in the arch's frame. big rock a stands LEG_A_Z ahead
 * along z, big rock b LEG_B_Z behind, and the top TOP_Y up */
#define LEG_A_Z 73.0F
#define LEG_B_Z 57.0F
#define TOP_Y 110.0F

/* the top breaks into TOP_PIECES pieces FALL_WAIT + 2 steps after a big rock
 * starts to topple. the arch tests its rocks before it steps them, so it sees
 * the topple a step late. its timer then counts up from 0, and the top breaks
 * once the timer passes FALL_WAIT */
#define FALL_WAIT 8
#define TOP_PIECES 7

/* a piece flies off at a random speed. in x and in z, that speed is within
 * PIECE_SPREAD / 2 of 0. its speed up is below PIECE_LIFT */
#define PIECE_SPREAD 3.4F
#define PIECE_LIFT 3.0F
/* what a falling piece's upward speed loses each step, and how far a sinking
 * piece sinks each step */
#define PIECE_GRAVITY 0.1F
/* the share of its speed in x and z a piece keeps each step. FALL_KEEP
 * holds while it falls, and SINK_KEEP once it sinks */
#define FALL_KEEP 0.995F
#define SINK_KEEP 0.97F
/* how far a falling piece turns about x and about z each step, 65536 a
 * turn */
#define TUMBLE_X 500
#define TUMBLE_Z 800

struct arch_rock_vars {
    Float unk_00;
};

/* what a piece keeps in work->vars. its speed is in work->scl */
struct piece_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *part; /* its part of the top */
};

#define VARS(work) ((struct arch_rock_vars *)&(work)->vars)
#define PIECE(work) ((struct piece_vars *)&(work)->vars)

enum arch_rock_mode {
    ARCH_ROCK_STAND = 0,
    ARCH_ROCK_BREAK = 1,
    ARCH_ROCK_FALLEN = 3
};

/* the arch's parts, which index arch_rock_parts and arch_rock_part_counts */
enum arch_rock_part { PART_LEG_A, PART_TOP, PART_LEG_B, ARCH_ROCK_PARTS };

enum piece_mode { PIECE_FALL = 0, PIECE_SINK = 2 };

static void arch_rock_step(struct task *task);
static void arch_rock_draw(struct task *task);
static void arch_rock_destroy(struct task *task);
static void arch_rock_piece_start(struct task *task);
static void arch_rock_piece_step(struct task *task);
static void arch_rock_piece_draw(struct task *task);
static void arch_rock_piece_destroy(struct task *task);

/* each part's object whole, then the objects of its pieces */
static NJS_OBJECT *arch_rock_leg_a_parts[10] = {
    &beach_big_rock_a_object,         &beach_big_rock_a_piece_1_object,
    &beach_big_rock_a_piece_2_object, &beach_big_rock_a_piece_3_object,
    &beach_big_rock_a_piece_4_object, &beach_big_rock_a_piece_5_object,
    &beach_big_rock_a_piece_6_object, &beach_big_rock_a_piece_7_object,
    &beach_big_rock_a_piece_8_object, &beach_big_rock_a_piece_9_object,
};
static NJS_OBJECT *arch_rock_top_parts[TOP_PIECES + 1] = {
    &arch_rock_object,         &arch_rock_piece_1_object,
    &arch_rock_piece_2_object, &arch_rock_piece_3_object,
    &arch_rock_piece_4_object, &arch_rock_piece_5_object,
    &arch_rock_piece_6_object, &arch_rock_piece_7_object,
};
static NJS_OBJECT *arch_rock_leg_b_parts[11] = {
    &beach_big_rock_b_object,          &beach_big_rock_b_piece_1_object,
    &beach_big_rock_b_piece_2_object,  &beach_big_rock_b_piece_3_object,
    &beach_big_rock_b_piece_4_object,  &beach_big_rock_b_piece_5_object,
    &beach_big_rock_b_piece_6_object,  &beach_big_rock_b_piece_7_object,
    &beach_big_rock_b_piece_8_object,  &beach_big_rock_b_piece_9_object,
    &beach_big_rock_b_piece_10_object,
};

static int arch_rock_part_counts[ARCH_ROCK_PARTS] = {10, TOP_PIECES + 1, 11};
static NJS_OBJECT **arch_rock_parts[ARCH_ROCK_PARTS] = {
    arch_rock_leg_a_parts,
    arch_rock_top_parts,
    arch_rock_leg_b_parts,
};

/* the top's capsule, turned about x to lie along the arch */
/* clang-format off */
static struct collision_shape arch_rock_shape = {
    SHAPE_CAPSULE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 120.0F, 0.0F},
    20.0F, 10.0F, 0.0F, {NJM_DEG_ANG(90), 0, 0}, 0, NULL,
};
/* clang-format on */

void arch_rock_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *rock;
    Float *at;
    Float x;
    Float y;
    Float z;
    Float sin;
    Float cos;

    work->mode = ARCH_ROCK_STAND;
    VARS(work)->unk_00 = 0.0F;
    if (!placement_editing(task)) {
        collision_init(task, &arch_rock_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = ARCH_REACH;
        /* @hack spelling: the position read a float at a time through a
         * pointer, which loads it with FMOV.S @R4+ from 0x0c90b248; without it,
         * 87 of 109 words differ */
        at = VECTOR_FLOATS(&work->pos);
        x = *at++;
        y = *at++;
        z = *at;
        sin = njSin(work->ang.y);
        cos = njCos(work->ang.y);
        child = task_create_child(TASK_WORK | TASK_MOTION,
                                  beach_big_rock_child_init, task);
        rock = child->work;
        rock->scl.y = 0.0F; /* its kind: big rock a */
        rock->pos.x = x + (sin * LEG_A_Z);
        rock->pos.y = y;
        rock->pos.z = z + (cos * LEG_A_Z);
        child = task_create_child(TASK_WORK | TASK_MOTION,
                                  beach_big_rock_child_init, task);
        rock = child->work;
        rock->scl.y = 1.0F; /* its kind: big rock b */
        rock->pos.x = x - (sin * LEG_B_Z);
        rock->pos.y = y;
        rock->pos.z = z - (cos * LEG_B_Z);
    }
    task->step = arch_rock_step;
    task->draw = arch_rock_draw;
    task->destructor = arch_rock_destroy;
}

/* once either big rock's work mode leaves 0, as it starts to topple, wait
 * out FALL_WAIT. then stop drawing the top, and start its pieces as children.
 * each piece starts at its part's pos from the top, not turned by the arch's
 * heading */
static void arch_rock_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *piece;
    struct task_work *rock;
    NJS_OBJECT **next;
    NJS_OBJECT *part;
    Float x;
    Float y;
    Float z;
    Float *at;
    Angle ang;
    int i;

    if (placement_try_leave_by(task, LEAVE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case ARCH_ROCK_STAND:
        for (child = task->child; child != NULL; child = child->next) {
            rock = child->work;
            if (rock->mode) {
                work->timer = 0;
                work->mode = ARCH_ROCK_BREAK;
            }
        }
        arch_rock_draw(task);
        break;
    case ARCH_ROCK_BREAK:
        ++TASK_TIMER(work);
        if (TASK_TIMER(work) > FALL_WAIT) {
            x = work->pos.x;
            y = work->pos.y;
            z = work->pos.z;
            ang = work->ang.y;
            next = &arch_rock_top_parts[1];
            for (i = 0; i < TOP_PIECES; ++i) {
                child =
                    task_create_child(TASK_WORK, arch_rock_piece_start, task);
                piece = child->work;
                part = *next++;
                PIECE(piece)->part = part;
                piece->ang.x = 0;
                piece->ang.y = ang;
                piece->ang.z = 0;
                /* @hack spelling: the part's pos read through a pointer, which
                 * loads it with FMOV.S @R4+ from 0x0c90b404; without it, 94
                 * of 191 words differ */
                at = part->pos;
                piece->pos.x = x + *at++;
                piece->pos.y = y + *at++ + TOP_Y;
                piece->pos.z = z + *at;
                piece->scl.x = RANDOM(-0.5F, 0.5F) * PIECE_SPREAD;
                piece->scl.y = rand() * (PIECE_LIFT * RAND_SCALE);
                piece->scl.z = RANDOM(-0.5F, 0.5F) * PIECE_SPREAD;
            }
            work->mode = ARCH_ROCK_FALLEN;
            task->draw = NULL;
        }
        arch_rock_draw(task);
        break;
    case ARCH_ROCK_FALLEN:
        break;
    }
    if (!placement_editing(task)) {
        collision_register(work);
    }
    task_step_children(task);
}

/* draw the top. in the set editor, where arch_rock_init starts no big
 * rocks, draw both of them whole too */
static void arch_rock_draw(struct task *task)
{
    /* @hack spelling: arch, a copy of task, which SHC keeps in R13 across the
     * calls before placement_editing (MOV R4,R13 at 0x0c90b4c8); without it,
     * 34 of 86 words differ */
    struct task *arch = task;
    struct task_work *work = arch->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (placement_editing(arch)) {
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, LEG_A_Z);
        draw_tree(*arch_rock_parts[PART_LEG_A]);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, -LEG_B_Z);
        draw_tree(*arch_rock_parts[PART_LEG_B]);
        njPopMatrix(1);
    }
    njTranslate(NULL, 0.0F, TOP_Y, 0.0F);
    draw_tree(*arch_rock_parts[PART_TOP]);
    njPopMatrix(1);
}

static void arch_rock_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void arch_rock_piece_start(struct task *task)
{
    struct task_work *work = task->work;

    work->timer = 0;
    work->mode = PIECE_FALL;
    task->step = arch_rock_piece_step;
    task->draw = arch_rock_piece_draw;
    task->destructor = arch_rock_piece_destroy;
}

/* move a piece. it falls, tumbling, until it drops below SEA_LEVEL. then it
 * sinks PIECE_GRAVITY a step until the arch's destructor destroys it */
static void arch_rock_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the position and speed read and written a float at a time
     * through pointers, which load them with FMOV.S @R6+ and @R5+ from
     * 0x0c90b60e; without it, 33 of 62 words differ */
    Float *pos = VECTOR_FLOATS(&work->pos);
    Float *speed = VECTOR_FLOATS(&work->scl);
    Float x = *pos++;
    Float speed_x = *speed++;
    Float y = *pos++;
    Float speed_y = *speed++;
    Float z = *pos++;
    Float speed_z = *speed++;

    pos -= 3;
    speed -= 3;
    switch (work->mode) {
    case PIECE_FALL:
        x += speed_x;
        y += speed_y;
        z += speed_z;
        speed_x *= FALL_KEEP;
        speed_y -= PIECE_GRAVITY;
        work->ang.x += TUMBLE_X;
        work->ang.z += TUMBLE_Z;
        speed_z *= FALL_KEEP;
        if (y < SEA_LEVEL) {
            work->mode = PIECE_SINK;
        }
        break;
    case PIECE_SINK:
        y -= PIECE_GRAVITY;
        x += speed_x;
        z += speed_z;
        speed_x *= SINK_KEEP;
        speed_z *= SINK_KEEP;
        break;
    }
    *pos++ = x;
    *pos++ = y;
    *pos = z;
    *speed++ = speed_x;
    *speed++ = speed_y;
    *speed = speed_z;
    arch_rock_piece_draw(task);
}

/* draw the piece's model at its position, turned about x and then y. a
 * falling piece's ang.z turns each step too, but the draw leaves it out */
static void arch_rock_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(PIECE(work)->part->model);
    njPopMatrix(1);
}

static void arch_rock_piece_destroy(struct task *task)
{
}
