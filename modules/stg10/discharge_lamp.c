/* O DSCHG_LAMP bobbing lamp whose beam goes on and off for the times its set
 * entry gives. an attack bursts it into four pieces */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "enemy/enemy.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "sound/sound.h"
#include "stg10/discharge_lamp.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the whole lamp, its beam, and from the hit on, the lamp without the four
 * pieces */
extern NJS_ACTION discharge_lamp_action;
extern NJS_ACTION discharge_lamp_beam_action;
extern NJS_ACTION discharge_lamp_broken_action;
/* the nodes the four pieces take their models and starting places from */
extern NJS_OBJECT discharge_lamp_piece_0_object;
extern NJS_OBJECT discharge_lamp_piece_1_object;
extern NJS_OBJECT discharge_lamp_piece_2_object;
extern NJS_OBJECT discharge_lamp_piece_3_object;

/* the lamp is destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
#define REACH 200.0F /* the collision record's radius */

#define SE_EXPLOSION 25

#define BOB 1.2F /* how far the lamp bobs either way */
#define BOB_STEP 0x200 /* how far the bob's phase moves a step */
#define RATE 2.9F /* the lamp's action's frames a step */

/* the head's center, HEAD_Y above the lamp's position. the pieces spread
 * from it */
#define HEAD_Y 14.0F
#define PIECES 4

/* when the lamp is hit, PUFFS puffs fly out in a ring at PUFF_SPEED. each is
 * PUFF_SCALE times a puff's size */
#define PUFFS 16
#define PUFF_SPEED 40.0F
#define PUFF_SCALE 1.4F

/* each step, a piece's speed down grows by GRAVITY. in the air, the piece keeps
 * AIR_KEPT of its speed and of its spin. a piece that sinks below 1 unit above
 * the floor is put back at 1. its vertical speed then reverses, scaled by
 * BOUNCE. it keeps GROUND_KEPT of its horizontal speed, and GROUND_SPIN_KEPT of
 * its spin */
#define GRAVITY 0.027222222F
#define AIR_KEPT 0.97F
#define BOUNCE 0.86F
#define GROUND_KEPT 0.93F
#define GROUND_SPIN_KEPT 0.9F

/* an action's last frame, at which its frame wraps */
#define LAST_FRAME(action) ((action)->motion->nbFrame - 1.0F)

/* in work->tick: the beam is off */
#define BEAM_OFF 0x2

/* a hurt lamp bursts on LAMP_SHAKING's step SHAKE_STEPS. on LAMP_FALLING's
 * steps, it shakes before step SHAKE_END, and blasts can go off before step
 * BLAST_END. its pieces stop after step FALL_END + 1 */
#define SHAKE_STEPS 20
#define SHAKE_END 40
#define BLAST_END 130
#define FALL_END 160

struct discharge_lamp_piece {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct angles ang;
    struct angles spin; /* added to ang each step */
};

/* the lamp's record, from the heap, which the destructor frees */
struct discharge_lamp_state {
    Float floor_y; /* below the lamp, where its shadow and its pieces lie */
    Float rate; /* frames of its action a step */
    Float frame; /* the frame of the lamp's action, whole or broken */
    Uint8 unk_0c[4];
    Angle phase; /* the bob's phase */
    /* the lamp's tilt about x and about z. the tilt shakes the lamp from the
     * hit until SHAKE_END steps past the burst */
    Angle tilt_x;
    Angle tilt_z;
    Sint32 timer; /* steps the beam has been on, or off */
    struct discharge_lamp_piece pieces[PIECES];
};

struct discharge_lamp_vars {
    Float frame; /* the frame of the beam's action */
    Float bob; /* the lamp's height above its position */
    struct discharge_lamp_state *state;
};

#define VARS(work) ((struct discharge_lamp_vars *)&(work)->vars)

enum lamp_mode { LAMP_INIT, LAMP_RUN };
/* work->timer numbers LAMP_SHAKING's and LAMP_FALLING's steps from 1, and
 * TASK_TIMER reads it unsigned */
enum lamp_submode {
    LAMP_WHOLE,
    LAMP_SHAKING, /* hurt, for SHAKE_STEPS steps */
    LAMP_FALLING, /* burst, for FALL_END + 1 steps */
    LAMP_FALLEN
};

