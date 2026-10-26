/* O ROBO WALL wall that breaks for Zero to dash out through, leaving its
 * frame */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "enemy/zero.h"
#include "enemy/zero_handle.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg10/debris.h"
#include "stg10/puff.h"
#include "stg10/robo_wall.h"
#include "stg10/zero_appear.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT robo_wall_object;
/* the pieces the break throws, one for each child of its root */
extern NJS_OBJECT robo_wall_pieces_object;
extern NJS_OBJECT robo_wall_broken_object; /* the frame the break leaves */

#define SE_ROBO_WALL 292

/* in work->flags: the wall has broken, and draws robo_wall_broken_object */
#define WALL_BROKEN 0x200

/* the share of its spin a flying piece keeps each step */
#define SPIN_KEPT 0.98F

enum robo_wall_mode { WALL_INIT, WALL_WAIT, WALL_BREAK, WALL_DONE };
enum robo_wall_piece_mode { PIECE_INIT, PIECE_FLY, PIECE_SETTLE, PIECE_DONE };

struct robo_wall_piece_vars {
    Uint8 unk_00[8];
    NJS_MODEL *model;
};

#define PIECE(work) ((struct robo_wall_piece_vars *)&(work)->vars)

static void robo_wall_destroy(struct task *task);
static void robo_wall_draw(struct task *task);
static void robo_wall_draw_at(struct task_work *work);
static void robo_wall_break(struct task *task);
static void robo_wall_piece_step(struct task *task);
static void robo_wall_piece_draw(struct task *task);
static void robo_wall_dust(struct task_work *work);
static void robo_wall_send_zero(struct task_work *work);

/* a box 10 units along x, 20 high and 4 along z, its base at the wall's
 * position. it pushes only players, and items and enemies pass through it */
static struct collision_shape robo_wall_shape = {
    SHAPE_RECTANGLE | SHAPE_IGNORE_ENEMIES,
    0,
    PUSH_PLAYERS,
    {0.0F, 10.0F, 0.0F},
    5.0F,
    10.0F,
    2.0F,
    {0},
    0,
    NULL,
};

/* params for the ring of dust and for each piece's puff as it settles. before
 * each final_puff_create, its caller writes the puff's pos, speed and scale
 * here */
static struct dust_params robo_wall_puff = {
    1.0F,
    0.038F,
    0.05F,
    0.03F,
    0.01F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {2.0F, 1.0F, 1.0F, 1.0F},
};

/* wait for a player or an attack to touch the C APP ZERO numbered by
 * angle x. then throw the wall's pieces with a ring of dust, and send Zero
 * out through it. a wall already used starts broken. while the set editor is
 * open, the wall only draws */
void robo_wall_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        robo_wall_draw_at(work);
        return;
    }
    switch (work->mode) {
    case WALL_INIT:
        work->submode = work->ang.x;
        work->ang.x = 0;
        work->ang.z = 0;
        task->destructor = robo_wall_destroy;
        task->draw = robo_wall_draw;
        if (!placement_editing(task)) {
            collision_init(task, &robo_wall_shape, COLLISION_OBJECT, 1, 1);
        }
        if (placement_used(task)) {
            work->mode = WALL_BREAK;
        } else {
            work->mode = WALL_WAIT;
        }
        break;
    case WALL_WAIT:
        if (zero_appear_is_on(work->submode)) {
            work->mode = WALL_BREAK;
            placement_set_used(task);
            robo_wall_send_zero(work);
            robo_wall_break(task);
            robo_wall_dust(work);
            sound_play(SE_ROBO_WALL, 0, 0, 0);
        }
        break;
    case WALL_BREAK:
        work->flags |= WALL_BROKEN;
        if (work->timer == 0) {
            work->mode = WALL_DONE;
            break;
        }
        work->timer--;
        break;
    case WALL_DONE:
        break;
    }
    robo_wall_draw_at(work);
    task_step_children(task);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void robo_wall_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void robo_wall_draw(struct task *task)
{
    robo_wall_draw_at(task->work);
}

