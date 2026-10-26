/* O BREAK KABE, a wall Zero bursts out of once switch 19 turns on. the wall
 * breaks into pieces and chips, and leaves its frame */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "enemy/zero.h"
#include "enemy/zero_handle.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg10/debris.h"
#include "stg10/puff.h"
#include "stg12/break_wall.h"
#include "stg12/break_wall_object.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT break_wall_object;
extern NJS_OBJECT break_wall_broken_object; /* the frame the break leaves */
extern NJS_OBJECT break_wall_pieces_object; /* its children are the pieces */
/* the wall as the collision world holds it */
extern NJS_OBJECT break_wall_world_object;

#define SE_BREAK_WALL 329

#define SWITCH_BREAK_WALL 19

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* the collision world tests the wall only while a player is within 148
 * units of the task */
#define NEAR 148.0F

/* the landing a piece or chip stops on, as work->tick counts them. it bounces
 * on each landing before */
#define LANDINGS 4
/* the chips thrown beside each piece */
#define CHIPS 3
/* the puffs in the ring of dust */
#define PUFFS 16

enum break_wall_mode { WALL_INIT, WALL_WAIT, WALL_BREAK, WALL_BROKEN };
/* no piece takes mode 2 */
enum break_wall_piece_mode { PIECE_INIT = 0, PIECE_FLY = 1, PIECE_DONE = 3 };
enum break_wall_chip_mode { CHIP_INIT, CHIP_FLY, CHIP_DONE };

/* init clears unk_00 and unk_01, and nothing reads them */
struct break_wall_vars {
    Uint8 unk_00;
    Uint8 unk_01;
    Uint8 unk_02[6];
    /* its object in the collision world: the wall's, then the frame's */
    NJS_OBJECT *body;
};

struct break_wall_piece_vars {
    NJS_MODEL *model;
};

#define VARS(work) ((struct break_wall_vars *)&(work)->vars)
#define PIECE(work) ((struct break_wall_piece_vars *)&(work)->vars)

static void break_wall_init(struct task *task);
static void break_wall_swap_body(struct task *task);
static void break_wall_update(struct task *task);
static void break_wall_move_broken_body(struct task *task);
static void break_wall_draw(struct task *task);
static void break_wall_draw_broken(struct task *task);
static void break_wall_destroy(struct task *task);
static void break_wall_piece_draw(struct task *task);
static void break_wall_piece_step(struct task *task);
static void break_wall_break(struct task *task);
static void break_wall_chip_step(struct task *task);
static void break_wall_chip_draw(struct task *task);
static void break_wall_dust(struct task_work *work);
static void break_wall_send_zero(struct task_work *work);
static void break_wall_wait_switch(struct task_work *work);

/* no code reads this shape, but the image holds it */
static struct collision_shape break_wall_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    17.0F,
    1.5F,
    0.0F,
    {0},
    0,
    NULL,
};

/* the params of every puff: the ring of dust's, and each piece's at its
 * first landing. a puff copies pos, speed, scale and color when it is made,
 * and reads the rest each step */
static struct dust_params break_wall_puff = {
    1.0F,
    0.038F,
    0.05F,
    0.03F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

void break_wall_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case WALL_INIT:
            break_wall_init(task);
            break;
        case WALL_WAIT:
            break_wall_draw(task);
            break_wall_update(task);
            break_wall_wait_switch(work);
            break;
        case WALL_BREAK:
            break_wall_draw_broken(task);
            task->draw = break_wall_draw_broken;
            break_wall_break(task);
            break_wall_dust(work);
            break_wall_swap_body(task);
            break_wall_send_zero(work);
            work->mode = WALL_BROKEN;
            break;
        case WALL_BROKEN:
            break_wall_draw_broken(task);
            break_wall_move_broken_body(task);
            break;
        }
    } else {
        switch (work->mode) {
        case WALL_INIT:
            break_wall_init(task);
            break;
        case WALL_WAIT:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            break_wall_draw(task);
            break_wall_update(task);
            break;
        }
    }
    task_step_children(task);
}

static void break_wall_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *body;
    NJS_OBJECT *from;

    task->draw = break_wall_draw;
    task->destructor = break_wall_destroy;
    VARS(work)->body = body = world_object_alloc();
    from = &break_wall_world_object;
    body->evalflags = from->evalflags;
    body->scl[0] = 1.0F;
    body->scl[1] = 1.0F;
    body->scl[2] = 1.0F;
    body->ang[0] = 0;
    body->ang[1] = work->ang.y;
    body->ang[2] = 0;
    body->pos[0] = work->pos.x + from->pos[0];
    body->pos[1] = work->pos.y + from->pos[1];
    body->pos[2] = work->pos.z + from->pos[2];
    body->model = from->model;
    body->child = NULL;
    body->sibling = NULL;
    world_add(WORLD_LARGE, task, body);
    VARS(work)->unk_00 = 0;
    VARS(work)->unk_01 = 0;
    work->mode = WALL_WAIT;
}

