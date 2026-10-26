/* O HIYOKOYUKA: a floor that breaks into pieces when an attack hurts it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg12/hiyoko_floor.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT hiyoko_floor_0_object;
extern NJS_OBJECT hiyoko_floor_1_object;
extern NJS_OBJECT hiyoko_floor_0_pieces_object;
extern NJS_OBJECT hiyoko_floor_1_pieces_object;

#define SE_BREAK_FLOOR 310

#define FLOOR_WORLD (WORLD_LARGE | WORLD_SOLID)

/* destroyed beyond 1000 units of the camera and player 0 */
#define RANGE (1000.0F * 1000.0F)

/* the collision world tests the floor only while player 0 or 1 is within
 * NEAR of it */
#define NEAR 200.0F

/* the kinds of floor, which a set entry's scl.x picks. each kind's pieces
 * tree has PIECES children, one for each piece it breaks into */
#define FLOORS 2
#define PIECES 13

/* the steps the floor runs its pieces after the break */
#define BROKEN_STEPS 400

/* a piece drops to DROP_DEPTH below the floor's position, then sinks */
#define DROP_DEPTH 15.0F

/* the steps a piece sinks before it fades */
#define SINK_STEPS 100

enum hiyoko_floor_mode { FLOOR_WHOLE = 0, FLOOR_BROKEN = 1, FLOOR_GONE = 5 };
enum hiyoko_floor_piece_mode {
    PIECE_WAIT = 0,
    PIECE_DROP = 2,
    PIECE_SINK = 3,
    PIECE_FADE = 4,
    PIECE_GONE = 5
};

/* work->submode is the floor's kind, and TASK_TIMER(work) counts the steps
 * since the break */
struct hiyoko_floor_vars {
    NJS_OBJECT *world; /* in the collision world, or null in the set editor */
};

/* TASK_TIMER(work) counts down the steps a piece waits to drop, then up the
 * steps it has sunk */
struct hiyoko_floor_piece_vars {
    Float floor; /* the height at which it stops dropping and starts to sink */
    NJS_OBJECT *node; /* its node in the pieces tree */
    Float alpha; /* the constant material's alpha, from 0 to -1 as it fades */
};

struct hiyoko_floor_piece_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    Angle pitch_spin;
    Angle yaw_spin;
};

#define VARS(work) ((struct hiyoko_floor_vars *)&(work)->vars)
#define PIECE(work) ((struct hiyoko_floor_piece_vars *)&(work)->vars)

static void hiyoko_floor_run(struct task *task);
static void hiyoko_floor_draw(struct task *task);
static void hiyoko_floor_destroy(struct task *task);
static void hiyoko_floor_piece_start(struct task *task);
static void hiyoko_floor_piece_step(struct task *task);
static void hiyoko_floor_piece_draw(struct task *task);
static void hiyoko_floor_piece_destroy(struct task *task);

static struct collision_shape hiyoko_floor_0_shape = {
    SHAPE_RECTANGLE | SHAPE_HURTABLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {73.25F, 0.0F, -7.5F},
    73.25F,
    5.0F,
    85.5F,
    {0},
    0,
    NULL,
};
static struct collision_shape hiyoko_floor_1_shapes[2] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     0,
     PUSH_FIRM(PUSH_PLAYERS),
     {21.5F, 0.0F, -2.0F},
     45.5F,
     5.0F,
     91.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {-54.0F, 0.0F, -2.0F},
     30.0F,
     5.0F,
     50.0F,
     {0},
     0,
     NULL},
};

static NJS_OBJECT *hiyoko_floor_objects[FLOORS] = {
    &hiyoko_floor_0_object,
    &hiyoko_floor_1_object,
};
static NJS_OBJECT *hiyoko_floor_pieces[FLOORS] = {
    &hiyoko_floor_0_pieces_object,
    &hiyoko_floor_1_pieces_object,
};

void hiyoko_floor_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    Uint8 kind;
    NJS_OBJECT *world;

    work->mode = FLOOR_WHOLE;
    work->submode = kind = (Uint8)(int)njAbs(work->scl.x) % FLOORS;
    work->timer = 0;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        VARS(work)->world = world = world_object_alloc();
        world->evalflags =
            NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
        world->scl[0] = 1.0F;
        world->scl[1] = 1.0F;
        world->scl[2] = 1.0F;
        world->ang[0] = 0;
        world->ang[1] = 0;
        world->ang[2] = 0;
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = hiyoko_floor_objects[kind]->model;
        world->child = NULL;
        world->sibling = NULL;
        world_add(FLOOR_WORLD, task, world);
        work->flags |= WORLD_TESTED;
        if (kind == 0) {
            collision_init(task, &hiyoko_floor_0_shape, COLLISION_ENEMY, 1, 1);
        } else {
            collision_init(task, hiyoko_floor_1_shapes, COLLISION_ENEMY, 1, 2);
        }
    }
    task->step = hiyoko_floor_run;
    task->draw = hiyoko_floor_draw;
    task->destructor = hiyoko_floor_destroy;
}

