/* O PARASOL: a beach parasol of five kinds, which an attack breaks to
 * pieces */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "stg01/parasol.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT beach_parasol_a_object;
extern NJS_OBJECT beach_parasol_a_piece_1_object;
extern NJS_OBJECT beach_parasol_a_piece_2_object;
extern NJS_OBJECT beach_parasol_a_piece_3_object;
extern NJS_OBJECT beach_parasol_a_piece_4_object;
extern NJS_OBJECT beach_parasol_a_piece_5_object;
extern NJS_OBJECT beach_parasol_a_piece_6_object;
extern NJS_OBJECT beach_parasol_a_piece_7_object;
extern NJS_OBJECT beach_parasol_b_object;
extern NJS_OBJECT beach_parasol_b_piece_1_object;
extern NJS_OBJECT beach_parasol_b_piece_2_object;
extern NJS_OBJECT beach_parasol_b_piece_3_object;
extern NJS_OBJECT beach_parasol_b_piece_4_object;
extern NJS_OBJECT beach_parasol_b_piece_5_object;
extern NJS_OBJECT beach_parasol_b_piece_6_object;
extern NJS_OBJECT beach_parasol_b_piece_7_object;
extern NJS_OBJECT beach_parasol_c_object;
extern NJS_OBJECT beach_parasol_c_piece_1_object;
extern NJS_OBJECT beach_parasol_c_piece_2_object;
extern NJS_OBJECT beach_parasol_c_piece_3_object;
extern NJS_OBJECT beach_parasol_c_piece_4_object;
extern NJS_OBJECT beach_parasol_c_piece_5_object;
extern NJS_OBJECT beach_parasol_d_object;
extern NJS_OBJECT beach_parasol_d_piece_1_object;
extern NJS_OBJECT beach_parasol_d_piece_2_object;
extern NJS_OBJECT beach_parasol_d_piece_3_object;
extern NJS_OBJECT beach_parasol_d_piece_4_object;
extern NJS_OBJECT beach_parasol_d_piece_5_object;
extern NJS_OBJECT beach_parasol_d_piece_6_object;
extern NJS_OBJECT beach_parasol_e_object;
extern NJS_OBJECT beach_parasol_e_piece_1_object;
extern NJS_OBJECT beach_parasol_e_piece_2_object;
extern NJS_OBJECT beach_parasol_e_piece_3_object;
extern NJS_OBJECT beach_parasol_e_piece_4_object;
extern NJS_OBJECT beach_parasol_e_piece_5_object;
extern NJS_OBJECT beach_parasol_e_piece_6_object;

#define PARASOL_KINDS 5
/* the pieces' task ends once its timer passes PIECE_STEPS */
#define PIECE_STEPS 300

/* a piece's fall speed gains PIECE_GRAVITY each step. once its timer passes
 * PIECE_FADE_AFTER, it loses PIECE_FADE of alpha each step */
#define PIECE_GRAVITY 0.1F
#define PIECE_FADE 0.02F
#define PIECE_FADE_AFTER 20

/* the parasol's kind, from scale x modulo PARASOL_KINDS, in work->tick */
#define KIND(work) ((work)->tick)

struct parasol_vars {
    Float unk_00; /* init clears it, and nothing reads it */
};

struct piece_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *part;
    Float fade; /* the alpha added to its material, from 0 down */
};

/* a piece's state, laid over its task's motion record. speed and the spins
 * are its move and its turns each step */
struct piece_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    Angle spin_x;
    Angle spin_y;
};

#define VARS(work) ((struct parasol_vars *)&(work)->vars)
#define PIECE(work) ((struct piece_vars *)&(work)->vars)
#define PIECE_MOTION(task) ((struct piece_motion *)(task)->motion)

enum parasol_mode { PARASOL_STAND, PARASOL_BROKEN };

enum pieces_mode { PIECES_START = 0, PIECES_RUN = 2 };

static void beach_parasol_step(struct task *task);
static void beach_parasol_draw(struct task *task);
static void beach_parasol_destroy(struct task *task);
static void beach_parasol_pieces_step(struct task *task);
static void beach_parasol_pieces_destroy(struct task *task);
static void beach_parasol_piece_start(struct task *task);
static void beach_parasol_piece_step(struct task *task);
static void beach_parasol_piece_draw(struct task *task);
static void beach_parasol_piece_destroy(struct task *task);

