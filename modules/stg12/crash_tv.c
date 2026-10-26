/* O Crash TV, a TV Zero dashes through once switch 5 turns on. the TV
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
#include "sound/sound.h"
#include "stg10/debris.h"
#include "stg12/crash_tv.h"
#include "stg12/crash_tv_object.h"
#include "stg12/robo_tv.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the frame the break leaves, which is crash_tv_object's root model */
extern NJS_MODEL crash_tv_frame_model;
extern NJS_OBJECT crash_tv_world_object;
extern NJS_OBJECT crash_tv_broken_world_object;

#define SE_CRASH_TV 309

#define SWITCH_CRASH_TV 5

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the landing a piece or chip stops on, as work->tick counts them. it bounces
 * on each landing before */
#define LANDINGS 4
/* the chips thrown beside each piece */
#define CHIPS 3
/* the puffs in the ring of dust */
#define PUFFS 16

enum crash_tv_mode { CRASH_INIT, CRASH_WAIT, CRASH_BREAK, CRASH_BROKEN };
/* no piece takes mode 2 */
enum crash_tv_piece_mode { PIECE_INIT = 0, PIECE_FLY = 1, PIECE_DONE = 3 };
enum crash_tv_chip_mode { CHIP_INIT, CHIP_FLY, CHIP_DONE };

/* init clears unk_00 and unk_01, and nothing reads them */
struct crash_tv_vars {
    Uint8 unk_00;
    Uint8 unk_01;
    Uint8 unk_02[6];
    /* its object in the collision world: the whole TV's, then the broken
     * TV's */
    NJS_OBJECT *body;
};

struct crash_tv_piece_vars {
    NJS_MODEL *model;
};

#define VARS(work) ((struct crash_tv_vars *)&(work)->vars)
#define PIECE(work) ((struct crash_tv_piece_vars *)&(work)->vars)

static void crash_tv_init(struct task *task);
static void crash_tv_move_body(struct task *task);
static void crash_tv_move_broken_body(struct task *task);
static void crash_tv_draw(struct task *task);
static void crash_tv_draw_broken(struct task *task);
static void crash_tv_destroy(struct task *task);
static void crash_tv_swap_body(struct task *task);
static void crash_tv_piece_draw(struct task *task);
static void crash_tv_piece_step(struct task *task);
static void crash_tv_break(struct task *task);
static void crash_tv_chip_step(struct task *task);
static void crash_tv_chip_draw(struct task *task);
static void crash_tv_dust(struct task_work *work);
static void crash_tv_send_zero(struct task_work *work);
static void crash_tv_wait_switch(struct task_work *work);

/* the robo TV that init creates as the TV's child. the step destroys it as
 * the TV breaks */
static struct task *robo_tv_task;

/* the position init gives the TV, wherever it was placed */
static NJS_POINT3 crash_tv_pos = {540.0F, 153.0F, -2789.4F};

/* no code reads this shape, but the image holds it */
static struct collision_shape crash_tv_shape = {
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

/* the params crash_tv_dust and each piece's first landing fill in. no puff
 * is ever made from them */
static struct dust_params crash_tv_puff = {
    1.0F,
    0.038F,
    0.05F,
    0.03F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* outside the set editor, the TV draws nothing until it breaks. the step it
 * breaks on draws it whole once, and later steps draw its frame */
void crash_tv_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case CRASH_INIT:
            crash_tv_init(task);
            task->draw = NULL;
            break;
        case CRASH_WAIT:
            crash_tv_wait_switch(work);
            break;
        case CRASH_BREAK:
            crash_tv_draw(task);
            task->draw = crash_tv_draw_broken;
            crash_tv_break(task);
            crash_tv_dust(work);
            task_destroy(robo_tv_task);
            crash_tv_swap_body(task);
            crash_tv_send_zero(work);
            work->mode = CRASH_BROKEN;
            break;
        case CRASH_BROKEN:
            crash_tv_draw_broken(task);
            crash_tv_move_broken_body(task);
            break;
        }
    } else {
        switch (work->mode) {
        case CRASH_INIT:
            crash_tv_init(task);
            break;
        case CRASH_WAIT:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            crash_tv_draw(task);
            crash_tv_move_body(task);
            break;
        }
    }
    task_step_children(task);
}

