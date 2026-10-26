/* O DRUM drum that bursts into pieces and chips when an attack hurts it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg10/debris.h"
#include "stg10/drum.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the drum in pieces: each child is a piece, at its offset from the drum */
extern NJS_OBJECT final_drum_pieces_object;

#define SE_DRUM_BREAK 290

/* a burst's frames, bomb01 to bomb16 of bomb_pool's texlist */
#define BURST_FRAMES 16
#define BURST_RATE 0.4F /* the frames a burst advances each step */
#define BURST_KEPT 0.95F /* the share of its speed a burst keeps each step */
#define BURSTS 8
#define CHIPS 20
/* the landing that stops a piece or a chip. work->tick counts its landings */
#define LANDINGS 4
/* the share of its turns a piece or a chip keeps at each bounce */
#define SPIN_KEPT 0.6F

enum final_drum_mode { DRUM_INIT, DRUM_WAIT, DRUM_BROKEN };
enum final_drum_piece_mode { PIECE_INIT, PIECE_FLY, PIECE_DONE };
enum final_drum_chip_mode { CHIP_INIT, CHIP_FLY, CHIP_DONE };

struct final_drum_piece_vars {
    NJS_MODEL *model;
};

#define PIECE(work) ((struct final_drum_piece_vars *)&(work)->vars)

static void final_drum_destroy(struct task *task);
static void final_drum_draw(struct task *task);
static void final_drum_draw_at(struct task_work *work);
static void final_drum_piece_draw(struct task *task);
static void final_drum_piece_step(struct task *task);
static void final_drum_break(struct task *task);
static void final_drum_burst_step(struct particle *particle,
                                  struct particle_pool *pool);
static void final_drum_burst(struct task_work *work);
static void final_drum_dust_create(struct dust_params *params);
static void final_drum_chip_step(struct task *task);
static void final_drum_chip_draw(struct task *task);

/* clang-format off */
static struct collision_shape final_drum_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 10.0F, 0.0F}, 5.0F, 10.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */
/* the dust a piece raises between its first landing and its second. the
 * piece step sets pos and scale here before each puff */
static struct dust_params final_drum_puff = {
    1.0F,
    0.038F,
    0.2F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {2.0F, 1.0F, 1.0F, 1.0F},
};
/* a sphere nothing reads, where final_drum_shape is a cylinder */
/* clang-format off */
static struct collision_shape dat_0cb00f98 = {
    SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
    PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 10.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */
/* the ten dust frames, blended so that a puff only darkens what lies behind
 * it */
static struct particle_anim final_drum_dust_anim = {
    &dust_texlist, dust_frames, 10, NJD_COLOR_BLENDING_ZERO,
    NJD_COLOR_BLENDING_INVSRCALPHA};

struct particle_pool_spec final_drum_dust_pool = {&final_drum_dust_anim, 0,
                                                  NULL, NULL};

/* the drum stands until an attack hurts it. it then bursts into BURSTS bomb
 * puffs, its pieces and CHIPS chips. its placement makes no more drums */
void final_drum_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DRUM_INIT:
        task->destructor = final_drum_destroy;
        task->draw = final_drum_draw;
        if (!placement_editing(task)) {
            collision_init(task, &final_drum_shape, COLLISION_OBJECT, 1, 1);
        }
        work->mode = DRUM_WAIT;
        break;
    case DRUM_WAIT:
        if (work->flags & COLLISION_DAMAGED) {
            sound_play(SE_DRUM_BREAK, 0, 0, 0);
            for (i = 0; i < BURSTS; ++i) {
                final_drum_burst(work);
            }
            final_drum_break(task);
            placement_stop(task);
            work->mode = DRUM_BROKEN;
        }
        break;
    case DRUM_BROKEN:
        /* task_destroy_children runs the pieces and chips from here. it
         * would make the task destroy itself once none is left, but none
         * ends. the draw stays, so the paused game shows the drum whole among
         * its pieces */
        task->step = task_destroy_children;
        return;
    }
    task_step_children(task);
    final_drum_draw_at(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void final_drum_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void final_drum_draw(struct task *task)
{
    final_drum_draw_at(task->work);
}

static void final_drum_draw_at(struct task_work *work)
{
    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_tree(&final_drum_object);
        njPopMatrix(1);
    }
}