/* each kind's models: the whole parasol, then its pieces */
static NJS_OBJECT *beach_parasol_a_parts[8] = {
    &beach_parasol_a_object,         &beach_parasol_a_piece_1_object,
    &beach_parasol_a_piece_2_object, &beach_parasol_a_piece_3_object,
    &beach_parasol_a_piece_4_object, &beach_parasol_a_piece_5_object,
    &beach_parasol_a_piece_6_object, &beach_parasol_a_piece_7_object,
};
static NJS_OBJECT *beach_parasol_b_parts[8] = {
    &beach_parasol_b_object,         &beach_parasol_b_piece_1_object,
    &beach_parasol_b_piece_2_object, &beach_parasol_b_piece_3_object,
    &beach_parasol_b_piece_4_object, &beach_parasol_b_piece_5_object,
    &beach_parasol_b_piece_6_object, &beach_parasol_b_piece_7_object,
};
static NJS_OBJECT *beach_parasol_c_parts[6] = {
    &beach_parasol_c_object,         &beach_parasol_c_piece_1_object,
    &beach_parasol_c_piece_2_object, &beach_parasol_c_piece_3_object,
    &beach_parasol_c_piece_4_object, &beach_parasol_c_piece_5_object,
};
static NJS_OBJECT *beach_parasol_d_parts[7] = {
    &beach_parasol_d_object,         &beach_parasol_d_piece_1_object,
    &beach_parasol_d_piece_2_object, &beach_parasol_d_piece_3_object,
    &beach_parasol_d_piece_4_object, &beach_parasol_d_piece_5_object,
    &beach_parasol_d_piece_6_object,
};
static NJS_OBJECT *beach_parasol_e_parts[7] = {
    &beach_parasol_e_object,         &beach_parasol_e_piece_1_object,
    &beach_parasol_e_piece_2_object, &beach_parasol_e_piece_3_object,
    &beach_parasol_e_piece_4_object, &beach_parasol_e_piece_5_object,
    &beach_parasol_e_piece_6_object,
};

static int beach_parasol_part_counts[PARASOL_KINDS] = {8, 8, 6, 7, 7};
static NJS_OBJECT **beach_parasol_parts[PARASOL_KINDS] = {
    beach_parasol_a_parts, beach_parasol_b_parts, beach_parasol_c_parts,
    beach_parasol_d_parts, beach_parasol_e_parts,
};

/* a sphere around the canopy and a capsule along the pole, for the tall
 * parasols and for the short ones */
/* clang-format off */
static struct collision_shape beach_parasol_tall_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 15.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_CAPSULE | SHAPE_HURTABLE | LOCKON_NO_NUMBER, 0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 10.0F, 0.0F}, 2.0F, 10.0F, 0.0F,
     {0, 0, 0}, 0, NULL},
};
static struct collision_shape beach_parasol_short_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 8.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_CAPSULE | SHAPE_HURTABLE | LOCKON_NO_NUMBER, 0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 4.0F, 0.0F}, 6.0F, 4.0F, 0.0F,
     {0, 0, 0}, 0, NULL},
};
/* clang-format on */

/* each kind's shapes. a kind is below PARASOL_KINDS, so no kind reads the
 * sixth entry */
static struct collision_shape *beach_parasol_shapes[PARASOL_KINDS + 1] = {
    beach_parasol_tall_shapes,  beach_parasol_tall_shapes,
    beach_parasol_short_shapes, beach_parasol_short_shapes,
    beach_parasol_short_shapes, beach_parasol_tall_shapes,
};

/* give the parasol an enemy's record, so that E-102 can lock on to it. then
 * read its kind from scale x */
void beach_parasol_init(struct task *task)
{
    struct task_work *work = task->work;

    enemy_alloc_record(task, work);
    KIND(work) = (Uint8)work->scl.x % PARASOL_KINDS;
    work->mode = PARASOL_STAND;
    work->timer = 0;
    VARS(work)->unk_00 = 0.0F;
    if (!placement_editing(task)) {
        collision_init(task, beach_parasol_shapes[KIND(work)], COLLISION_ITEM,
                       1, 2);
    }
    task->step = beach_parasol_step;
    task->draw = beach_parasol_draw;
    task->destructor = beach_parasol_destroy;
}

/* when an attack hits the parasol, create its pieces' task where it
 * stands. the next step kills the placement. a hit while player 0 is E-102
 * makes a puff too. until a hit, register the parasol for collision checks.
 * in the set editor, label scale x as the kind, and draw the parasol
 * instead */
