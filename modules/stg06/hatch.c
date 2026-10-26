/* the hatches O CHAMBR: nine hatches in the deck that open as it tilts. an
 * open hatch releases a piece, which slides with the tilt */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg06/floor.h"
#include "stg06/hatch.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the square of the distance from the camera or player 0 within which a
 * piece stays. HATCH_RANGE is the same for a hatch and its lids */
#define PIECE_RANGE (2010.0F * 2010.0F)
#define HATCH_RANGE (3010.0F * 3010.0F)
/* the world tests a hatch while a player is within HATCH_NEAR of it */
#define HATCH_NEAR 520.0F
/* the radii draw_in_view tests a piece and a hatch with */
#define PIECE_VIEW 20.0F
#define HATCH_VIEW 100.0F

/* a shut hatch counts its steps, and can open once the count passes SHUT_WAIT.
 * it opens only while player 0 is within OPEN_NEAR of it */
#define SHUT_WAIT 60
#define OPEN_NEAR 1000.0F
/* an open hatch stays open while its piece is within PIECE_OUT of where the
 * piece started. the selected hatch starts to shut after SELECTED_OPEN steps
 * open */
#define PIECE_OUT 185.0F
#define SELECTED_OPEN 60
/* a shutting hatch shuts after SHUTTING_STEPS steps */
#define SHUTTING_STEPS 30
/* hatches_step picks the selected hatch at random from the first SELECTABLE of
 * the hatches table */
#define SELECTABLE 6.0F

/* a piece's speed along x and along z is PIECE_SLIDE times world_gravity's. a
 * piece on a floor draws a random number from 0 to 1 each step. it raises dust
 * when the number is above DUST_ABOVE */
#define PIECE_SLIDE 7.0F
#define DUST_ABOVE 0.66F
/* a piece bursts once farther than BURST_DISTANCE from its parent hatch, or
 * piece 8 once farther than BURST_DISTANCE_8. the explosion's scale is
 * BURST_SCALE */
#define BURST_DISTANCE 550.0F
#define BURST_DISTANCE_8 650.0F
#define BURST_SCALE 1.4F

/* a lid's height above its hatch. while the hatch is open, the lid slides
 * LID_SPEED a step, out to LID_TRAVEL from where it started */
#define LID_HEIGHT 25.0F
#define LID_SPEED 1.0F
#define LID_TRAVEL 25.0F

/* a hatch's work->mode. its lids slide out while it is HATCH_OPEN, and back
 * while it is HATCH_SHUTTING */
enum hatch_mode { HATCH_INIT, HATCH_SHUT, HATCH_OPEN, HATCH_SHUTTING };
/* the group's modes. in HATCHES_STOP it stops stepping its hatches, and no
 * step here sets it */
enum hatches_mode { HATCHES_INIT = 0, HATCHES_RUN = 1, HATCHES_STOP = 4 };
/* the modes of a part: a piece or a lid */
enum part_mode { PART_INIT, PART_RUN };

/* a hatch's work->tick bits */
/* the hatch hatches_step chose for Knuckles. it draws hatch_selected_object,
 * and releases no piece */
#define HATCH_SELECTED 0x1
/* the hatch's piece is within PIECE_OUT of where it started. the hatch sets the
 * bit as it releases its piece. the piece sets it each step it is that near.
 * the open hatch tests it, then clears it before it steps the piece */
#define HATCH_PIECE_NEAR 0x2

/* a lid's work->tick, which its spec's flags set. it is the lid's row of
 * hatch_shapes, and says which way the lid slides as its hatch opens */
#define LID_UP 9
#define LID_DOWN 10

struct hatch_vars {
    /* a piece's or a lid's model, or a hatch's world object */
    union {
        NJS_OBJECT *object;
        NJS_ACTION *action;
    } model;
    /* a piece's action frame, or a hatch's drawn object */
    union {
        Float frame;
        NJS_OBJECT *object;
    } value;
};

#define VARS(work) ((struct hatch_vars *)&(work)->vars)