static void hiyoko_floor_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        work->submode = (Uint8)(int)njAbs(work->scl.x) % FLOORS;
        hiyoko_floor_draw(task);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> KIND OF YUKA");
        return;
    }
    switch (work->mode) {
    case FLOOR_WHOLE:
        if ((work->flags & COLLISION_DAMAGED) &&
            work->collision->hit->kind != COLLISION_PLAYER) {
            NJS_OBJECT *node;
            Float x;
            Float y;
            Float z;
            Float floor;
            int i;

            work->flags &= ~COLLISION_DAMAGED;
            node = hiyoko_floor_pieces[work->submode]->child;
            x = work->pos.x;
            y = work->pos.y;
            z = work->pos.z;
            floor = y - DROP_DEPTH;
            for (i = 0; i < PIECES; i++) {
                /* @hack spelling: the node's position read through a cursor,
                 * for the post-increment loads FMOV.S @R4+,FR3 at 0x0c90d7aa
                 * and 0x0c90d7b6; through pos[0] to pos[2], the object's code
                 * is 32 bytes longer and 357 of 438 words differ */
                Float *point = node->pos;
                Float dx = *point++;
                Float dy = *point++;
                Float dz = *point;
                struct task_work *piece;
                struct task *child;

                child = task_create_child(TASK_WORK | TASK_MOTION,
                                          hiyoko_floor_piece_start, task);
                piece = child->work;
                piece->pos.x = x + dx;
                piece->pos.y = y + dy;
                piece->pos.z = z + dz;
                piece->timer = i * 2;
                PIECE(piece)->floor = floor;
                PIECE(piece)->node = node;
                node = node->sibling;
            }
            work->timer = 0;
            work->mode = FLOOR_BROKEN;
            work->flags &= ~WORLD_TESTED;
            task->draw = NULL;
            sound_play(SE_BREAK_FLOOR, 0, 0, 0);
        } else {
            if (player_in_range(&work->pos, NEAR)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            collision_register(work);
            hiyoko_floor_draw(task);
        }
        break;
    case FLOOR_BROKEN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > BROKEN_STEPS) {
            work->timer = 0;
            task_destroy_children(task);
            work->mode = FLOOR_GONE;
        } else {
            task_step_children(task);
        }
        break;
    case FLOOR_GONE:
        break;
    }
}

static void hiyoko_floor_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_tree(hiyoko_floor_objects[work->submode]);
        njPopMatrix(1);
    }
}

static void hiyoko_floor_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
    task_destroy_children(task);
}

static void hiyoko_floor_piece_start(struct task *task)
{
    struct task_work *work = task->work;
    struct hiyoko_floor_piece_motion *motion =
        (struct hiyoko_floor_piece_motion *)task->motion;

    PIECE(work)->alpha = 0.0F;
    work->mode = PIECE_WAIT;
    motion->speed.x = 0.0F;
    motion->speed.y = rand() * (2.0F * RAND_SCALE);
    motion->speed.z = 0.0F;
    motion->yaw_spin = (Angle)(RANDOM(-0.5F, 0.5F) * 1500.0F);
    motion->pitch_spin = (Angle)(RANDOM(-0.5F, 0.5F) * 1500.0F);
    task->step = hiyoko_floor_piece_step;
    task->draw = hiyoko_floor_piece_draw;
    task->destructor = hiyoko_floor_piece_destroy;
    hiyoko_floor_piece_draw(task);
}

static void hiyoko_floor_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct hiyoko_floor_piece_motion *motion =
        (struct hiyoko_floor_piece_motion *)task->motion;
    /* @hack spelling: the position and the speed read and written a Float at a
     * time through cursors, for the image's post-increment loads through R14
     * and R13 (0x0c90dbac to 0x0c90dbc2); through the members, 134 of 164
     * words differ */
    Float *speed = &motion->speed.x;
    Float *point = &work->pos.x;
    Float x = *point++;
    Float y = *point++;
    Float z = *point++;
    Float speed_x = *speed++;
    Float speed_y = *speed++;
    Float speed_z = *speed++;

    point -= 3;
    speed -= 3;
    switch (work->mode) {
    case PIECE_WAIT:
        if (TASK_TIMER(work) <= 0) {
            work->timer = 0;
            work->mode = PIECE_DROP;
        } else {
            TASK_TIMER(work)--;
        }
        hiyoko_floor_piece_draw(task);
        break;
    case PIECE_DROP:
        x += speed_x;
        y += speed_y;
        z += speed_z;
        speed_y -= 0.1F;
        work->ang.y += motion->yaw_spin;
        work->ang.x += motion->pitch_spin;
        if (y <= PIECE(work)->floor) {
            work->mode = PIECE_SINK;
            speed_y = -0.4F;
            work->timer = 0;
        }
        hiyoko_floor_piece_draw(task);
        break;
    case PIECE_SINK:
        x += speed_x;
        y += speed_y;
        z += speed_z;
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > SINK_STEPS) {
            work->timer = 0;
            work->mode = PIECE_FADE;
        }
        hiyoko_floor_piece_draw(task);
        break;
    case PIECE_FADE:
        x += speed_x;
        y += speed_y;
        z += speed_z;
        PIECE(work)->alpha -= 0.02F;
        if (PIECE(work)->alpha < -1.0F) {
            PIECE(work)->alpha = -1.0F;
            work->mode = PIECE_GONE;
        }
        hiyoko_floor_piece_draw(task);
        break;
    case PIECE_GONE:
        task->draw = NULL;
        break;
    }
    *point++ = x;
    *point++ = y;
    *point = z;
    *speed++ = speed_x;
    *speed++ = speed_y;
    *speed = speed_z;
}

static void hiyoko_floor_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_set_material(PIECE(work)->alpha, 0.0F, 0.0F, 0.0F);
        draw_model(PIECE(work)->node->model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
    }
}

static void hiyoko_floor_piece_destroy(struct task *task)
{
}