static void break_wall_swap_body(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *body;
    NJS_OBJECT *from;

    world_remove(task, VARS(work)->body);
    world_object_free(VARS(work)->body);
    body = world_object_alloc();
    VARS(work)->body = body;
    from = &break_wall_broken_world_object;
    body->evalflags = from->evalflags;
    body->scl[0] = 1.0F;
    body->scl[1] = 1.0F;
    body->scl[2] = 1.0F;
    body->ang[0] = 0;
    body->ang[1] = work->ang.y;
    body->ang[2] = 0;
    body->pos[0] = work->pos.x + from->pos[0];
    body->pos[1] = work->pos.y + from->pos[1];
    body->pos[2] = work->pos.z + from->pos[2];
    body->model = from->model;
    body->child = NULL;
    body->sibling = NULL;
    world_add(WORLD_LARGE, task, body);
}

static void break_wall_update(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *from;
    NJS_OBJECT *body;

    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    from = &break_wall_world_object;
    body = VARS(work)->body;
    body->ang[0] = 0;
    body->ang[1] = work->ang.y;
    body->ang[2] = 0;
    body->pos[0] = work->pos.x + from->pos[0];
    body->pos[1] = work->pos.y + from->pos[1];
    body->pos[2] = work->pos.z + from->pos[2];
    body->model = from->model;
    body->child = NULL;
    body->sibling = NULL;
}

static void break_wall_move_broken_body(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *body = VARS(work)->body;
    NJS_OBJECT *from = &break_wall_broken_world_object;

    body->ang[0] = 0;
    body->ang[1] = work->ang.y;
    body->ang[2] = 0;
    body->pos[0] = work->pos.x + from->pos[0];
    body->pos[1] = work->pos.y + from->pos[1];
    body->pos[2] = work->pos.z + from->pos[2];
    body->model = from->model;
    body->child = NULL;
    body->sibling = NULL;
}

static void break_wall_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter0_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&break_wall_object);
        njPopMatrix(1);
    }
}

static void break_wall_draw_broken(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter0_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&break_wall_broken_object);
        njPopMatrix(1);
    }
}

static void break_wall_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->body);
    world_object_free(VARS(work)->body);
}

static void break_wall_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter0_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        draw_model(PIECE(work)->model);
        njPopMatrix(1);
    }
}

/* a piece flies, spinning, and stops on its fourth landing. each landing
 * before that bounces it and keeps 0.6 of its spin. the first also raises a
 * puff */
static void break_wall_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case PIECE_INIT:
        debris_set_body(task, NULL);
        task->draw = break_wall_piece_draw;
        work->mode = PIECE_FLY;
        break;
    case PIECE_FLY:
        if (work->flags & DEBRIS_LANDED) {
            if (++work->tick == LANDINGS) {
                motion->spin.z = 0;
                work->mode = PIECE_DONE;
            } else {
                debris_bounce(work, motion);
                motion->spin.z = (Angle)(motion->spin.z * 0.6F);
                if (work->tick == 1) {
                    break_wall_puff.pos = work->pos;
                    break_wall_puff.speed.x = RANDOM(-0.5F, 0.5F);
                    break_wall_puff.speed.y = 0.2F;
                    break_wall_puff.speed.z = RANDOM(-0.5F, 0.5F);
                    break_wall_puff.scale = 3.0F;
                    final_puff_create(&break_wall_puff);
                }
            }
        }
        debris_fall(work, motion);
        debris_move(work, motion);
        work->ang.z += motion->spin.z;
        break;
    case PIECE_DONE:
        break;
    }
    debris_land(work, motion);
    break_wall_piece_draw(task);
}

/* throw a piece for each child of the pieces tree, and CHIPS chips beside
 * each. a chip starts up to 5 units from its piece along each axis. all fly
 * out from the wall's origin and toward its -z, the way Zero dashes */