/* a piece, as a row of hatch_pieces holds it. a piece with an action draws
 * the action, and one without draws its object. it starts at ang, and at
 * offset from its hatch along the world's axes */
struct hatch_piece {
    NJS_ACTION *action;
    NJS_OBJECT *object;
    struct angles ang;
    NJS_VECTOR offset;
    /* the piece's height above a floor it stands on. off a floor, it falls
     * no lower than this above its hatch */
    Float radius;
    /* scales scl.y as the piece lands. the next step sets scl.y again before
     * the piece moves, so the bounce never shows */
    Float bounce;
};

static void hatch_lid_step(struct task *task);
static void hatch_step(struct task *task);

extern NJS_OBJECT hatch_object;
extern NJS_OBJECT hatch_selected_object;
extern NJS_OBJECT hatch_lid_0_object;
extern NJS_OBJECT hatch_lid_1_object;
extern NJS_ACTION hatch_piece_4_action;
extern NJS_OBJECT hatch_piece_5_object;
extern NJS_OBJECT hatch_piece_6_object;
extern NJS_OBJECT hatch_piece_7_object;
extern NJS_OBJECT dat_0ca56494;
extern NJS_OBJECT dat_0ca57428;
extern NJS_OBJECT dat_0ca587f8;
extern NJS_OBJECT dat_0ca59290;
extern NJS_OBJECT dat_0ca5a820;
extern NJS_OBJECT dat_0ca5c48c;

/* each piece's shape by its row of hatch_pieces, then each lid's by its
 * tick, LID_UP or LID_DOWN */
static struct collision_shape hatch_shapes[11] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
         PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     1.15F,
     3.2F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     15.0F,
     5.0F,
     0.0F,
     {0, 0, 0xc000},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     4.6F,
     10.4F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
         PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     4.6F,
     4.79F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
         PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     4.6F,
     4.79F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
         PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     6.38F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
         PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     6.38F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
         PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     6.38F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     8.0F,
     8.9F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 12.5F, 0.0F},
     25.0F,
     12.5F,
     0.27F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -12.5F, 0.0F},
     25.0F,
     12.5F,
     0.27F,
     {0, 0, 0},
     0,
     NULL},
};
/* the glow dust a piece raises on a floor. piece_step sets its speed and pos
 * before each puff */
static struct dust_params hatch_dust = {
    0.7F,
    0.07F,
    0.1F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.32F, 0.8F, 0.8F, 1.0F},
};
/* the pieces, each starting 25 above its hatch. a hatch at y angle 0
 * releases piece 8, 185 toward -z of it. one at 0xc000 releases one of 0 to
 * 7, 185 toward +x of it */