static void final_drum_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_model(PIECE(work)->model);
        njPopMatrix(1);
    }
}

/* a piece flies, turning about z and x. each landing before the last bounces
 * it and keeps SPIN_KEPT of each turn, and landing LANDINGS stops it */
static void final_drum_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case PIECE_INIT:
        debris_set_body(task, NULL);
        task->draw = final_drum_piece_draw;
        work->mode = PIECE_FLY;
        break;
    case PIECE_FLY:
        if (work->flags & DEBRIS_LANDED) {
            /* @hack redundant: an else after the arm that breaks, which loads
             * debris_bounce's address into R3 (0x0c90b890); without it, R2
             * takes it and 2 of 105 words differ */
            if (++work->tick == LANDINGS) {
                motion->spin.z = 0;
                motion->spin.x = 0;
                work->mode = PIECE_DONE;
                break;
            } else {
                debris_bounce(work, motion);
                motion->spin.x = (Angle)(motion->spin.x * SPIN_KEPT);
                motion->spin.z = (Angle)(motion->spin.z * SPIN_KEPT);
            }
        }
        debris_fall(work, motion);
        debris_move(work, motion);
        work->ang.x += motion->spin.x;
        work->ang.z += motion->spin.z;
        /* @bug logic: a piece's timer stays at the 0 task_create gives
         * it, so the test of its low two bits always holds (TST #3,R0 at
         * 0x0c90b8e4). the piece puffs every step from its first landing to
         * its second */
        if (work->tick == 1 && !(TASK_TIMER(work) & 3)) {
            final_drum_puff.pos = work->pos;
            final_drum_puff.scale = 2.0F;
            final_drum_dust_create(&final_drum_puff);
        }
        break;
    case PIECE_DONE:
        break;
    }
    debris_land(work, motion);
    final_drum_piece_draw(task);
}

/* throw a piece for each child of final_drum_pieces_object. each starts at
 * its child's offset about the drum and flies outward along it. then throw
 * CHIPS chips from the drum's position at random headings */
