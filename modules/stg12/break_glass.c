/* O BREAK GLASS, a pane Zero dashes through once switch 18 turns on. the pane
 * breaks into pieces and chips, and leaves nothing behind */
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
#include "stg12/break_glass.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the pane as drawn, whose root model is also its collision model */
extern NJS_OBJECT break_glass_object;
extern NJS_OBJECT break_glass_pieces_object; /* its children are the pieces */

#define SE_BREAK_GLASS 309

#define SWITCH_BREAK_GLASS 18

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* the collision world tests the pane only while a player is within 148
 * units of the task */
#define NEAR 148.0F

/* the landing a piece or chip stops on, as work->tick counts them. it bounces
 * on each landing before */
#define LANDINGS 4
/* the chips thrown beside each piece */
#define CHIPS 3
/* the puffs in the ring of dust */
#define PUFFS 16

enum break_glass_mode { GLASS_INIT, GLASS_WAIT, GLASS_BREAK, GLASS_BROKEN };
/* no piece takes mode 2 */
enum break_glass_piece_mode { PIECE_INIT = 0, PIECE_FLY = 1, PIECE_DONE = 3 };
enum break_glass_chip_mode { CHIP_INIT, CHIP_FLY, CHIP_DONE };

/* init clears unk_00 and unk_01, and nothing reads them */
struct break_glass_vars {
    Uint8 unk_00;
    Uint8 unk_01;
    Uint8 unk_02[6];
    NJS_OBJECT *pane; /* the pane's object in the collision world */
};

struct break_glass_piece_vars {
    NJS_MODEL *model;
};

#define VARS(work) ((struct break_glass_vars *)&(work)->vars)
#define PIECE(work) ((struct break_glass_piece_vars *)&(work)->vars)

static void break_glass_init(struct task *task);
static void break_glass_update(struct task *task);
static void break_glass_draw(struct task *task);
static void break_glass_destroy(struct task *task);
static void break_glass_piece_draw(struct task *task);
static void break_glass_piece_step(struct task *task);
static void break_glass_break(struct task *task);
static void break_glass_chip_step(struct task *task);
static void break_glass_chip_draw(struct task *task);
static void break_glass_dust(struct task_work *work);
static void break_glass_send_zero(struct task_work *work);
static void break_glass_wait_switch(struct task_work *work);

