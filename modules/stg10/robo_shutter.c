/* O ROBO SHUTTER shutter that bursts open for Zero to dash out through,
 * leaving its frame */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "enemy/zero.h"
#include "enemy/zero_handle.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg10/debris.h"
#include "stg10/puff.h"
#include "stg10/robo_shutter.h"
#include "stg10/zero_appear.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the shutter, shut, which O SHUTTER draws too */
extern NJS_OBJECT shutter_object;
/* the shutter bursting open, as a motion and a shape motion of its points */
extern NJS_OBJECT robo_shutter_burst_object;
extern NJS_MOTION robo_shutter_burst_motion;
extern NJS_MOTION robo_shutter_burst_shape;
extern NJS_OBJECT robo_shutter_open_object; /* the frame the burst leaves */
/* the pieces the burst throws, one for each child of its root */
extern NJS_OBJECT robo_shutter_pieces_object;

#define SE_ROBO_SHUTTER 293

/* the shutter bursts once a player or an attack touches the C APP ZERO
 * numbered 0 */
#define CUE 0

/* how far the collision record reaches from the shutter */
#define REACH 120.0F

/* the burst's frame starts at 0 and gains 1 a step. it first passes
 * BURST_LAST at 29, the last of the motion's 30. the shutter breaks on that
 * step */
#define BURST_LAST 28.99F

/* a piece or chip stops on this landing, and bounces on each before it.
 * work->tick counts the landings */
#define PIECE_LANDINGS 4
/* the share of its spin a piece or chip keeps at each bounce */
#define SPIN_KEPT 0.6F

#define CHIPS 3 /* chips thrown about each piece */

enum robo_shutter_mode {
    SHUTTER_INIT,
    SHUTTER_WAIT,
    SHUTTER_BURST,
    SHUTTER_OPEN,
    SHUTTER_DONE
};
/* a piece never takes mode 2 */
enum robo_shutter_piece_mode { PIECE_INIT = 0, PIECE_FLY = 1, PIECE_DONE = 3 };
enum robo_shutter_chip_mode { CHIP_INIT, CHIP_FLY, CHIP_DONE };

struct robo_shutter_vars {
    Uint8 unk_00[8];
    Float frame; /* the burst motion's frame */
};

struct robo_shutter_piece_vars {
    NJS_MODEL *model;
};

#define VARS(work) ((struct robo_shutter_vars *)&(work)->vars)
#define PIECE(work) ((struct robo_shutter_piece_vars *)&(work)->vars)

static void robo_shutter_destroy(struct task *task);
static void robo_shutter_draw(struct task *task);
static void robo_shutter_draw_at(struct task_work *work);
static void robo_shutter_piece_draw(struct task *task);
static void robo_shutter_piece_step(struct task *task);
static void robo_shutter_break(struct task *task);
static void robo_shutter_chip_step(struct task *task);
static void robo_shutter_chip_draw(struct task *task);
static void robo_shutter_dust(struct task_work *work);
static void robo_shutter_send_zero(struct task_work *work);

/* params for the ring of dust and for each piece's first landing. before each
 * final_puff_create, its caller writes the puff's pos, speed and scale here */
static struct dust_params robo_shutter_puff = {
    1.0F,
    0.038F,
    0.05F,
    0.03F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* a box 4 units along x, 80 high and 124 along z, its base at the shutter's
 * position. it pushes only players, and items and enemies pass through it */
static struct collision_shape robo_shutter_shape = {
    SHAPE_RECTANGLE | SHAPE_IGNORE_ENEMIES,
    0,
    PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
    {0.0F, 40.0F, 0.0F},
    2.0F,
    40.0F,
    62.0F,
    {0},
    0,
    NULL,
};

/* wait for the CUE flag, then play the burst. at its end, throw the pieces
 * with a ring of dust, and end any Zero already out. robo_shutter_send_zero's
 * zero_appear makes no Zero while one is out. on the next step, Zero dashes
 * out through the shutter. a shutter already used starts open */
void robo_shutter_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SHUTTER_INIT:
        task->destructor = robo_shutter_destroy;
        task->draw = robo_shutter_draw;
        if (!placement_editing(task)) {
            collision_init(task, &robo_shutter_shape, COLLISION_OBJECT, 1, 1);
            work->collision->radius = REACH;
        }
        if (placement_used(task)) {
            work->mode = SHUTTER_DONE;
        } else {
            work->mode = SHUTTER_WAIT;
        }
        break;
    case SHUTTER_WAIT:
        if (zero_appear_is_on(CUE)) {
            VARS(work)->frame = 0.0F;
            work->mode = SHUTTER_BURST;
        }
        break;
    case SHUTTER_BURST:
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame < BURST_LAST) {
            break;
        }
        robo_shutter_break(task);
        robo_shutter_dust(work);
        work->mode = SHUTTER_OPEN;
        zero_kill();
        break;
    case SHUTTER_OPEN:
        sound_play(SE_ROBO_SHUTTER, 0, 0, 0);
        placement_set_used(task);
        robo_shutter_send_zero(work);
        work->mode = SHUTTER_DONE;
        break;
    case SHUTTER_DONE:
        break;
    }
    task_step_children(task);
    robo_shutter_draw_at(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void robo_shutter_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void robo_shutter_draw(struct task *task)
{
    robo_shutter_draw_at(task->work);
}

static void robo_shutter_draw_at(struct task_work *work)
{
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        switch (work->mode) {
        case SHUTTER_INIT:
        case SHUTTER_WAIT:
            draw_tree(&shutter_object);
            break;
        case SHUTTER_BURST:
            draw_shape_motion(&robo_shutter_burst_object,
                              &robo_shutter_burst_motion,
                              &robo_shutter_burst_shape, VARS(work)->frame);
            break;
        case SHUTTER_OPEN:
        case SHUTTER_DONE:
            draw_tree(&robo_shutter_open_object);
            break;
        }
        njPopMatrix(1);
    }
}