static void discharge_lamp_init(struct task *task);
static void discharge_lamp_reset(struct task *task);
static void discharge_lamp_run(struct task *task);
static void discharge_lamp_draw(struct task *task);
static void discharge_lamp_draw_pieces(struct discharge_lamp_state *state);
static void discharge_lamp_destroy(struct task *task);
static void discharge_lamp_spread_pieces(struct task_work *work,
                                         Float distance);
static void discharge_lamp_scatter_pieces(struct task_work *work);

static NJS_OBJECT *discharge_lamp_pieces[PIECES] = {
    &discharge_lamp_piece_0_object, &discharge_lamp_piece_1_object,
    &discharge_lamp_piece_2_object, &discharge_lamp_piece_3_object};

/* the head and the base, then the beam. the beam reaches from 6 to 142 units
 * ahead, and hurts players */
static struct collision_shape discharge_lamp_shapes[3] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 13.0F, 0.0F},
     10.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 5.2F, 0.0F},
     4.0F,
     3.5F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 5.0F, 74.0F},
     1.0F,
     1.0F,
     68.0F,
     {0},
     0,
     NULL},
};

/* the puffs share it: each takes its position and speed as it starts */
static struct dust_params discharge_lamp_puff = {
    1.0F,
    0.1F,
    0.19F,
    0.62F,
    0.07F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static Uint8 dat_0cb00618[4] = {0};

/* in the lamp's frame: where the puffs start, where it bursts, and where the
 * small blasts go off as the pieces fall */
static NJS_POINT3 discharge_lamp_puff_from = {0.0F, HEAD_Y, 0.0F};
static NJS_POINT3 discharge_lamp_burst_from = {0.0F, HEAD_Y, 0.0F};
static NJS_POINT3 discharge_lamp_blast_from = {0.0F, HEAD_Y, 0.0F};

void discharge_lamp_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case LAMP_INIT:
            discharge_lamp_init(task);
            break;
        case LAMP_RUN:
            discharge_lamp_draw(task);
            discharge_lamp_run(task);
            break;
        }
    } else {
        switch (work->mode) {
        case LAMP_INIT:
            discharge_lamp_init(task);
            break;
        case LAMP_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> OFF TIME");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> ON TIME");
            discharge_lamp_draw(task);
            discharge_lamp_run(task);
            break;
        }
    }
}

/* @hack spelling: defined after discharge_lamp_step, which places it after the
 * step's strings among the constants, at 0x0c919724. defined above the step,
 * it comes before them: 21 of the constants' 24 words differ, and so do the 4
 * pool words that hold their addresses */
static const NJS_VECTOR discharge_lamp_spray = {0.0F, 0.0F, 0.0F};

static void discharge_lamp_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = discharge_lamp_draw;
    task->destructor = discharge_lamp_destroy;
    enemy_alloc_record(task, work);
    collision_init(task, discharge_lamp_shapes, COLLISION_ITEM, 1, 3);
    work->collision->radius = REACH;
    discharge_lamp_reset(task);
    work->mode = LAMP_RUN;
}

/* give the lamp its record, find its floor, and turn its beam on */
static void discharge_lamp_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct discharge_lamp_state *state;
    struct angles slope;

    VARS(work)->state = heap_alloc(sizeof *state);
    state = VARS(work)->state;
    work->collision->flags |= COLLISION_TARGET;
    work->submode = LAMP_WHOLE;
    VARS(work)->frame = 0.0F;
    work->tick = 0;
    work->timer = 0;
    VARS(work)->bob = 0.0F;
    state->timer = 0;
    state->floor_y =
        ground_below(work->pos.x, work->pos.y + 10.0F, work->pos.z, &slope);
    state->rate = RATE;
    state->frame = 0.0F;
    state->phase = 0;
    state->tilt_x = 0;
    state->tilt_z = 0;
    work->collision->shapes[2].flags &= ~SHAPE_OFF;
}

/* step one piece: move it by add_vector, turn it, and bounce it on the floor.
 * on the floor it keeps ground_kept of its horizontal speed. while trail is
 * set, it trails emit's flames. piece then points at the next piece. the run
 * passes its locals for njAddVector, flame_create and GROUND_KEPT */