static struct hatch_piece hatch_pieces[9] = {
    {NULL, &dat_0ca587f8, {0, 0, 0}, {185.0F, 25.0F, 0.0F}, 4.35F, -0.5F},
    {NULL, &dat_0ca5a820, {0, 0xc000, 0}, {185.0F, 25.0F, 0.0F}, 15.0F, -0.8F},
    {NULL, &dat_0ca59290, {0, 0, 0}, {185.0F, 25.0F, 0.0F}, 4.6F, -0.3F},
    {NULL, &dat_0ca57428, {0, 0, 0}, {185.0F, 25.0F, 0.0F}, 9.39F, -0.6F},
    {&hatch_piece_4_action,
     &dat_0ca56494,
     {0, 0xc000, 0},
     {185.0F, 25.0F, 0.0F},
     9.39F,
     -0.7F},
    {NULL,
     &hatch_piece_5_object,
     {0, 0xc000, 0},
     {185.0F, 25.0F, 0.0F},
     6.38F,
     -0.4F},
    {NULL,
     &hatch_piece_6_object,
     {0, 0xc000, 0},
     {185.0F, 25.0F, 0.0F},
     6.38F,
     -0.4F},
    {NULL,
     &hatch_piece_7_object,
     {0, 0xc000, 0},
     {185.0F, 25.0F, 0.0F},
     6.38F,
     -0.4F},
    {NULL, &dat_0ca5c48c, {0, 0, 0x4000}, {0.0F, 25.0F, -185.0F}, 8.0F, -0.4F},
};
static struct task_spec hatch_lids[3] = {
    {hatch_lid_step,
     TASK_WORK,
     LID_UP,
     {0, 0, 0},
     {0.0F, LID_HEIGHT, -2.0F},
     (Sint32)&hatch_lid_1_object},
    {hatch_lid_step,
     TASK_WORK,
     LID_DOWN,
     {0, 0, 0},
     {0.0F, LID_HEIGHT, -2.0F},
     (Sint32)&hatch_lid_0_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};
/* the nine hatches, at world positions. a spec's pos is turned by its parent's
 * angles and added to its parent's pos. so hatches_step puts its own task at
 * the origin, unturned, before it makes them */
static struct task_spec hatches[10] = {
    {hatch_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0, 0},
     {-781.9F, -200.0F, -295.0F},
     (Sint32)&hatch_object},
    {hatch_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0, 0},
     {-601.9F, -200.0F, -295.0F},
     (Sint32)&hatch_object},
    {hatch_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0xc000, 0},
     {601.0F, -100.0F, -298.64F},
     (Sint32)&hatch_object},
    {hatch_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0xc000, 0},
     {601.0F, -100.0F, -118.64F},
     (Sint32)&hatch_object},
    {hatch_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0xc000, 0},
     {601.0F, -100.0F, 118.8F},
     (Sint32)&hatch_object},
    {hatch_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0xc000, 0},
     {601.0F, -100.0F, 298.8F},
     (Sint32)&hatch_object},
    {hatch_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0, 0},
     {-691.9F, -200.0F, -295.0F},
     (Sint32)&hatch_object},
    {hatch_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0xc000, 0},
     {601.0F, -100.0F, -208.64F},
     (Sint32)&hatch_object},
    {hatch_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0xc000, 0},
     {601.0F, -100.0F, 208.8F},
     (Sint32)&hatch_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

static void hatch_draw(struct task *task);

static void part_draw(struct task *task)
{
    struct task_work *work = task->work;
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, PIECE_VIEW)) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        texture_set_stage();
        draw_tree(VARS(work)->model.object);
        njPopMatrix(1);
    }
}

/* draw the piece's action at its frame while it is in view. unless the set
 * editor holds the piece, step the frame on, back to 0 after the last.
 * task->draw runs this too, so the action plays while the game is paused */
static void piece_draw_action(struct task *task)
{
    struct task_work *work = task->work;
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, PIECE_VIEW)) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        texture_set_stage();
        draw_action(VARS(work)->model.action, VARS(work)->value.frame);
        njPopMatrix(1);
        if (!placement_editing(task)) {
            /* @hack spelling: the frame stepped in the test; stepped ahead of
             * it, 14 of 90 words differ and the function ends 2 bytes late */
            if ((VARS(work)->value.frame += 1.0F) >=
                (Float)VARS(work)->model.action->motion->nbFrame) {
                VARS(work)->value.frame = 0.0F;
            }
        }
    }
}

/* clear the model and kill the task. as the destructor, the kill comes to
 * nothing, since task_destroy then frees the task */
static void part_destroy(struct task *task)
{
    struct task_work *work = task->work;
    if (VARS(work)->model.object) {
        VARS(work)->model.object = 0;
    }
    task_kill(task);
}

/* burst the piece and kill its task if it is farther than distance from its
 * parent. a piece without a parent never bursts */
static void piece_burst_beyond(struct task *task, Float distance)
{
    struct task_work *work = task->work;
    if (task->parent) {
        if (njDistanceP2P(&work->pos, &task->parent->work->pos) > distance) {
            explosion_create_at(work->pos.x, work->pos.y, work->pos.z,
                                BURST_SCALE);
            part_destroy(task);
        }
    }
}