static void crash_tv_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *tv;
    NJS_OBJECT *body;
    NJS_OBJECT *from;

    task->draw = crash_tv_draw;
    task->destructor = crash_tv_destroy;
    work->pos.x = crash_tv_pos.x;
    work->pos.y = crash_tv_pos.y;
    work->pos.z = crash_tv_pos.z;
    work->ang.x = 0;
    work->ang.y = NJM_DEG_ANG(90);
    work->ang.z = 0;
    robo_tv_task =
        task_create_child(TASK_WORK | TASK_MOTION, robo_tv_step, task);
    tv = robo_tv_task->work;
    tv->pos.x = 540.0F;
    tv->pos.y = 228.0F;
    tv->pos.z = -2605.0F;
    tv->ang.x = 0;
    tv->ang.y = NJM_DEG_ANG(90);
    tv->ang.z = 0;
    body = world_object_alloc();
    VARS(work)->body = body;
    from = &crash_tv_world_object;
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
    work->mode = CRASH_WAIT;
}

static void crash_tv_move_body(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *body = VARS(work)->body;
    NJS_OBJECT *from = &crash_tv_world_object;

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

static void crash_tv_move_broken_body(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *body = VARS(work)->body;
    NJS_OBJECT *from = &crash_tv_broken_world_object;

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

static void crash_tv_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter2_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&crash_tv_object);
        njPopMatrix(1);
    }
}

static void crash_tv_draw_broken(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter2_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&crash_tv_frame_model);
        njPopMatrix(1);
    }
}

static void crash_tv_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->body);
    world_object_free(VARS(work)->body);
}

static void crash_tv_swap_body(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *body;
    NJS_OBJECT *from;

    world_remove(task, VARS(work)->body);
    world_object_free(VARS(work)->body);
    body = world_object_alloc();
    VARS(work)->body = body;
    from = &crash_tv_broken_world_object;
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

static void crash_tv_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter2_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        draw_model(PIECE(work)->model);
        njPopMatrix(1);
    }
}

/* a piece flies, spinning, and stops on its fourth landing. each landing
 * before that bounces it and keeps 0.6 of its spin. the first also fills
 * crash_tv_puff, but makes no puff */
static void crash_tv_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case PIECE_INIT:
        debris_set_body(task, NULL);
        task->draw = crash_tv_piece_draw;
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
                    crash_tv_puff.pos = work->pos;
                    crash_tv_puff.speed.x = RANDOM(-0.5F, 0.5F);
                    crash_tv_puff.speed.y = 0.2F;
                    crash_tv_puff.speed.z = RANDOM(-0.5F, 0.5F);
                    crash_tv_puff.scale = 3.0F;
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
    crash_tv_piece_draw(task);
}

/* throw a piece for each child of crash_tv_object, and CHIPS chips beside each.
 * a chip starts up to 5 units from its piece along each axis. all fly out from
 * the TV's origin and toward its -x. init turns the TV a quarter about y, which
 * points its -x along +z, the way Zero dashes */
static void crash_tv_break(struct task *task)
{
    /* @hack spelling: a copy of task, through which the function reads the work
     * and which every piece and chip takes as its parent. with it, SHC loads
     * each piece's y and z into FR12 and FR14, at 0x0c9063ea and 0x0c9063f2;
     * without it, the two swap and 10 of 298 words differ */
    struct task *parent = task;
    struct task_work *work;
    NJS_OBJECT *root = &crash_tv_object;
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
            struct task *child = task_create_child(TASK_WORK | TASK_MOTION,
                                                   crash_tv_piece_step, parent);
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
             * floats by FMOV.S @R4+ at 0x0c9063e8; read as node->pos[0], [1]
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
            x -= RANDOM(1.0F, 3.5F);
            /* @hack spelling: a cursor down from the end of motion->speed,
             * which stores z, y and x by FMOV.S @-R4 from 0x0c906414 on; stored
             * to the members in that order, 222 of 298 words differ */
            point = &motion->speed.x + 3;
            *--point = z;
            *--point = y;
            *--point = x;
            /* @hack type: 0 rather than NULL, which keeps side in R8, as MOVT
             * R8 at 0x0c9063dc shows; with NULL, side lives on the stack and
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
            struct task *child = task_create_child(TASK_WORK | TASK_MOTION,
                                                   crash_tv_chip_step, parent);
            struct task_work *chip = child->work;
            struct debris_motion *motion =
                (struct debris_motion *)child->motion;
            Float *point;
            Float x;
            Float y;
            Float z;
            Float scale;

            /* @hack redundant: a store of 0 to motion->spin.z that never runs.
             * it keeps 0 in R8 and the task at @R15, at 0x0c90637c and
             * 0x0c90637e. without it, the task takes R8, the function runs 4
             * bytes long and 255 of 298 words differ */
            if (0) {
                motion->spin.z = 0;
            }
            debris_pick_chip(chip, node->model);
            /* @hack spelling: a cursor through node->pos, which reads its
             * floats by FMOV.S @R4+ at 0x0c906480; read as node->pos[0], [1]
             * and [2], 151 of 298 words differ */
            point = node->pos;
            x = *point++;
            y = *point++;
            z = *point;
            x += RANDOM(-0.5F, 0.5F) * 10.0F;
            y += RANDOM(-0.5F, 0.5F) * 10.0F;
            /* @hack redundant: a copy of z to scale that never runs. it keeps
             * the loads at 0x0c9063b8 and 0x0c9063f2 in FR13 and FR14. without
             * it, the two trade and 17 of 298 words differ */
            if (0) {
                scale = z;
            }
            z += RANDOM(-0.5F, 0.5F) * 10.0F;
            /* @hack spelling: a cursor down from the end of chip->pos, which
             * stores z, y and x by FMOV.S @-R4 from 0x0c9064ce on; stored to
             * the members in that order, 135 of 298 words differ */
            point = &chip->pos.x + 3;
            *--point = z;
            *--point = y;
            *--point = x;
            scale = SQUARE(x) + SQUARE(y) + SQUARE(z);
            scale = _builtin_fsrra(scale);
            x *= scale;
            y *= scale;
            z *= scale;
            x -= RANDOM(2.0F, 4.0F);
            /* @hack spelling: a cursor down from the end of motion->speed,
             * which stores z, y and x by FMOV.S @-R4 from 0x0c906560 on; stored
             * to the members in that order, 113 of 298 words differ */
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
static void crash_tv_chip_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case CHIP_INIT:
        debris_set_body(task, NULL);
        task->draw = crash_tv_chip_draw;
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
    crash_tv_chip_draw(task);
}