static void beach_parasol_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    struct task *pieces;
    struct task_work *pieces_work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        KIND(work) = (Uint8)work->scl.x % PARASOL_KINDS;
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> KIND OF PARASOL");
        beach_parasol_draw(task);
    } else {
        switch (work->mode) {
        case PARASOL_STAND:
            if (enemy_take_hit(work, LOCKON_TARGET(task))) {
                if (player_get_character(0) == CHARACTER_E102) {
                    pos.x = work->pos.x;
                    pos.y = work->pos.y + 10.0F;
                    pos.z = work->pos.z;
                    bomb_create(&pos, 0.4F);
                }
                work->mode = PARASOL_BROKEN;
                pieces = task_create(TASK_WORK | TASK_MOTION, 3,
                                     beach_parasol_pieces_step);
                pieces_work = pieces->work;
                pieces_work->mode = PIECES_START;
                KIND(pieces_work) = KIND(work);
                pieces_work->ang.y = work->ang.y;
                pieces_work->pos.x = work->pos.x;
                pieces_work->pos.y = work->pos.y;
                pieces_work->pos.z = work->pos.z;
            } else {
                collision_register(work);
            }
            beach_parasol_draw(task);
            break;
        case PARASOL_BROKEN:
            placement_kill(task);
            break;
        }
    }
}

static void beach_parasol_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(beach_parasol_parts[KIND(work)][0]);
    njPopMatrix(1);
}

static void beach_parasol_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

/* the pieces' task. its first step creates a child task for each piece. a
 * piece starts at the parasol's position plus its object's offset, which
 * isn't turned with the parasol. later steps step the pieces, and the
 * destructor destroys them */
static void beach_parasol_pieces_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    int count;
    Float x;
    Float y;
    Float z;
    Angle ang;
    struct task *child;
    struct task_work *piece;
    NJS_OBJECT *part;
    NJS_OBJECT **parts;
    NJS_OBJECT **next;

    switch (work->mode) {
    case PIECES_START:
        count = beach_parasol_part_counts[KIND(work)];
        ang = work->ang.y;
        parts = beach_parasol_parts[KIND(work)];
        x = work->pos.x;
        y = work->pos.y;
        z = work->pos.z;
        /* @hack spelling: the cursor next, which reads each part by
         * MOV.L @R10+,R5 at 0x0c903074. as parts[i], 16 of 114 words
         * differ */
        next = &parts[1];
        for (i = 1; i < count; i++) {
            child = task_create_child(TASK_WORK | TASK_MOTION,
                                      beach_parasol_piece_start, task);
            piece = child->work;
            part = *next++;
            PIECE(piece)->part = part;
            piece->ang.y = ang;
            piece->pos.x = x + part->pos[0];
            piece->pos.y = y + part->pos[1];
            piece->pos.z = z + part->pos[2];
        }
        work->timer = 0;
        task->destructor = beach_parasol_pieces_destroy;
        work->mode = PIECES_RUN;
        break;
    case PIECES_RUN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > PIECE_STEPS) {
            task_kill(task);
        }
        task_step_children(task);
        break;
    }
}

static void beach_parasol_pieces_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* a piece's first step. it sends the piece up and out at a random speed.
 * the piece spins at random about x and y */
static void beach_parasol_piece_start(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);

    work->timer = 0;
    PIECE(work)->fade = 0.0F;
    motion->speed.x = RANDOM(-0.5F, 0.5F) * 1.6F;
    motion->speed.y = RANDOM(2.0F, 3.5F);
    motion->speed.z = RANDOM(-0.5F, 0.5F) * 1.6F;
    motion->spin_y = RANDOM(-0.5F, 0.5F) * 2000.0F;
    motion->spin_x = RANDOM(-0.5F, 0.5F) * 3500.0F;
    task->step = beach_parasol_piece_step;
    task->draw = beach_parasol_piece_draw;
    task->destructor = beach_parasol_piece_destroy;
}

/* move and spin the piece, and draw it. it fades once its timer passes
 * PIECE_FADE_AFTER */
static void beach_parasol_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);

    work->pos.x += motion->speed.x;
    work->pos.y += motion->speed.y;
    work->pos.z += motion->speed.z;
    motion->speed.y -= PIECE_GRAVITY;
    work->ang.y += motion->spin_y;
    work->ang.x += motion->spin_x;
    if (TASK_TIMER(work) > PIECE_FADE_AFTER) {
        PIECE(work)->fade -= PIECE_FADE;
    } else {
        TASK_TIMER(work)++;
    }
    beach_parasol_piece_draw(task);
}

static void beach_parasol_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(PIECE(work)->fade, 0.0F, 0.0F, 0.0F);
    draw_model(PIECE(work)->part->model);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
    njPopMatrix(1);
}

static void beach_parasol_piece_destroy(struct task *task)
{
}