/* a piece's step. its row of hatch_pieces is its submode, which a piece with a
 * parent takes from the parent's. as it starts, the piece moves by the row's
 * offset, and keeps where it then is in task->extra.
 *
 * each later step, its speed, in scl, is world_gravity with x and z scaled by
 * PIECE_SLIDE. on a floor, sky_deck_floor_move moves it by that speed, and the
 * step moves it by the speed again. there, it raises dust on about one step in
 * three. off a floor, it falls no lower than its radius above its parent.
 *
 * a player that touches it takes world_gravity as its speed, with y made
 * positive. landing scales scl.y by the bounce, and an attack turns scl round.
 * the next step sets scl before the piece moves, so neither changes its path.
 *
 * each step it also turns by an eighth of gravity's tilt. piece 8 turns about
 * y, by the tilt about x taken away. pieces 0, 3, 5, 6 and 7 turn about x and
 * about z. each of their turns adds a small random part. pieces 1 and 2 turn
 * about z alone, and piece 4 not at all */
static void piece_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *home = task->extra;
    struct hatch_piece *piece;
    NJS_POINT3 pos;
    NJS_VECTOR spd;
    struct angles ang;
    Uint8 row;
    Uint8 kind;
    Float lowest;
    Float dist;
    char player;
    Angle tilt;

    switch (work->mode) {
    case PART_INIT:
        work->mode = PART_RUN;
        if (task->parent) {
            work->submode = (task->parent->work)->submode;
        }
        row = work->submode;
        piece = &hatch_pieces[row];
        njAddVector(&work->pos, &piece->offset);
        work->ang = piece->ang;
        /* @hack spelling: the action and the object read through
         * hatch_pieces[row], beside piece, which works out the row's address
         * again (EXTU.B R12,R2 at 0x0c909458); through piece, 454 of 497
         * words differ and hatch.c's code runs 32 bytes short */
        if (piece->action) {
            VARS(work)->model.action = hatch_pieces[row].action;
            task->draw = piece_draw_action;
        } else {
            VARS(work)->model.object = hatch_pieces[row].object;
            task->draw = part_draw;
        }
        collision_init(task, &hatch_shapes[row], COLLISION_OBJECT, 1, 1);
        task->destructor = part_destroy;
        work->scl.x = work->scl.y = work->scl.z = 0.0F;
        home->x = work->pos.x;
        home->y = work->pos.y;
        home->z = work->pos.z;
        break;
    case PART_RUN:
        work->scl.x = world_gravity.x * PIECE_SLIDE;
        work->scl.y = world_gravity.y;
        work->scl.z = world_gravity.z * PIECE_SLIDE;
        pos = work->pos;
        spd = work->scl;
        ang = work->ang;
        row = work->submode;
        if (sky_deck_floor_move(&pos, &spd, &ang, hatch_pieces[row].radius)) {
            work->pos.x = pos.x + spd.x;
            work->pos.y = pos.y + spd.y;
            work->pos.z = pos.z + spd.z;
            work->scl.x = spd.x;
            work->scl.y *= hatch_pieces[row].bounce;
            work->scl.z = spd.z;
            switch (work->submode) {
            case 1:
            case 2:
            case 4:
                work->ang.y = ang.y;
            }
            hatch_dust.speed = spd;
            pos.y -= hatch_pieces[row].radius;
            hatch_dust.pos = pos;
            if (rand() * RAND_SCALE > DUST_ABOVE) {
                dust_create_glow(&hatch_dust);
            }
        } else {
            lowest = task->parent->work->pos.y + hatch_pieces[row].radius;
            work->pos.x += work->scl.x;
            work->pos.y += work->scl.y;
            work->pos.z += work->scl.z;
            if (lowest > work->pos.y) {
                work->pos.y = lowest;
            }
        }
        dist = njDistanceP2P(&work->pos, home);
        if (dist < PIECE_OUT) {
            task->parent->work->tick |= HATCH_PIECE_NEAR;
        }
        if (work->collision->flags & COLLISION_HIT) {
            kind = work->collision->hit->kind;
            if (kind == COLLISION_PLAYER) {
                player = PLAYER_VARS(work->collision->hit->owner->work)->player;

                player_set_speed(player, world_gravity.x,
                                 world_gravity.y > 0.0F ? world_gravity.y
                                                        : -world_gravity.y,
                                 world_gravity.z);
            }
            if (kind == COLLISION_ATTACK) {
                work->scl.x *= -1.0F;
                if (work->scl.y < 0.0F) {
                    work->scl.y *= -1.0F;
                }
                work->scl.z += 5.0F;
                work->scl.z *= -1.0F;
            }
        }
        collision_register(work);

        /* @hack spelling: tilt set in the switch's comma expression, which
         * pools BURST_DISTANCE at 0x0c9097c8 ahead of world_gravity_ang_x's
         * address at 0x0c9097cc; set in a statement ahead of the switch, the
         * two trade and 13 of 497 words differ */
        switch (tilt = world_gravity_ang_x >> 3, row) {
        case 8:
            work->ang.y -= tilt & 0xffff;
            part_draw(task);
            piece_burst_beyond(task, BURST_DISTANCE_8);
            break;
        case 0:
        case 3:
        case 5:
        case 6:
        case 7:
            work->ang.x += (tilt + (int)(rand() * (1.0F / 180.0F))) & 0xffff;
            work->ang.z +=
                ((world_gravity_ang_z >> 3) + (int)(rand() * (1.0F / 180.0F))) &
                0xffff;
            part_draw(task);
            piece_burst_beyond(task, BURST_DISTANCE);
            break;
        case 1:
        case 2:
            work->ang.z += (world_gravity_ang_z >> 3) & 0xffff;
            part_draw(task);
            piece_burst_beyond(task, BURST_DISTANCE);
            break;
        case 4:
            piece_draw_action(task);
            piece_burst_beyond(task, BURST_DISTANCE);
            break;
        }
        break;
    }
}