static void break_wall_break(struct task *task)
{
    /* @hack spelling: a copy of task, through which the function reads the work
     * and which every piece and chip takes as its parent. with it, SHC loads
     * each piece's x and y into FR12 and FR13 at 0x0c90a8e8; without it, the
     * two swap and 10 of 298 words differ */
    struct task *parent = task;
    struct task_work *work;
    NJS_OBJECT *root = &break_wall_pieces_object;
    NJS_OBJECT *node;
    int side = 0; /* each piece's submode, 0 and 1 by turns. no code reads it */
    int count;
    Angle ang;

    work = parent->work;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    for (node = root->child; node != NULL; node = node->sibling) {
        {
            struct task *child = task_create_child(
                TASK_WORK | TASK_MOTION, break_wall_piece_step, parent);
            struct task_work *piece = child->work;
            struct debris_motion *motion =
                (struct debris_motion *)child->motion;
            Float *point;
            Float x;
            Float y;
            Float z;
            Float scale;

            piece->submode = side;
            side = !side;
            njCalcPoint(NULL, (NJS_POINT3 *)node->pos, &piece->pos);
            /* @hack spelling: a cursor through node->pos, which reads its
             * floats by FMOV.S @R4+ at 0x0c90a8e8; read as node->pos[0], [1]
             * and [2], 227 of 298 words differ */
            point = node->pos;
            x = *point++;
            y = *point++;
            z = *point;
            scale = SQUARE(x) + SQUARE(y) + SQUARE(z);
            scale = _builtin_fsrra(scale);
            x *= scale;
            y *= scale;
            z *= scale;
            z -= RANDOM(1.0F, 3.5F);
            /* @hack spelling: a cursor down from the end of motion->speed,
             * which stores z, y and x by FMOV.S @-R4 at 0x0c90a922; stored to
             * the members in that order, 218 of 298 words differ */
            point = &motion->speed.x + 3;
            *--point = z;
            *--point = y;
            *--point = x;
            /* @hack type: 0 rather than NULL, which keeps side in R8, as MOVT
             * R8 at 0x0c90a8dc shows; with NULL, side lives on the stack and
             * 281 of 298 words differ */
            njCalcVector(0, &motion->speed, &motion->speed);
            PIECE(piece)->model = node->model;
            piece->ang.y = work->ang.y;
            piece->ang.z = 0;
            motion->spin.z = (Angle)(rand() * (1024.0F * RAND_SCALE)) + 1024;
            motion->ground = work->pos.y + 3.0F;
            motion->weight = 1.0F;
        }
        for (count = CHIPS; count > 0; count--) {
            struct task *child = task_create_child(
                TASK_WORK | TASK_MOTION, break_wall_chip_step, parent);
            struct task_work *chip = child->work;
            struct debris_motion *motion =
                (struct debris_motion *)child->motion;
            Float *point;
            Float x;
            Float y;
            Float z;
            Float scale;

            /* @hack redundant: a store of 0 to motion->spin.z that never runs.
             * it keeps 0 in R8 and the task at @R15, at 0x0c90a87c and
             * 0x0c90a87e. without it, the task takes R8, the function runs 4
             * bytes long and 258 of 298 words differ */
            if (0) {
                motion->spin.z = 0;
            }
            debris_pick_chip(chip, node->model);
            /* @hack spelling: a cursor through node->pos, which reads its
             * floats by FMOV.S @R4+ at 0x0c90a980; read as node->pos[0], [1]
             * and [2], 151 of 298 words differ */
            point = node->pos;
            x = *point++;
            y = *point++;
            z = *point;
            x += RANDOM(-0.5F, 0.5F) * 10.0F;
            /* @hack redundant: a copy of y to scale that never runs. it keeps
             * the order of the loads and the random offsets from 0x0c90a982
             * to 0x0c90a9b8. without it, they reorder and 14 of 298 words
             * differ */
            if (0) {
                scale = y;
            }
            y += RANDOM(-0.5F, 0.5F) * 10.0F;
            z += RANDOM(-0.5F, 0.5F) * 10.0F;
            /* @hack spelling: a cursor down from the end of chip->pos, which
             * stores z, y and x by FMOV.S @-R4 at 0x0c90a9ce; stored to the
             * members in that order, 127 of 298 words differ */
            point = &chip->pos.x + 3;
            *--point = z;
            *--point = y;
            *--point = x;
            scale = SQUARE(x) + SQUARE(y) + SQUARE(z);
            scale = _builtin_fsrra(scale);
            x *= scale;
            y *= scale;
            z *= scale;
            z -= RANDOM(2.0F, 4.0F);
            /* @hack spelling: a cursor down from the end of motion->speed,
             * which stores z, y and x by FMOV.S @-R4 at 0x0c90aa64; stored to
             * the members in that order, 116 of 298 words differ */
            point = &motion->speed.x + 3;
            *--point = z;
            *--point = y;
            *--point = x;
            chip->ang.x = RANDOM_ANGLE();
            chip->ang.y = work->ang.y;
            chip->ang.z = RANDOM_ANGLE();
            chip->scl.x = RANDOM(0.5F, 1.5F);
            motion->spin.z = (Angle)(rand() * (4096.0F * RAND_SCALE)) - 2048;
            motion->ground = work->pos.y + 5.0F;
            motion->weight = 1.0F;
            njCalcPoint(NULL, &chip->pos, &chip->pos);
            njCalcVector(NULL, &motion->speed, &motion->speed);
        }
    }
    njPopMatrix(1);
}