#define STEP_PIECE(piece, state, add_vector, emit, trail, ground_kept)         \
    {                                                                          \
        add_vector(&(piece)->pos, &(piece)->speed);                            \
        (piece)->ang.x += (piece)->spin.x;                                     \
        (piece)->ang.y += (piece)->spin.y;                                     \
        (piece)->ang.z += (piece)->spin.z;                                     \
        if ((piece)->pos.y < (state)->floor_y + 1.0F) {                        \
            (piece)->pos.y = (state)->floor_y + 1.0F;                          \
            (piece)->speed.y = -((piece)->speed.y * BOUNCE);                   \
            (piece)->speed.x *= (ground_kept);                                 \
            (piece)->speed.z *= (ground_kept);                                 \
            (piece)->spin.x *= GROUND_SPIN_KEPT;                               \
            (piece)->spin.y *= GROUND_SPIN_KEPT;                               \
            (piece)->spin.z *= GROUND_SPIN_KEPT;                               \
        } else {                                                               \
            (piece)->speed.x *= AIR_KEPT;                                      \
            (piece)->speed.y *= AIR_KEPT;                                      \
            (piece)->speed.z *= AIR_KEPT;                                      \
            (piece)->spin.x *= AIR_KEPT;                                       \
            (piece)->spin.y *= AIR_KEPT;                                       \
            (piece)->spin.z *= AIR_KEPT;                                       \
        }                                                                      \
        (piece)->speed.y -= GRAVITY;                                           \
        if (trail)                                                             \
            emit(&(piece)->pos, &(piece)->speed, 1.0F);                        \
        /* @hack spelling: piece = piece + 1, which keeps flame_create's       \
         * address in R8 and njAddVector's in R9 (0x0c9018a4, 0x0c9018ae); as  \
         * ++piece, the two trade and 10 of discharge_lamp_run's 1206 words    \
         * differ */                                                           \
        (piece) = (piece) + 1;                                                 \
    }

/* while the lamp is whole, its beam is on for scale y steps, then off for scale
 * x. with scale x below 1, the beam stays on. once hurt, the lamp puffs and
 * shakes, and bursts SHAKE_STEPS steps later. its pieces then fly and fall for
 * FALL_END + 1 steps, and stop where they lie. blasts go off at random for the
 * first BLAST_END - 1 of those steps */