/* the step of the piece a hatch releases, whose row is the hatch's submode */
static void hatch_piece_step(struct task *task)
{
    if (!placement_try_leave_by(task, PIECE_RANGE)) {
        piece_step(task);
    }
}

void hatch_piece_8_step(struct task *task)
{
    struct task_work *work = task->work;
    if (!placement_try_leave_by(task, PIECE_RANGE)) {
        if (edit_mode) {
            if (!VARS(work)->model.object) {
                VARS(work)->model.object = hatch_pieces[8].object;
            }
            work->ang.x = 0;
            work->ang.y = 0;
            work->ang.z = 0x4000;
            part_draw(task);
        } else {
            work->submode = 8;
            piece_step(task);
        }
    }
}

void hatch_piece_0_step(struct task *task)
{
    struct task_work *work = task->work;
    if (!placement_try_leave_by(task, PIECE_RANGE)) {
        if (edit_mode) {
            if (!VARS(work)->model.object) {
                VARS(work)->model.object = hatch_pieces[0].object;
            }
            work->ang.x = 0;
            work->ang.y = 0;
            work->ang.z = 0;
            part_draw(task);
        } else {
            work->submode = 0;
            piece_step(task);
        }
    }
}

void hatch_piece_1_step(struct task *task)
{
    struct task_work *work = task->work;
    if (!placement_try_leave_by(task, PIECE_RANGE)) {
        if (edit_mode) {
            if (!VARS(work)->model.object) {
                VARS(work)->model.object = hatch_pieces[1].object;
            }
            work->ang.x = 0;
            work->ang.y = 0xc000;
            work->ang.z = 0;
            part_draw(task);
        } else {
            work->submode = 1;
            piece_step(task);
        }
    }
}

void hatch_piece_2_step(struct task *task)
{
    struct task_work *work = task->work;
    if (!placement_try_leave_by(task, PIECE_RANGE)) {
        if (edit_mode) {
            if (!VARS(work)->model.object) {
                VARS(work)->model.object = hatch_pieces[2].object;
            }
            work->ang.x = 0;
            work->ang.y = 0;
            work->ang.z = 0;
            part_draw(task);
        } else {
            work->submode = 2;
            piece_step(task);
        }
    }
}