/* a chip flies, spinning, and stops on its fourth landing. each landing
 * before that bounces it and keeps 0.6 of its spin */
static void break_wall_chip_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case CHIP_INIT:
        debris_set_body(task, NULL);
        task->draw = break_wall_chip_draw;
        work->mode = CHIP_FLY;
        break;
    case CHIP_FLY:
        if (work->flags & DEBRIS_LANDED) {
            if (++work->tick == LANDINGS) {
                motion->spin.z = 0;
                work->mode = CHIP_DONE;
                break;
            }
            debris_bounce(work, motion);
            motion->spin.z = (Angle)(motion->spin.z * 0.6F);
        }
        debris_fall(work, motion);
        debris_move(work, motion);
        work->ang.z += motion->spin.z;
        break;
    case CHIP_DONE:
        break;
    }
    debris_land(work, motion);
    break_wall_chip_draw(task);
}

/* draw a chip at its scale in work->scl.x, which break_wall_break sets
 * from 0.5 to 1.5 */
static void break_wall_chip_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    if (frame_step == 0) {
        njSetTexture(&hotshelter0_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        scale = work->scl.x;
        njScale(NULL, scale, scale, scale);
        debris_draw_chip(work);
        njPopMatrix(1);
    }
}

/* raise a ring of PUFFS puffs about the wall's x axis, 10 to 30 units out.
 * each moves out from the axis and along +x */
static void break_wall_dust(struct task_work *work)
{
    Angle ang;
    int heading;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    for (heading = 0; heading < 0x10000; heading += 0x10000 / PUFFS) {
        Float sine = njSin(heading);
        Float cosine = njCos(heading);
        Float *to = &break_wall_puff.speed.x;
        Float speed = RANDOM(0.5F, 1.3F);
        Float x = 0.8F + (rand() * (4.0F * RAND_SCALE));
        Float y = sine * speed;
        Float z = cosine * speed;

        /* @hack spelling: a cursor down from the end of break_wall_puff.speed,
         * which stores z, y and x from 0x0c90ad96 on; stored to the members in
         * that order, 435 of 492 words differ */
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        to = &break_wall_puff.pos.x;
        speed = RANDOM(10.0F, 30.0F);
        x = 0.0F;
        y = sine * speed;
        z = cosine * speed;
        /* @hack spelling: a cursor down from the end of break_wall_puff.pos,
         * which stores z, y and x from 0x0c90add4 on; stored to the members in
         * that order, 401 of 492 words differ */
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        break_wall_puff.scale = RANDOM(6.0F, 9.0F);
        njCalcPoint(NULL, &break_wall_puff.pos, &break_wall_puff.pos);
        njCalcVector(NULL, &break_wall_puff.speed, &break_wall_puff.speed);
        final_puff_create(&break_wall_puff);
    }
    njPopMatrix(1);
}

/* Zero dashes out of the wall, from 10 units above the wall's origin. it heads
 * for a point 20 units along the wall's -z, at that height */
static void break_wall_send_zero(struct task_work *work)
{
    struct zero_start start;
    Angle ang;

    start.how = ZERO_DASH;
    start.pos.z = 0.0F;
    start.pos.y = 10.0F;
    start.pos.x = 0.0F;
    start.target.z = -20.0F;
    start.target.y = 10.0F;
    start.target.x = 0.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &start.pos, &start.pos);
    njCalcPoint(NULL, &start.target, &start.target);
    zero_appear(&start);
    njPopMatrix(1);
}

static void break_wall_wait_switch(struct task_work *work)
{
    if (switch_is_on(SWITCH_BREAK_WALL)) {
        work->mode = WALL_BREAK;
        sound_play(SE_BREAK_WALL, 0, 0, 0);
    }
}