static void discharge_lamp_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct discharge_lamp_state *state = VARS(work)->state;
    NJS_ACTION *action;
    struct discharge_lamp_piece *piece;
    NJS_POINT3 point;
    NJS_VECTOR spray;
    NJS_VECTOR speed;
    Angle ang;
    Bool (*pop)(Uint32);
    void (*calc_point)(NJS_MATRIX *, NJS_POINT3 *, NJS_POINT3 *);
    void (*emit)(NJS_POINT3 *, NJS_VECTOR *, Float);
    void (*emitter)(NJS_POINT3 *, NJS_VECTOR *, Float);
    void (*add_vector)(NJS_VECTOR *, NJS_VECTOR *);
    void (*translate)(NJS_MATRIX *, Float, Float, Float);
    Float ground_kept;
    int i;
    int trail;
    int pieces_left;
    Sint32 shake;

    state->frame += state->rate;
    /* @hack spelling: action set in the test, which puts its address after
     * -1.0F in the pool (0x0c901258); set before the test, the two trade, and 2
     * of discharge_lamp_run's 1206 words and 4 pool words differ */
    if (state->frame >= LAST_FRAME(action = &discharge_lamp_action)) {
        state->frame -= LAST_FRAME(action);
    } else if (state->frame < 0.0F) {
        state->frame += LAST_FRAME(action);
    }
    VARS(work)->bob = njSin(state->phase) * BOB;
    state->phase += BOB_STEP;
    /* @hack spelling: njTranslate called through a local, which keeps its
     * address in R9 (0x0c90129a); called by name, 75 of discharge_lamp_run's
     * 1206 words and 10 pool words differ */
    translate = njTranslate;
    /* @hack spelling: njCalcPoint called through a local, which keeps its
     * address in R14 (0x0c901292); called by name, 4 of the run's 1206 words
     * and 2 pool words differ */
    calc_point = njCalcPoint;
    /* @hack spelling: njPopMatrix called through a local, which keeps its
     * address in R8 (0x0c901298); called by name, 990 of the run's 1206 words
     * differ */
    pop = njPopMatrix;
    switch (work->submode) {
    case LAMP_WHOLE:
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame >= LAST_FRAME(&discharge_lamp_beam_action)) {
            VARS(work)->frame -= LAST_FRAME(&discharge_lamp_beam_action);
        }
        if (!(work->tick & BEAM_OFF)) {
            if (work->scl.x >= 1.0F) {
                state->timer++;
                if (state->timer >= work->scl.y) {
                    work->tick |= BEAM_OFF;
                    state->timer = 0;
                    collision->shapes[2].flags |= SHAPE_OFF;
                }
            }
        } else {
            state->timer++;
            if (state->timer >= work->scl.x) {
                work->tick &= ~BEAM_OFF;
                state->timer = 0;
                collision->shapes[2].flags &= ~SHAPE_OFF;
            }
        }
        if (work->flags & COLLISION_DAMAGED) {
            sound_play(SE_EXPLOSION, 0, 0, 0);
            lockon_release(task);
            collision->shapes[0].levels |= SHAPE_GUARD;
            collision->flags &= ~COLLISION_TARGET;
            collision->shapes[2].flags |= SHAPE_OFF;
            njPushMatrix(&_unit_matrix_);
            translate(NULL, work->pos.x, work->pos.y + VARS(work)->bob,
                      work->pos.z);
            TURN_VIA(njRotateY, work->ang.y, ang);
            spray = discharge_lamp_spray;
            calc_point(NULL, &discharge_lamp_puff_from, &point);
            for (i = 0; i < PUFFS; i++) {
                spray.x = njCos(i * (0x10000 / PUFFS)) * PUFF_SPEED;
                spray.z = njSin(i * (0x10000 / PUFFS)) * PUFF_SPEED;
                spray.y = 0.0F;
                njCalcVector(NULL, &spray, &speed);
                discharge_lamp_puff.scale = PUFF_SCALE;
                discharge_lamp_puff.speed = speed;
                discharge_lamp_puff.pos = point;
                dust_create_from(&discharge_lamp_puff);
            }
            discharge_lamp_spread_pieces(work, 0.0F);
            pop(1);
            work->submode = LAMP_SHAKING;
            work->timer = 0;
        }
        break;
    case LAMP_SHAKING:
        VARS(work)->frame += (rand() * (1.0F / 8192.0F)) + 2.0F;
        if (VARS(work)->frame >= LAST_FRAME(&discharge_lamp_beam_action)) {
            VARS(work)->frame -= LAST_FRAME(&discharge_lamp_beam_action);
        }
        shake = (30 - TASK_TIMER(work)) * 15;
        state->tilt_x = shake * njSin(TASK_TIMER(work) * 0x4000);
        state->tilt_z = shake * njSin(TASK_TIMER(work) * 0x3000);
        njPushMatrix(&_unit_matrix_);
        translate(NULL, work->pos.x, work->pos.y + VARS(work)->bob,
                  work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, state->tilt_x, ang);
        TURN_VIA(njRotateZ, state->tilt_z, ang);
        {
            Float distance;

            if (TASK_TIMER(work) < 10) {
                distance = TASK_TIMER(work) * 0.1F;
            } else {
                distance = 1.0F;
            }
            discharge_lamp_spread_pieces(work, distance);
        }
        if (TASK_TIMER(work) >= SHAKE_STEPS) {
            sound_play(SE_EXPLOSION, 0, 0, 0);
            calc_point(NULL, &discharge_lamp_burst_from, &point);
            bomb_create(&point, 1.0F);
            discharge_lamp_scatter_pieces(work);
            work->tick |= BEAM_OFF;
            work->submode = LAMP_FALLING;
            work->timer = 0;
        }
        pop(1);
        break;
    case LAMP_FALLING:
        if (TASK_TIMER(work) < SHAKE_END) {
            shake = (SHAKE_END - TASK_TIMER(work)) * 30;
            state->tilt_x = shake * njSin(TASK_TIMER(work) * 0x4000);
            state->tilt_z = shake * njSin(TASK_TIMER(work) * 0x3000);
        } else {
            state->tilt_x = 0;
            state->tilt_z = 0;
        }
        if (TASK_TIMER(work) < BLAST_END && rand() * RAND_SCALE < 0.1F) {
            njPushMatrix(&_unit_matrix_);
            translate(
                NULL,
                work->pos.x + (((rand() * (1.0F / 16384.0F)) - 1.0F) * 10.0F),
                work->pos.y + VARS(work)->bob,
                work->pos.z + (((rand() * (1.0F / 16384.0F)) - 1.0F) * 10.0F));
            TURN_VIA(njRotateY, work->ang.y, ang);
            TURN_VIA(njRotateX, state->tilt_x, ang);
            TURN_VIA(njRotateZ, state->tilt_z, ang);
            calc_point(NULL, &discharge_lamp_blast_from, &point);
            bomb_create(&point, 0.2F);
            pop(1);
        }
        if (!(TASK_TIMER(work) & 3)) {
            trail = 1;
        } else {
            trail = 0;
        }
        piece = state->pieces;
        pieces_left = PIECES;
        /* @hack spelling: flame_create called through a local; called by name,
         * 20 of discharge_lamp_run's 1206 words and 6 pool words differ */
        emitter = flame_create;
        /* @hack spelling: emit, a copy of emitter, which keeps flame_create's
         * address in R8 and njAddVector's in R9 (0x0c9018a4, 0x0c9018ae);
         * through emitter alone, the two trade and 12 of the run's 1206 words
         * differ */
        emit = emitter;
        /* @hack spelling: njAddVector called through a local; called by name,
         * the run's frame grows from 56 bytes to 60 (ADD #-56,R15 at
         * 0x0c9011dc), and 603 of its 1206 words differ */
        add_vector = njAddVector;
        /* @hack spelling: GROUND_KEPT through a local; in its place, the run's
         * frame grows to 60 bytes, and 600 of its 1206 words differ */
        ground_kept = GROUND_KEPT;
        /* @hack spelling: the four pieces' steps written as one pass of a
         * loop, with the count SHC keeps in R10 when it unrolls one (ADD #-1
         * three times, then DT at 0x0c901c7e). as a for loop over the pieces,
         * which SHC unrolls the same way, njAddVector's address loads before
         * flame_create's, and 2 of the run's 1206 words differ */
        while (pieces_left != 0) {
            STEP_PIECE(piece, state, add_vector, emit, trail, ground_kept);
            --pieces_left;
            STEP_PIECE(piece, state, add_vector, emit, trail, ground_kept);
            --pieces_left;
            STEP_PIECE(piece, state, add_vector, emit, trail, ground_kept);
            --pieces_left;
            STEP_PIECE(piece, state, add_vector, emit, trail, ground_kept);
            --pieces_left;
        }
        if (TASK_TIMER(work) > FALL_END) {
            work->submode = LAMP_FALLEN;
        }
        break;
    }
    collision->shapes[0].center.y = VARS(work)->bob + 13.0F;
    collision->shapes[1].center.y = VARS(work)->bob + 5.2F;
    collision->shapes[2].center.y = VARS(work)->bob + 5.0F;
    work->timer++;
    collision_register(work);
}