static void robo_shutter_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        draw_model(PIECE(work)->model);
        njPopMatrix(1);
    }
}

/* a piece flies, spinning, and stops on landing PIECE_LANDINGS. each landing
 * before it bounces the piece and keeps SPIN_KEPT of its spin. the first also
 * raises a puff */
static void robo_shutter_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case PIECE_INIT:
        debris_set_body(task, NULL);
        task->draw = robo_shutter_piece_draw;
        work->mode = PIECE_FLY;
        break;
    case PIECE_FLY:
        if (work->flags & DEBRIS_LANDED) {
            if (++work->tick == PIECE_LANDINGS) {
                motion->spin.z = 0;
                work->mode = PIECE_DONE;
            } else {
                debris_bounce(work, motion);
                motion->spin.z = (Angle)(motion->spin.z * SPIN_KEPT);
                if (work->tick == 1) {
                    robo_shutter_puff.pos = work->pos;
                    robo_shutter_puff.speed.x = RANDOM(-0.5F, 0.5F);
                    robo_shutter_puff.speed.y = 0.2F;
                    robo_shutter_puff.speed.z = RANDOM(-0.5F, 0.5F);
                    robo_shutter_puff.scale = 3.0F;
                    final_puff_create(&robo_shutter_puff);
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
    robo_shutter_piece_draw(task);
}

/* throw a piece for each child of robo_shutter_pieces_object, and CHIPS chips
 * about it. a piece starts at its node. a chip starts up to 5 units from that
 * node along each axis. each moves away from the shutter's position and
 * forward along x */
static void robo_shutter_break(struct task *task)
{
    /* @hack spelling: parent, a copy of task, which keeps the piece's y in
     * FR12 and its z in FR14 (0x0c90ad2a, 0x0c90ad32); through task, the two
     * trade and 10 of 298 words differ */
    struct task *parent = task;
    struct task_work *work;
    NJS_OBJECT *root = &robo_shutter_pieces_object;
    NJS_OBJECT *node;
    int odd = 0;
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
                TASK_WORK | TASK_MOTION, robo_shutter_piece_step, parent);
            struct task_work *piece = child->work;
            struct debris_motion *motion =
                (struct debris_motion *)child->motion;
            Float *point;
            Float x;
            Float y;
            Float z;
            Float scale;

            piece->submode = odd;
            odd = !odd;
            njCalcPoint(NULL, (NJS_POINT3 *)node->pos, &piece->pos);
            /* @hack spelling: node->pos read a Float at a time through point,
             * for the post-increment loads at 0x0c90ad28 and 0x0c90ad2a;
             * through node->pos[0] to [2], 227 of 298 words differ */
            point = node->pos;
            x = *point++;
            y = *point++;
            z = *point;
            scale = SQUARE(x) + SQUARE(y) + SQUARE(z);
            scale = _builtin_fsrra(scale);
            x *= scale;
            y *= scale;
            z *= scale;
            x += RANDOM(1.0F, 3.5F);
            /* @hack spelling: the speed written from z down to x through point,
             * for the pre-decrement stores at 0x0c90ad54 to 0x0c90ad66; through
             * the members, 222 of 298 words differ */
            point = &motion->speed.x + 3;
            *--point = z;
            *--point = y;
            *--point = x;
            /* @hack type: the null matrix as 0, an int. SHC then keeps odd in
             * R8 (MOV #0,R8 at 0x0c90acbc). as NULL, odd takes R9 and a frame
             * word, the function runs 4 bytes long and 281 of 298 words
             * differ */
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
                TASK_WORK | TASK_MOTION, robo_shutter_chip_step, parent);
            struct task_work *chip = child->work;
            struct debris_motion *motion =
                (struct debris_motion *)child->motion;
            Float *point;
            Float x;
            Float y;
            Float z;
            Float scale;

            /* @hack redundant: a store of 0 to motion->spin.z that never runs.
             * SHC then keeps odd in R8 and the task in the frame at @R15
             * (0x0c90acbc, 0x0c90acbe). without it, the task takes R8, the
             * function runs 4 bytes long and 255 of 298 words differ */
            if (0) {
                motion->spin.z = 0;
            }
            debris_pick_chip(chip, node->model);
            /* @hack spelling: node->pos read a Float at a time through point,
             * as for the piece (0x0c90adc0, 0x0c90adc2); through node->pos[0]
             * to [2], 151 of 298 words differ */
            point = node->pos;
            x = *point++;
            y = *point++;
            z = *point;
            x += RANDOM(-0.5F, 0.5F) * 10.0F;
            /* @hack redundant: a copy of y to scale that never runs, which
             * keeps RAND_SCALE in FR13 and the piece's z in FR14 (0x0c90acf8,
             * 0x0c90ad32); without it, the two trade and 17 of 298 words
             * differ */
            if (0) {
                scale = y;
            }
            y += RANDOM(-0.5F, 0.5F) * 10.0F;
            z += RANDOM(-0.5F, 0.5F) * 10.0F;
            /* @hack spelling: the chip's pos written from z down to x through
             * point (0x0c90ae0e to 0x0c90ae12); through the members, 135 of
             * 298 words differ */
            point = &chip->pos.x + 3;
            *--point = z;
            *--point = y;
            *--point = x;
            scale = SQUARE(x) + SQUARE(y) + SQUARE(z);
            scale = _builtin_fsrra(scale);
            x *= scale;
            y *= scale;
            z *= scale;
            x += RANDOM(2.0F, 4.0F);
            /* @hack spelling: the chip's speed written from z down to x through
             * point (0x0c90aea0 to 0x0c90aeaa); through the members, 113 of
             * 298 words differ */
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

/* a chip flies as a piece does, but raises no puff. unlike a piece, it doesn't
 * move on the step it stops */
static void robo_shutter_chip_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case CHIP_INIT:
        debris_set_body(task, NULL);
        task->draw = robo_shutter_chip_draw;
        work->mode = CHIP_FLY;
        break;
    case CHIP_FLY:
        if (work->flags & DEBRIS_LANDED) {
            if (++work->tick == PIECE_LANDINGS) {
                motion->spin.z = 0;
                work->mode = CHIP_DONE;
                break;
            }
            debris_bounce(work, motion);
            motion->spin.z = (Angle)(motion->spin.z * SPIN_KEPT);
        }
        debris_fall(work, motion);
        debris_move(work, motion);
        work->ang.z += motion->spin.z;
        break;
    case CHIP_DONE:
        break;
    }
    debris_land(work, motion);
    robo_shutter_chip_draw(task);
}