static void robo_wall_draw_at(struct task_work *work)
{
    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        if (!(work->flags & WALL_BROKEN)) {
            draw_tree(&robo_wall_object);
        } else {
            draw_tree(&robo_wall_broken_object);
        }
        njPopMatrix(1);
    }
}

/* throw a piece for each child of robo_wall_pieces_object, from its node. a
 * piece leaves at 1 to 3 units a step along -x. its y and z speeds are those of
 * the unit vector from the wall's center to its node. every piece lands at the
 * height of the ground 20 units out along -x */
static void robo_wall_break(struct task *task)
{
    /* @hack spelling: parent, a copy of task, which keeps work in R10 and
     * rand's address in R9 (0x0c909f9a, 0x0c909ff4); through task, the two
     * trade and 12 of 189 words differ */
    struct task *parent = task;
    NJS_OBJECT *root = &robo_wall_pieces_object;
    NJS_POINT3 center = {0.0F, 10.0F, 0.0F};
    struct task_work *work = parent->work;
    Float ground;
    NJS_OBJECT *node;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    {
        NJS_POINT3 below = {-20.0F, 0.0F, 0.0F};
        struct angles slope;

        njCalcPoint(NULL, &below, &below);
        ground = ground_below(below.x, below.y, below.z, &slope);
        for (node = root->child; node != NULL; node = node->sibling) {
            struct task *child = task_create_child(
                TASK_WORK | TASK_MOTION, robo_wall_piece_step, parent);
            struct task_work *piece = child->work;
            struct debris_motion *motion =
                (struct debris_motion *)child->motion;
            Float x;
            Float y;
            Float z;
            Float cx;
            Float cy;
            Float cz;
            Float scale;
            Float squared;
            Float *from;
            Float *to;

            piece->ang.y = work->ang.y;
            PIECE(piece)->model = node->model;
            motion->ground = ground;
            motion->weight = 1.0F;
            /* @hack spelling: node->pos read a Float at a time through from,
             * for the post-increment loads at 0x0c90a028 and 0x0c90a02a;
             * through node->pos[0] to [2], 135 of 189 words differ */
            from = node->pos;
            x = *from++;
            y = *from++;
            z = *from;
            /* @hack spelling: the piece's pos written from z down to x through
             * to, for the pre-decrement stores at 0x0c90a032 to 0x0c90a036;
             * through the members, 164 of 189 words differ */
            to = &piece->pos.x + 3;
            *--to = z;
            *--to = y;
            *--to = x;
            /* @hack spelling: center read a Float at a time through
             * VECTOR_FLOATS, for the post-increment loads at 0x0c90a03c and
             * 0x0c90a03e; from &center.x, 167 of 189 words differ, and
             * through the members, 82 */
            from = VECTOR_FLOATS(&center);
            cx = *from++;
            cy = *from++;
            cz = *from;
            z -= cz;
            y -= cy;
            x -= cx;
            squared = SQUARE(x) + SQUARE(y) + SQUARE(z);
            scale = _builtin_fsrra(squared);
            y *= scale;
            z *= scale;
            x = 1.0F + (rand() * (2.0F * RAND_SCALE));
            x = -x;
            /* @hack spelling: the speed written from z down to x through to
             * (0x0c90a06e to 0x0c90a076); through the members, 166 of 189
             * words differ */
            to = &motion->speed.x + 3;
            *--to = z;
            *--to = y;
            *--to = x;
            piece->ang.z = 256;
            if (y < 0.0F) {
                piece->ang.z = -piece->ang.z;
            }
            motion->spin.z = (Angle)(rand() * (512.0F * RAND_SCALE)) + 256;
            njCalcPoint(NULL, &piece->pos, &piece->pos);
            njCalcVector(NULL, &motion->speed, &motion->speed);
        }
    }
    njPopMatrix(1);
}

/* a piece flies, keeping SPIN_KEPT of its spin each step, until it lands. it
 * then turns about z to the nearer quarter turn, onto its side. it turns faster
 * each step, and raises a puff once there */