#undef STEP_PIECE

/* on a frame's first step, draw the shadow on the floor, smaller the higher
 * the lamp stands. then draw the lamp, whole or broken, and its beam while the
 * beam is on. once the lamp is hurt, draw its pieces too */
static void discharge_lamp_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct discharge_lamp_state *state = VARS(work)->state;
    Bool (*set_textures)(void);
    void (*draw)(NJS_ACTION *, Float);
    Float scale;
    Angle ang;

    if (frame_step) {
        return;
    }
    scale = 1.0F / (1.0F + (_builtin_fabsf(work->pos.y + VARS(work)->bob -
                                           state->floor_y) *
                            0.03F));
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, state->floor_y + 0.2F, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njRotateY(NULL, 0x4000);
    njScale(NULL, scale, scale, scale * 1.4F);
    texture_set_regular();
    njEasyDrawObject(shadow_object);
    njPopMatrix(1);
    /* @hack spelling: draw_action called through a local, which keeps its
     * address in R11 (0x0c901dc0); called by name, 12 of
     * discharge_lamp_draw's 253 words and 4 pool words differ */
    draw = draw_action;
    /* @hack spelling: texture_set_stage called through a local, which keeps its
     * address in R12 (0x0c901dbe); called by name, 20 of the draw's 253 words
     * differ */
    set_textures = texture_set_stage;
    switch (work->submode) {
    case LAMP_WHOLE:
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->bob,
                    work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        if (!(work->tick & BEAM_OFF)) {
            set_textures();
            draw(&discharge_lamp_beam_action, VARS(work)->frame);
        }
        set_textures();
        draw(&discharge_lamp_action, state->frame);
        njPopMatrix(1);
        break;
    case LAMP_SHAKING:
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->bob,
                    work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, state->tilt_x, ang);
        TURN_VIA(njRotateZ, state->tilt_z, ang);
        if (!(work->tick & BEAM_OFF)) {
            set_textures();
            draw(&discharge_lamp_beam_action, VARS(work)->frame);
        }
        set_textures();
        draw(&discharge_lamp_broken_action, state->frame);
        njPopMatrix(1);
        discharge_lamp_draw_pieces(state);
        break;
    case LAMP_FALLING:
    case LAMP_FALLEN:
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->bob,
                    work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, state->tilt_x, ang);
        TURN_VIA(njRotateZ, state->tilt_z, ang);
        set_textures();
        draw(&discharge_lamp_broken_action, state->frame);
        njPopMatrix(1);
        discharge_lamp_draw_pieces(state);
        break;
    }
}