/* no code reads this shape, but the image holds it */
static struct collision_shape break_glass_shape = {
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
static struct dust_params break_glass_puff = {
    1.0F,
    0.038F,
    0.05F,
    0.03F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

void break_glass_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case GLASS_INIT:
            break_glass_init(task);
            break;
        case GLASS_WAIT:
            break_glass_draw(task);
            break_glass_update(task);
            break_glass_wait_switch(work);
            break;
        case GLASS_BREAK:
            break_glass_draw(task);
            task->draw = NULL;
            break_glass_send_zero(work);
            break_glass_break(task);
            break_glass_dust(work);
            work->mode = GLASS_BROKEN;
            work->flags &= ~WORLD_TESTED;
            break;
        case GLASS_BROKEN:
            break;
        /* @hack redundant: an empty default, which takes a BRA of its own past
         * the switch, at 0x0c90b29a; without it, 63 of 98 words differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case GLASS_INIT:
            break_glass_init(task);
            break;
        case GLASS_WAIT:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            break_glass_draw(task);
            break_glass_update(task);
            break;
        }
    }
    task_step_children(task);
}

static void break_glass_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *pane;

    task->draw = break_glass_draw;
    task->destructor = break_glass_destroy;
    VARS(work)->pane = pane = world_object_alloc();
    pane->evalflags = break_glass_object.evalflags;
    pane->scl[0] = 1.0F;
    pane->scl[1] = 1.0F;
    pane->scl[2] = 1.0F;
    pane->ang[0] = 0;
    pane->ang[1] = work->ang.y;
    pane->ang[2] = 0;
    pane->pos[0] = work->pos.x + break_glass_object.pos[0];
    pane->pos[1] = work->pos.y + break_glass_object.pos[1];
    pane->pos[2] = work->pos.z + break_glass_object.pos[2];
    pane->model = break_glass_object.model;
    pane->child = NULL;
    pane->sibling = NULL;
    world_add(WORLD_LARGE, task, pane);
    VARS(work)->unk_00 = 0;
    VARS(work)->unk_01 = 0;
    work->mode = GLASS_WAIT;
}

static void break_glass_update(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *pane;

    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    pane = VARS(work)->pane;
    pane->ang[0] = 0;
    pane->ang[1] = work->ang.y;
    pane->ang[2] = 0;
    pane->pos[0] = work->pos.x + break_glass_object.pos[0];
    pane->pos[1] = work->pos.y + break_glass_object.pos[1];
    pane->pos[2] = work->pos.z + break_glass_object.pos[2];
    pane->model = break_glass_object.model;
    pane->sibling = pane->child = NULL;
}

static void break_glass_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter0_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&break_glass_object);
        njPopMatrix(1);
    }
}

static void break_glass_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->pane);
    world_object_free(VARS(work)->pane);
}

static void break_glass_piece_draw(struct task *task)
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
static void break_glass_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case PIECE_INIT:
        debris_set_body(task, NULL);
        task->draw = break_glass_piece_draw;
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
                    break_glass_puff.pos = work->pos;
                    break_glass_puff.speed.x = RANDOM(-0.5F, 0.5F);
                    break_glass_puff.speed.y = 0.2F;
                    break_glass_puff.speed.z = RANDOM(-0.5F, 0.5F);
                    break_glass_puff.scale = 3.0F;
                    final_puff_create(&break_glass_puff);
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
    break_glass_piece_draw(task);
}

/* throw a piece for each child of the pieces tree, and CHIPS chips beside
 * each. a chip starts up to 5 units from its piece along each axis. all fly
 * out from the pane's origin and toward its +z, the way Zero dashes */
static void break_glass_break(struct task *task)
{
    /* @hack spelling: a copy of task, through which the function reads the work
     * and which every piece and chip takes as its parent. with it, SHC loads
     * each piece's x and y into FR12 and FR13 at 0x0c90b7c8; without it, the
     * two swap and 10 of 298 words differ */
    struct task *parent = task;
    struct task_work *work;
    NJS_OBJECT *root = &break_glass_pieces_object;
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
                TASK_WORK | TASK_MOTION, break_glass_piece_step, parent);
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
             * floats by FMOV.S @R4+ at 0x0c90b7c8; read as node->pos[0], [1]
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
            z += RANDOM(1.0F, 3.5F);
            /* @hack spelling: a cursor down from the end of motion->speed,
             * which stores z, y and x by FMOV.S @-R4 at 0x0c90b802; stored to
             * the members in that order, 218 of 298 words differ */
            point = &motion->speed.x + 3;
            *--point = z;
            *--point = y;
            *--point = x;
            /* @hack type: 0 rather than NULL, which keeps side in R8, as MOVT
             * R8 at 0x0c90b7bc shows; with NULL, side lives on the stack and
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
                TASK_WORK | TASK_MOTION, break_glass_chip_step, parent);
            struct task_work *chip = child->work;
            struct debris_motion *motion =
                (struct debris_motion *)child->motion;
            Float *point;
            Float x;
            Float y;
            Float z;
            Float scale;

            /* @hack redundant: a store of 0 to motion->spin.z that never runs.
             * it keeps 0 in R8 and the task at @R15, at 0x0c90b75c and
             * 0x0c90b75e. without it, the task takes R8, the function runs 4
             * bytes long and 258 of 298 words differ */
            if (0) {
                motion->spin.z = 0;
            }
            debris_pick_chip(chip, node->model);
            /* @hack spelling: a cursor through node->pos, which reads its
             * floats by FMOV.S @R4+ at 0x0c90b860; read as node->pos[0], [1]
             * and [2], 151 of 298 words differ */
            point = node->pos;
            x = *point++;
            y = *point++;
            z = *point;
            x += RANDOM(-0.5F, 0.5F) * 10.0F;
            /* @hack redundant: a copy of y to scale that never runs. it keeps
             * the order of the loads and the random offsets from 0x0c90b862
             * to 0x0c90b898. without it, they reorder and 14 of 298 words
             * differ */
            if (0) {
                scale = y;
            }
            y += RANDOM(-0.5F, 0.5F) * 10.0F;
            z += RANDOM(-0.5F, 0.5F) * 10.0F;
            /* @hack spelling: a cursor down from the end of chip->pos, which
             * stores z, y and x by FMOV.S @-R4 at 0x0c90b8ae; stored to the
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
            z += RANDOM(2.0F, 4.0F);
            /* @hack spelling: a cursor down from the end of motion->speed,
             * which stores z, y and x by FMOV.S @-R4 at 0x0c90b944; stored to
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
static void break_glass_chip_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case CHIP_INIT:
        debris_set_body(task, NULL);
        task->draw = break_glass_chip_draw;
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
    break_glass_chip_draw(task);
}

/* draw a chip at its scale in work->scl.x, which break_glass_break sets
 * from 0.5 to 1.5 */
static void break_glass_chip_draw(struct task *task)
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

/* raise a ring of PUFFS puffs about the pane's x axis, 10 to 30 units out.
 * each moves out from the axis and along +x */
static void break_glass_dust(struct task_work *work)
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
        Float *to = &break_glass_puff.speed.x;
        Float speed = RANDOM(0.5F, 1.3F);
        Float x = 0.8F + (rand() * (4.0F * RAND_SCALE));
        Float y = sine * speed;
        Float z = cosine * speed;

        /* @hack spelling: a cursor down from the end of break_glass_puff.speed,
         * which stores z, y and x from 0x0c90bc76 on; stored to the members in
         * that order, 435 of 492 words differ */
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        to = &break_glass_puff.pos.x;
        speed = RANDOM(10.0F, 30.0F);
        x = 0.0F;
        y = sine * speed;
        z = cosine * speed;
        /* @hack spelling: a cursor down from the end of break_glass_puff.pos,
         * which stores z, y and x from 0x0c90bcb4 on; stored to the members in
         * that order, 401 of 492 words differ */
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        break_glass_puff.scale = RANDOM(6.0F, 9.0F);
        njCalcPoint(NULL, &break_glass_puff.pos, &break_glass_puff.pos);
        njCalcVector(NULL, &break_glass_puff.speed, &break_glass_puff.speed);
        final_puff_create(&break_glass_puff);
    }
    njPopMatrix(1);
}

/* Zero dashes through the pane, from 10 units behind it toward a point 40 units
 * in front of it and 1 up */
static void break_glass_send_zero(struct task_work *work)
{
    struct zero_start start;
    Angle ang;

    start.how = ZERO_DASH;
    start.pos.z = -10.0F;
    start.pos.y = 0.0F;
    start.pos.x = 0.0F;
    start.target.z = 40.0F;
    start.target.y = 1.0F;
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

static void break_glass_wait_switch(struct task_work *work)
{
    if (switch_is_on(SWITCH_BREAK_GLASS)) {
        sound_play(SE_BREAK_GLASS, 0, 0, 0);
        work->mode = GLASS_BREAK;
    }
}