static void final_drum_break(struct task *task)
{
    /* @hack spelling: task copied to parent, which keeps njCos's address in
     * R8 and njSin's in R9 (0x0c90ba82, 0x0c90ba7e), and the chips' floats
     * where the image's frame keeps them (0x0c90bb44 to 0x0c90bf20); without
     * it, the two addresses trade, the floats move and 34 of 748 words
     * differ */
    struct task *parent = task;
    NJS_OBJECT *root = &final_drum_pieces_object;
    struct task_work *work = parent->work;
    NJS_OBJECT *node;
    NJS_OBJECT *chip_root;
    int i;

    for (node = root->child; node != NULL; node = node->sibling) {
        struct task *child = task_create_child(TASK_WORK | TASK_MOTION,
                                               final_drum_piece_step, parent);
        struct task_work *piece = child->work;
        struct debris_motion *motion = (struct debris_motion *)child->motion;
        Float *from = node->pos;
        Float *position;
        Float *point;
        Float *speed;
        Float x;
        Float y;
        Float z;
        Float px;
        Float py;
        Float pz;
        Float scale;
        Float length_squared;
        Float fling;
        Float saved;

        /* @hack spelling: a cursor over the drum's position, for the image's
         * post-increment loads (0x0c90b9b6 to 0x0c90b9c0); through the
         * members, with the sums apart, 630 of 748 words differ */
        position = VECTOR_FLOATS(&work->pos);
        x = *from++;
        y = *from++;
        z = *from;
        /* @hack spelling: the cursor stepped on lines of its own, which keeps
         * the chips' floats where the image's frame keeps them (0x0c90bb44 to
         * 0x0c90bf20); stepped in its loads, as *position++, 26 of 748 words
         * differ */
        px = *position;
        position++;
        py = *position;
        position++;
        pz = *position;
        px += x;
        py += y;
        pz += z;
        /* @hack redundant: a never-run test of px, which keeps px in FR4, py
         * in FR5 and pz in FR6 (0x0c90b9b6 to 0x0c90b9ca); without it, the
         * three rotate and 18 of 748 words differ */
        if (0) {
            if (px) {
            }
        }
        /* @hack spelling: px copied to saved, which keeps x in FR14 and z in
         * FR12 (0x0c90b9a8, 0x0c90b9b8); without it, the two trade, the
         * function runs 4 bytes short and 605 of its 748 words differ */
        saved = px;
        /* @hack spelling: a cursor from the end of the piece's position, for
         * the image's pre-decrement stores (0x0c90b9c8 to 0x0c90b9cc); through
         * the members, the function runs 4 bytes long and 704 of 748 words
         * differ */
        point = VECTOR_FLOATS(&piece->pos) + 3;
        /* @hack redundant: a never-run test of point, which keeps py in FR5
         * and pz in FR6 (0x0c90b9ba to 0x0c90b9ca), and the chips' motion at
         * sp+4 and speed cursor at sp+8 (0x0c90ba9e, 0x0c90bb4e); without it,
         * each pair trades and 29 of 748 words differ */
        if (0) {
            if (point) {
            }
        }
        *--point = pz;
        *--point = py;
        *--point = saved;
        length_squared = SQUARE(x) + SQUARE(y) + SQUARE(z);
        scale = _builtin_fsrra(length_squared);
        x *= scale;
        y *= scale;
        z *= scale;
        fling = RANDOM(4.0F, 7.0F);
        /* @hack spelling: a cursor from the end of the piece's speed, for the
         * image's pre-decrement stores (0x0c90ba1e to 0x0c90ba24); through
         * the members, 578 of 748 words differ */
        /* @hack spelling: the cursor set and moved to the speed's end apart,
         * which keeps the chips' motion at sp+4 and speed cursor at sp+8 in
         * the third of their loop's four unrolled copies (0x0c90bd52,
         * 0x0c90bd8e); in one statement, the two trade and 11 of 748 words
         * differ */
        speed = VECTOR_FLOATS(&motion->speed);
        speed += 3;
        x *= fling;
        y *= fling * 0.6F;
        z *= fling;
        *--speed = z;
        *--speed = y;
        *--speed = x;
        piece->ang.y =
            (Angle)(atan2f(x, z) * ANGLE_PER_RADIAN) + NJM_DEG_ANG(90);
        PIECE(piece)->model = node->model;
        motion->spin.x = (Angle)(rand() * (4096.0F * RAND_SCALE)) - 2048;
        motion->spin.z = (Angle)(rand() * (4096.0F * RAND_SCALE)) - 2048;
        motion->ground = work->pos.y + 2.0F;
    }
    root = &final_drum_object;
    /* @hack spelling: root copied to chip_root, which keeps x in FR14 and z
     * in FR12 in the pieces' loop (0x0c90b9a8, 0x0c90b9b8); without it, the
     * two trade, the function runs 4 bytes short and 606 of its 748 words
     * differ */
    chip_root = root;
    for (i = CHIPS; i > 0; i--) {
        struct task *child = task_create_child(TASK_WORK | TASK_MOTION,
                                               final_drum_chip_step, parent);
        struct task_work *chip = child->work;
        struct debris_motion *motion = (struct debris_motion *)child->motion;
        Float *speed;
        Float x;
        Float y;
        Float z;
        Float fling;
        Angle ang;

        debris_pick_chip(chip, chip_root->model);
        chip->pos = work->pos;
        ang = RANDOM_ANGLE();
        fling = RANDOM(2.0F, 4.0F);
        /* @hack spelling: a cursor from the end of the chip's speed, which the
         * image keeps at sp+8 and steps back for each store (0x0c90bb4e to
         * 0x0c90bba0); through the members, the function runs 60 bytes short
         * and 605 of 748 words differ */
        /* @hack spelling: the cursor set before the floats and moved to the end
         * after them, which keeps it at sp+8 (0x0c90bb4e, 0x0c90bb7c) and py
         * in FR5 and pz in FR6 in the pieces' loop (0x0c90b9ba to 0x0c90b9ca);
         * moved to the end as it is set, the function runs 16 bytes short and
         * 555 of 748 words differ */
        speed = VECTOR_FLOATS(&motion->speed);
        x = fling * njCos(ang);
        y = RANDOM(1.0F, 2.5F);
        z = -(fling * njSin(ang));
        speed += 3;
        *--speed = z;
        *--speed = y;
        *--speed = x;
        chip->ang.x = RANDOM_ANGLE();
        chip->ang.y = ang;
        chip->ang.z = RANDOM_ANGLE();
        chip->scl.x = RANDOM(0.5F, 1.5F);
        motion->spin.z = (Angle)(rand() * (4096.0F * RAND_SCALE)) - 2048;
        motion->ground = work->pos.y;
        motion->weight = 1.0F;
    }
}