static void discharge_lamp_draw_pieces(struct discharge_lamp_state *state)
{
    struct discharge_lamp_piece *piece;
    NJS_OBJECT **object;
    int i;
    Angle ang;

    texture_set_stage();
    object = discharge_lamp_pieces;
    piece = state->pieces;
    for (i = 0; i < PIECES; i++) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &piece->pos);
        TURN_VIA(njRotateY, piece->ang.y, ang);
        TURN_VIA(njRotateX, piece->ang.x, ang);
        TURN_VIA(njRotateZ, piece->ang.z, ang);
        draw_model((*object)->model);
        njPopMatrix(1);
        piece++;
        object++;
    }
}

static void discharge_lamp_destroy(struct task *task)
{
    heap_free(VARS(task->work)->state);
    /* @hack redundant: a goto to the next statement, which keeps heap_free a
     * call (JSR at 0x0c9020ec); without it, SHC makes the call a jump, and 11
     * of discharge_lamp_destroy's 11 words differ */
    goto done;
done:
    return;
}

/* place each piece where its node sits, moved distance further out from the
 * head's center. the current matrix maps the place into the world. each piece
 * takes the lamp's turn and tilt */
static void discharge_lamp_spread_pieces(struct task_work *work, Float distance)
{
    struct discharge_lamp_state *state = VARS(work)->state;
    NJS_OBJECT **object = discharge_lamp_pieces;
    struct discharge_lamp_piece *piece = state->pieces;
    Float x;
    Float y;
    Float z;
    Float length;
    int i;
    /* @hack redundant: 12 bytes never used, which give the image's 56-byte
     * frame (ADD #-56,R15 at 0x0c902158); without them, 2 of
     * discharge_lamp_spread_pieces's 266 words differ */
    Uint8 unk[12];

    for (i = 0; i < PIECES; i++) {
        NJS_POINT3 point;

        x = (*object)->pos[0];
        y = (*object)->pos[1] - HEAD_Y;
        z = (*object)->pos[2];
        length = njSqrt((x * x) + (y * y) + (z * z));
        point.x = x + (distance * (x / length));
        point.y = y + (distance * (y / length)) + HEAD_Y;
        point.z = z + (distance * (z / length));
        njCalcPoint(NULL, &point, &piece->pos);
        piece->ang.y = work->ang.y;
        piece->ang.x = state->tilt_x;
        piece->ang.z = state->tilt_z;
        piece++;
        object++;
    }
}

/* give each piece a random spin, up to 8192 a step either way about each
 * axis. its speed is 1 up and 0.9 out from the lamp's upright axis. the
 * current matrix turns the speed */
static void discharge_lamp_scatter_pieces(struct task_work *work)
{
    struct discharge_lamp_state *state = VARS(work)->state;
    NJS_OBJECT **object = discharge_lamp_pieces;
    struct discharge_lamp_piece *piece = state->pieces;
    NJS_VECTOR speed;
    Float x;
    Float z;
    Float length;
    int i;

    for (i = 0; i < PIECES; i++) {
        x = (*object)->pos[0];
        z = (*object)->pos[2];
        piece->spin.x = ((rand() * (1.0F / 16384.0F)) - 1.0F) * 8192.0F;
        piece->spin.y = ((rand() * (1.0F / 16384.0F)) - 1.0F) * 8192.0F;
        piece->spin.z = ((rand() * (1.0F / 16384.0F)) - 1.0F) * 8192.0F;
        length = njSqrt((x * x) + (z * z));
        speed.x = (x / length) * 0.9F;
        speed.y = 1.0F;
        speed.z = (z / length) * 0.9F;
        njCalcVector(NULL, &speed, &piece->speed);
        piece++;
        object++;
    }
}