/* draw a chip at its scale in work->scl.x, which crash_tv_break sets from
 * 0.5 to 1.5 */
static void crash_tv_chip_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    if (frame_step == 0) {
        njSetTexture(&hotshelter2_texlist);
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

/* work out a ring of PUFFS puffs about the TV's x axis, 10 to 30 units out.
 * each moves out from the axis and along +x. crash_tv_puff takes each in
 * turn, but no puff is made */
static void crash_tv_dust(struct task_work *work)
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
        Float *to = &crash_tv_puff.speed.x;
        Float speed = RANDOM(0.5F, 1.3F);
        Float x = 0.8F + (rand() * (4.0F * RAND_SCALE));
        Float y = sine * speed;
        Float z = cosine * speed;

        /* @hack spelling: a cursor down from the end of crash_tv_puff.speed,
         * which stores z, y and x from 0x0c906894 on; stored to the members in
         * that order, the function runs 80 bytes short and 413 of 476 words
         * differ */
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        to = &crash_tv_puff.pos.x;
        speed = RANDOM(10.0F, 30.0F);
        x = 0.0F;
        y = sine * speed;
        z = cosine * speed;
        /* @hack spelling: a cursor down from the end of crash_tv_puff.pos,
         * which stores z, y and x from 0x0c9068d2 on; stored to the members in
         * that order, the function runs 80 bytes short and 383 of 476 words
         * differ */
        to += 3;
        *--to = z;
        *--to = y;
        to[-1] = x;
        crash_tv_puff.scale = RANDOM(6.0F, 9.0F);
        njCalcPoint(NULL, &crash_tv_puff.pos, &crash_tv_puff.pos);
        njCalcVector(NULL, &crash_tv_puff.speed, &crash_tv_puff.speed);
    }
    njPopMatrix(1);
}

/* Zero dashes through the TV along +z. the dash starts 10 units short of the
 * TV's origin. it heads for a point 40 units past the origin and 1 up.
 * unlike the pieces' speeds, the points don't turn with the TV */
static void crash_tv_send_zero(struct task_work *work)
{
    struct zero_start start;

    start.how = ZERO_DASH;
    start.pos.x = 0.0F;
    start.pos.y = 0.0F;
    start.pos.z = -10.0F;
    start.target.x = 0.0F;
    start.target.y = 1.0F;
    start.target.z = 40.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njCalcPoint(NULL, &start.pos, &start.pos);
    njCalcPoint(NULL, &start.target, &start.target);
    zero_appear(&start);
    njPopMatrix(1);
}

static void crash_tv_wait_switch(struct task_work *work)
{
    if (switch_is_on(SWITCH_CRASH_TV)) {
        work->mode = CRASH_BREAK;
        sound_play(SE_CRASH_TV, 0, 0, 0);
    }
}