void hatch_piece_3_step(struct task *task)
{
    struct task_work *work = task->work;
    if (!placement_try_leave_by(task, PIECE_RANGE)) {
        if (edit_mode) {
            if (!VARS(work)->model.object) {
                VARS(work)->model.object = hatch_pieces[3].object;
            }
            work->ang.x = 0;
            work->ang.y = 0;
            work->ang.z = 0;
            part_draw(task);
        } else {
            work->submode = 3;
            piece_step(task);
        }
    }
}

void hatch_piece_4_step(struct task *task)
{
    struct task_work *work = task->work;
    if (!placement_try_leave_by(task, PIECE_RANGE)) {
        if (edit_mode) {
            if (!VARS(work)->model.action) {
                VARS(work)->model.action = &hatch_piece_4_action;
            }
            work->ang.x = 0;
            work->ang.y = 0xc000;
            work->ang.z = 0;
            piece_draw_action(task);
        } else {
            work->submode = 4;
            piece_step(task);
        }
    }
}

void hatch_piece_5_step(struct task *task)
{
    struct task_work *work = task->work;
    if (!placement_try_leave_by(task, PIECE_RANGE)) {
        if (edit_mode) {
            /* @bug memory: piece 5 sets value.object (MOV.L R3,@(12,R14) at
             * 0x0c909b34), but part_draw draws model.object (MOV.L @(8,R14),R4
             * at 0x0c90924a, for the call of draw_tree at 0x0c909248). the work
             * starts zeroed. so in the set editor, a piece 5 that no step
             * outside it has given a model passes null to draw_tree once in
             * view, and draw_tree reads through it */
            if (!VARS(work)->value.object) {
                VARS(work)->value.object = &hatch_piece_5_object;
            }
            work->ang.x = 0;
            work->ang.y = 0xc000;
            work->ang.z = 0;
            part_draw(task);
        } else {
            work->submode = 5;
            piece_step(task);
        }
    }
}

static void hatch_draw(struct task *task)
{
    struct task_work *work = task->work;
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, HATCH_VIEW)) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        texture_set_stage();
        njEasyDrawObject(VARS(work)->value.object);
        njPopMatrix(1);
    }
}

static void hatch_destroy(struct task *task)
{
    struct task_work *work = task->work;
    if (!placement_editing(task) && VARS(work)->model.object) {
        world_remove(task, VARS(work)->model.object);
        world_object_free(VARS(work)->model.object);
    }
    task_destroy_children(task);
}

/* set the hatch's drawn object, hatch_selected_object for the selected
 * hatch. put a copy of it in the world where the hatch stands. a null from
 * world_object_alloc isn't tested */
static void hatch_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    if (work->tick & HATCH_SELECTED) {
        VARS(work)->value.object = &hatch_selected_object;
    } else {
        VARS(work)->value.object = &hatch_object;
    }
    world = world_object_alloc();
    VARS(work)->model.object = world;
    world->evalflags = VARS(work)->value.object->evalflags &
                       ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = VARS(work)->value.object->model;
    world->child = VARS(work)->value.object->child;
    world->sibling = VARS(work)->value.object->sibling;
    world_add(WORLD_LARGE, task, world);
}