/* a burst plays its BURST_FRAMES frames once, BURST_RATE a step, and keeps
 * BURST_KEPT of its speed each step */
static void final_drum_burst_step(struct particle *particle,
                                  struct particle_pool *pool)
{
    Float *speed;
    Float *pos;
    Float x;
    Float y;
    Float z;
    Float px;
    Float py;
    Float pz;

    particle->frame = floorf(particle->time);
    if (particle->frame >= BURST_FRAMES) {
        particle->frame = BURST_FRAMES - 1;
        particle_kill(particle);
        return;
    }
    particle->time += BURST_RATE;
    /* @hack spelling: cursors over the speed's and the position's floats, for
     * the image's post-increment loads and pre-decrement stores (0x0c90c00e to
     * 0x0c90c032); through the members, the function runs 16 bytes long and
     * 34 of 55 words differ */
    pos = VECTOR_FLOATS(&particle->pos);
    speed = VECTOR_FLOATS(&particle->speed);
    /* @hack spelling: the cursors stepped on lines of their own, which keeps
     * BURST_KEPT in FR6 and the position in FR8 to FR10 (0x0c90c010 to
     * 0x0c90c032); stepped in their loads and stores, as *speed++ and
     * *--pos, 15 of 55 words differ */
    x = *speed;
    speed++;
    y = *speed;
    speed++;
    z = *speed;
    speed++;
    px = *pos;
    pos++;
    py = *pos;
    pos++;
    pz = *pos;
    pos++;
    x *= BURST_KEPT;
    y *= BURST_KEPT;
    z *= BURST_KEPT;
    px += x;
    speed--;
    *speed = z;
    speed--;
    /* @hack spelling: y stored through speed inside py's sum, which keeps
     * BURST_KEPT in FR6 and y in FR7 (0x0c90c010, 0x0c90c012); as two
     * statements, the two trade and 7 of 55 words differ */
    py += (*speed = y);
    pz += z;
    speed--;
    *speed = x;
    pos--;
    *pos = pz;
    pos--;
    *pos = py;
    pos--;
    *pos = px;
    /* @hack redundant: a never-run test of z, which keeps the speed cursor in
     * R4 and the position cursor in R5 (0x0c90bffc to 0x0c90c004); without
     * it, the two trade and 16 of 55 words differ */
    if (0) {
        if (z) {
        }
    }
}

/* a bomb puff at the drum, thrown up and out at random */
static void final_drum_burst(struct task_work *work)
{
    struct particle *particle =
        particle_alloc(bomb_pool.pool, final_drum_burst_step);

    if (particle != NULL) {
        particle->pos = work->pos;
        particle->speed.x = RANDOM(-0.5F, 0.5F) * 4.0F;
        particle->speed.y = RANDOM(1.0F, 3.0F);
        particle->speed.z = RANDOM(-0.5F, 0.5F) * 4.0F;
        particle->scale = RANDOM(0.3F, 0.4F);
        particle->time = 0.0F;
        particle->ang = RANDOM_ANGLE();
        particle->color.a = 0.8F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
    }
}

/* dust_create_from, with the puff from final_drum_dust_pool */
static void final_drum_dust_create(struct dust_params *params)
{
    struct particle_pool *pool = final_drum_dust_pool.pool;
    struct particle *particle = particle_alloc(pool, dust_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale * DUST_SIZE;
        particle->time = rand() * RAND_SCALE * 6.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color = params->color;
        DUST(particle)->params = params;
    }
}

/* a chip flies, turning about z. each landing before the last bounces it and
 * keeps SPIN_KEPT of its turn, and landing LANDINGS stops it */
static void final_drum_chip_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    switch (work->mode) {
    case CHIP_INIT:
        debris_set_body(task, NULL);
        task->draw = final_drum_chip_draw;
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
    final_drum_chip_draw(task);
}

/* draw a chip at the scale in work->scl.x */
static void final_drum_chip_draw(struct task *task)
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