static void robo_wall_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;
    Angle left;
    Angle right;

    switch (work->mode) {
    case PIECE_INIT:
        debris_set_body(task, NULL);
        task->draw = robo_wall_piece_draw;
        work->mode = PIECE_FLY;
        break;
    case PIECE_FLY:
        if (work->flags & DEBRIS_LANDED) {
            work->mode = PIECE_SETTLE;
            left = angle_distance(work->ang.z, 0x4000);
            right = angle_distance(work->ang.z, 0xc000);
            motion->aim.z = left < right ? 0x4000 : 0xc000;
            VECTOR_CLEAR(&motion->speed);
        }
        motion->spin.z = (Angle)(motion->spin.z * SPIN_KEPT);
        if (work->ang.z >= 0) {
            work->ang.z += motion->spin.z;
        } else {
            work->ang.z -= motion->spin.z;
        }
        debris_fall(work, motion);
        debris_move(work, motion);
        break;
    case PIECE_SETTLE:
        motion->spin.z += 8;
        work->ang.z =
            angle_approach(work->ang.z, motion->aim.z, motion->spin.z);
        if (work->ang.z == motion->aim.z) {
            work->mode = PIECE_DONE;
            work->timer = 20;
            robo_wall_puff.pos = work->pos;
            robo_wall_puff.speed.x = RANDOM(-0.5F, 0.5F);
            robo_wall_puff.speed.y = 0.1F;
            robo_wall_puff.speed.z = RANDOM(-0.5F, 0.5F);
            robo_wall_puff.scale = 2.0F;
            final_puff_create(&robo_wall_puff);
        }
        break;
    case PIECE_DONE:
        break;
    }
    debris_land(work, motion);
    robo_wall_piece_draw(task);
}

static void robo_wall_piece_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, work->ang.z);
        draw_model(PIECE(work)->model);
        njPopMatrix(1);
    }
}

/* raise a ring of 16 puffs, one each sixteenth of a turn about the wall's x
 * axis. each starts 10 to 30 units out. it moves outward at 0.5 to 1.3 units a
 * step, and along x at -3.2 to 0.8 */
static void robo_wall_dust(struct task_work *work)
{
    int heading;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    for (heading = 0; heading < 0x10000; heading += 0x1000) {
        Float sine = njSin(heading);
        Float cosine = njCos(heading);
        Float *to = &robo_wall_puff.speed.x;
        Float speed = RANDOM(0.5F, 1.3F);
        Float x = 0.8F - (rand() * (4.0F * RAND_SCALE));
        Float y = sine * speed;
        Float z = cosine * speed;

        /* @hack spelling: the speed written from z down to x through to, which
         * SHC keeps in the frame at @R15 (0x0c90a43a to 0x0c90a446); through
         * the members, the function runs 96 bytes short and 440 of 500 words
         * differ */
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        /* @hack spelling: the pos written through to as the speed is
         * (0x0c90a478 to 0x0c90a486); through the members, the function runs 96
         * bytes short and 405 of 500 words differ */
        to = &robo_wall_puff.pos.x;
        speed = RANDOM(10.0F, 30.0F);
        x = 0.0F;
        y = sine * speed;
        z = cosine * speed;
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        robo_wall_puff.scale = RANDOM(6.0F, 9.0F);
        njCalcPoint(NULL, &robo_wall_puff.pos, &robo_wall_puff.pos);
        njCalcVector(NULL, &robo_wall_puff.speed, &robo_wall_puff.speed);
        final_puff_create(&robo_wall_puff);
    }
    njPopMatrix(1);
}

/* Zero dashes out through the wall along -x, from its position toward a point
 * 30 units out. the pieces fly the same way */
static void robo_wall_send_zero(struct task_work *work)
{
    struct zero_start start;

    start.how = ZERO_DASH;
    start.pos.x = 0.0F;
    start.pos.y = 0.0F;
    start.pos.z = 0.0F;
    start.target.x = -30.0F;
    start.target.y = 0.0F;
    start.target.z = 0.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(NULL, &start.pos, &start.pos);
    njCalcPoint(NULL, &start.target, &start.target);
    zero_appear(&start);
    njPopMatrix(1);
}