static void robo_shutter_chip_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    if (frame_step == 0) {
        texture_set_stage();
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

/* raise a ring of 16 puffs, one each sixteenth of a turn about the shutter's
 * x axis. each starts 10 to 30 units out. it moves outward at 0.5 to 1.3
 * units a step, and along x at 0.8 to 4.8 */
static void robo_shutter_dust(struct task_work *work)
{
    Angle ang;
    int heading;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    for (heading = 0; heading < 0x10000; heading += 0x1000) {
        Float sine = njSin(heading);
        Float cosine = njCos(heading);
        Float *to = &robo_shutter_puff.speed.x;
        Float speed = RANDOM(0.5F, 1.3F);
        Float x = 0.8F + (rand() * (4.0F * RAND_SCALE));
        Float y = sine * speed;
        Float z = cosine * speed;

        /* @hack spelling: the speed written from z down to x through to, which
         * SHC keeps in the frame at @R15 (0x0c90b1d2 to 0x0c90b1e2); through
         * the members, 435 of 492 words differ */
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        to = &robo_shutter_puff.pos.x;
        speed = RANDOM(10.0F, 30.0F);
        x = 0.0F;
        y = sine * speed;
        z = cosine * speed;
        /* @hack spelling: the pos written through to as the speed is
         * (0x0c90b210 to 0x0c90b222); through the members, 401 of 492 words
         * differ */
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        robo_shutter_puff.scale = RANDOM(6.0F, 9.0F);
        njCalcPoint(NULL, &robo_shutter_puff.pos, &robo_shutter_puff.pos);
        njCalcVector(NULL, &robo_shutter_puff.speed, &robo_shutter_puff.speed);
        final_puff_create(&robo_shutter_puff);
    }
    njPopMatrix(1);
}

/* Zero dashes out through the shutter along x, from 10 units behind it toward
 * a point 40 units ahead and 1 up */
static void robo_shutter_send_zero(struct task_work *work)
{
    struct zero_start start;
    Angle ang;

    start.how = ZERO_DASH;
    start.pos.x = -10.0F;
    start.pos.y = 0.0F;
    start.pos.z = 0.0F;
    start.target.x = 40.0F;
    start.target.y = 1.0F;
    start.target.z = 0.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &start.pos, &start.pos);
    njCalcPoint(NULL, &start.target, &start.target);
    zero_appear(&start);
    njPopMatrix(1);
}