static void hatch_update_world(struct task *task)
{
    struct task_work *work = task->work;
    if (player_in_range(&work->pos, HATCH_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* a lid keeps where it started in scl. LID_UP lids slide up while their
 * hatch is open, and LID_DOWN lids down. they slide back while it shuts. a
 * hatch steps its lids only outside the set editor, so the editor branch
 * never runs */
static void hatch_lid_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *owner = task->parent->work;

    if (task->parent && placement_try_leave_by(task, HATCH_RANGE)) {
        return;
    }
    if (edit_mode) {
        /* @hack redundant: an empty test of the lid's model, whose load and TST
         * the image keeps (MOV.L @(8,R14),R3 at 0x0c909d82); without it, 9
         * of 173 words differ */
        if (VARS(work)->model.object) {
        }
        hatch_draw(task);
    } else {
        switch (work->mode) {
        case PART_INIT:
            work->mode = PART_RUN;
            work->ang.x = work->ang.z = 0;
            work->scl = work->pos;
            work->tick = work->flags;
            collision_init(task, &hatch_shapes[work->tick], COLLISION_OBJECT, 1,
                           1);
            task->draw = part_draw;
            task->destructor = part_destroy;
            break;
        case PART_RUN:
            switch (owner->mode) {
            case HATCH_OPEN:
                switch (work->tick) {
                case LID_UP:
                    if (work->pos.y - work->scl.y < LID_TRAVEL) {
                        work->pos.y += LID_SPEED;
                    }
                    break;
                case LID_DOWN:
                    if (work->pos.y - work->scl.y > -LID_TRAVEL) {
                        work->pos.y -= LID_SPEED;
                    }
                    break;
                }
                collision_register(work);
                part_draw(task);
                break;
            case HATCH_SHUTTING:
                switch (work->tick) {
                case LID_UP:
                    if (work->pos.y - work->scl.y > 0.0F) {
                        work->pos.y -= LID_SPEED;
                    }
                    break;
                case LID_DOWN:
                    if (work->pos.y - work->scl.y < 0.0F) {
                        work->pos.y += LID_SPEED;
                    }
                    break;
                }
                collision_register(work);
                part_draw(task);
                break;
            case HATCH_SHUT:
                collision_register(work);
                part_draw(task);
                break;
            }
            break;
        }
    }
}

/* a hatch at y angle 0 releases piece 8. it opens while world_gravity.z is
 * above 0 and world_gravity.x is 0. a hatch at 0xc000 releases one of pieces 0
 * to 7 at random. it opens while world_gravity.x is below 0 and world_gravity.z
 * is 0. the selected hatch opens, but releases nothing. after a hatch shuts,
 * its count starts at a random 0 to SHUT_WAIT - 1 */
static void hatch_step(struct task *task)
{
    struct task_work *work = task->work;

    if (task->parent && placement_try_leave_by(task, HATCH_RANGE)) {
        return;
    }
    if (edit_mode) {
        if (!VARS(work)->value.object) {
            VARS(work)->value.object = &hatch_object;
        }
        hatch_draw(task);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njTranslate(NULL, 0.0F, LID_HEIGHT, 0.0F);
        texture_set_stage();
        njEasyDrawObject(&hatch_lid_0_object);
        njEasyDrawObject(&hatch_lid_1_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case HATCH_INIT:
            work->mode = HATCH_SHUT;
            hatch_add_world(task);
            work->ang.x = work->ang.z = 0;
            task->draw = hatch_draw;
            task->destructor = hatch_destroy;
            task_create_children(hatch_lids, task);
            return;
        case HATCH_SHUT:
            if (++TASK_TIMER(work) > SHUT_WAIT) {
                NJS_POINT3 *target = &player_work[0]->pos;

                /* @hack redundant: an empty never-run branch ahead of the
                 * distance test, which loads njDistanceP2P's address into R1,
                 * not R2, at 0x0c90a052; without it, 2 of 276 words differ */
                if (0) {
                } else if (njDistanceP2P(&work->pos, target) < OPEN_NEAR) {
                    if ((Uint16)work->ang.y == 0 && world_gravity.z > 0.0F &&
                        world_gravity.x == 0.0F) {
                        work->mode = HATCH_OPEN;
                        work->submode = 8;
                        TASK_TIMER(work) = 0;
                        if (!(work->tick & HATCH_SELECTED)) {
                            task_create_child(TASK_WORK | TASK_EXTRA,
                                              hatch_piece_step, task);
                            work->tick |= HATCH_PIECE_NEAR;
                        }
                    } else if ((Uint16)work->ang.y == 0xc000 &&
                               world_gravity.x < 0.0F &&
                               world_gravity.z == 0.0F) {
                        work->mode = HATCH_OPEN;
                        work->submode = floorf(rand() * (8.0F * RAND_SCALE));
                        TASK_TIMER(work) = 0;
                        if (!(work->tick & HATCH_SELECTED)) {
                            task_create_child(TASK_WORK | TASK_EXTRA,
                                              hatch_piece_step, task);
                            work->tick |= HATCH_PIECE_NEAR;
                        }
                    } else {
                        TASK_TIMER(work) = SHUT_WAIT;
                    }
                }
            }
            hatch_update_world(task);
            hatch_draw(task);
            task_step_children(task);
            break;
        case HATCH_OPEN:
            if ((work->tick & HATCH_SELECTED &&
                 ++TASK_TIMER(work) > SELECTED_OPEN) ||
                !(work->tick & (HATCH_SELECTED | HATCH_PIECE_NEAR))) {
                work->mode = HATCH_SHUTTING;
                TASK_TIMER(work) = 0;
            }
            hatch_update_world(task);
            hatch_draw(task);
            work->tick &= ~HATCH_PIECE_NEAR;
            task_step_children(task);
            break;
        case HATCH_SHUTTING:
            if (++TASK_TIMER(work) > SHUTTING_STEPS) {
                work->mode = HATCH_SHUT;
                TASK_TIMER(work) = floorf(rand() * (SHUT_WAIT * RAND_SCALE));
            }
            hatch_update_world(task);
            hatch_draw(task);
            task_step_children(task);
            break;
        }
    }
}

static void hatches_destroy(struct task *task)
{
    task_destroy_children(task);
}

void hatches_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    unsigned int index;
    NJS_OBJECT *lid1;
    NJS_OBJECT *lid2;
    NJS_OBJECT *hatch;
    Bool (*push)(NJS_MATRIX *);

    if (placement_try_leave_by(task, HATCH_RANGE)) {
        return;
    }
    if (edit_mode) {
        /* @hack spelling: njPushMatrix called through push, which SHC keeps in
         * R12 (0x0c90a214); called by name, it and lid1 trade R12 and R9, and
         * 9 of 216 words differ */
        push = njPushMatrix;
        push(NULL);
        texture_set_stage();
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        push(NULL);
        njTranslate(NULL, 0.0F, 0.0F, 0.0F);

        /* @hack spelling: the first draw by name, though hatch holds it, which
         * keeps lid1 in R9 and lid2 in R10 (0x0c90a260, 0x0c90a266); drawn
         * through hatch, the two trade and 8 of 216 words differ */
        hatch = &hatch_object;
        njEasyDrawObject(&hatch_object);
        njTranslate(NULL, 0.0F, LID_HEIGHT, 0.0F);
        lid1 = &hatch_lid_0_object;
        njEasyDrawObject(lid1);
        lid2 = &hatch_lid_1_object;
        njEasyDrawObject(lid2);
        njPopMatrix(1);
        push(NULL);
        njTranslate(NULL, 90.0F, 0.0F, 0.0F);
        njEasyDrawObject(hatch);
        njTranslate(NULL, 0.0F, LID_HEIGHT, 0.0F);
        njEasyDrawObject(lid1);
        njEasyDrawObject(lid2);
        njPopMatrix(1);
        push(NULL);
        njTranslate(NULL, -90.0F, 0.0F, 0.0F);
        njEasyDrawObject(hatch);
        njTranslate(NULL, 0.0F, LID_HEIGHT, 0.0F);
        njEasyDrawObject(lid1);
        njEasyDrawObject(lid2);
        njPopMatrix(1);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case HATCHES_INIT:
            work->mode = HATCHES_RUN;
            work->pos.x = work->pos.y = work->pos.z = 0.0F;
            work->ang.x = work->ang.y = work->ang.z = 0;
            task->destructor = hatches_destroy;
            task_create_children(hatches, task);
            if (get_character() == CHARACTER_KNUCKLES) {
                child = task->child;
                index = floorf(rand() * (SELECTABLE * RAND_SCALE));
                for (; index != 0; index--) {
                    child = child->next;
                }
                child->work->tick |= HATCH_SELECTED;
            }
            break;
        case HATCHES_RUN:
            task_step_children(task);
            break;
        case HATCHES_STOP:
            break;
        }
    }
}
