/* O ARM WALL wall that an attack breaks into sparking pieces */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/sprite.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "effect/hit.h" /* NOLINT(misc-include-cleaner) */
#include "effect/particle.h"
#include "object/placement.h"
#include "stg10/arm_wall.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT arm_wall_object;
extern NJS_OBJECT arm_wall_pieces_object; /* its children are the pieces */

/* the blasts the break sends out */
#define BLASTS 40

/* a piece ends on the step that finds its timer past PIECE_TIMER_MAX. a
 * blast fades from the step that takes its time past BLAST_HOLD_MAX */
#define PIECE_TIMER_MAX 200
#define BLAST_HOLD_MAX 60.0F

enum arm_wall_mode { WALL_INIT, WALL_WAIT, WALL_BROKEN };
enum arm_wall_piece_mode { PIECE_INIT, PIECE_FLY };
/* a blast's mode, kept in particle->life */
enum arm_wall_blast_mode { BLAST_HOLD, BLAST_FADE };

/* a piece's model, and its spark's frame */
struct arm_wall_piece_vars {
    NJS_MODEL *model;
    Sint32 frame;
};

#define PIECE(work) ((struct arm_wall_piece_vars *)&(work)->vars)

static void arm_wall_destroy(struct task *task);
static void arm_wall_draw(struct task *task);
static void arm_wall_draw_at(struct task_work *work);
static void arm_wall_piece_step(struct task *task);
static void arm_wall_piece_draw(struct task *task);
static void arm_wall_break(struct task *task);
static void arm_wall_draw_spark(struct task_work *work);
static void arm_wall_blast_step(struct particle *particle,
                                struct particle_pool *pool);

/* a box 100 units wide, 100 high and 10 thick, with its top at the wall's
 * position. an attack can hurt it. no group can push it, and it pushes
 * none */
static struct collision_shape arm_wall_shape = {
    SHAPE_RECTANGLE | SHAPE_HURTABLE,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, -50.0F, 0.0F},
    50.0F,
    50.0F,
    5.0F,
    {0},
    0,
    NULL,
};

/* the spark a piece shows, moved to it as it draws */
static NJS_SPRITE arm_wall_spark = {
#include "stg10/arm_wall/arm_wall_spark.inc"
};

static const NJS_ARGB arm_wall_blast_color = {1.0F, 1.0F, 1.0F, 1.0F};

/* an attack that hurts the wall breaks it into its pieces. the step after
 * the break kills the wall with placement_kill, so it never comes back */
void arm_wall_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case WALL_INIT:
        task->destructor = arm_wall_destroy;
        task->draw = arm_wall_draw;
        if (!placement_editing(task)) {
            collision_init(task, &arm_wall_shape, COLLISION_OBJECT_MUTUAL, 1,
                           1);
            work->collision->radius = 100.0F;
        }
        work->mode = WALL_WAIT;
        break;
    case WALL_WAIT:
        if (work->flags & COLLISION_DAMAGED) {
            arm_wall_break(task);
            work->mode = WALL_BROKEN;
        }
        break;
    case WALL_BROKEN:
        placement_kill(task);
        return;
    }
    arm_wall_draw_at(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void arm_wall_destroy(struct task *task)
{
}

static void arm_wall_draw(struct task *task)
{
    arm_wall_draw_at(task->work);
}

static void arm_wall_draw_at(struct task_work *work)
{
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(&arm_wall_object);
    njPopMatrix(1);
}

/* a piece flies PIECE_TIMER_MAX + 2 steps, tumbling, then ends. on four steps
 * in every sixteen it draws its spark, at the next of eight frames */
static void arm_wall_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    Float drag;
    Float *speed;
    Float *pos;
    Float x;
    Float y;
    Float z;
    Float px;
    Float py;
    Float pz;

    switch (work->mode) {
    case PIECE_INIT:
        task->draw = arm_wall_piece_draw;
        work->mode = PIECE_FLY;
        break;
    case PIECE_FLY:
        if (TASK_TIMER(work)++ > PIECE_TIMER_MAX) {
            task_kill(task);
            return;
        }
        break;
    }
    /* @hack spelling: cursors over speed's and pos's floats, for the image's
     * post-increment loads and pre-decrement stores (0x0c90c504 to
     * 0x0c90c532); through the members, the object's code runs 32 bytes long
     * and 51 of 81 words differ */
    speed = VECTOR_FLOATS(&motion->speed);
    x = *speed++;
    y = *speed++;
    z = *speed++;
    pos = VECTOR_FLOATS(&work->pos);
    px = *pos++;
    /* @hack spelling: drag set between the loads of px and py, which loads z
     * ahead of px (0x0c90c512) and stores z ahead of pz's sum (0x0c90c524);
     * set before the loads, 6 of 81 words differ */
    drag = 0.95F;
    py = *pos++;
    pz = *pos++;
    x *= drag;
    y += -0.05F;
    z *= drag;
    px += x;
    py += y;
    pz += z;
    *--speed = z;
    *--speed = y;
    *--speed = x;
    *--pos = pz;
    *--pos = py;
    *--pos = px;
    work->ang.z += motion->phase;
    ++work->tick;
    if ((work->tick & 8) && (work->tick & 1)) {
        /* @hack redundant: five tests of motion that never run, which hold it
         * in R5 (0x0c90c4ae), the speed's cursor in R7 (0x0c90c500) and the
         * tick in R6 (0x0c90c546); without them, 19 of 81 words differ */
        if (0) {
            if (motion) {
            }
            if (motion) {
            }
            if (motion) {
            }
            if (motion) {
            }
            if (motion) {
            }
        }
        ++PIECE(work)->frame;
        PIECE(work)->frame &= 7;
        arm_wall_draw_spark(work);
    }
    arm_wall_piece_draw(task);
}

/* the spin the step keeps in ang.z turns the piece about x */
static void arm_wall_piece_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.z);
    draw_model(PIECE(work)->model);
    njPopMatrix(1);
}

/* throw a piece for each child of the pieces tree. each flies out along z
 * and up or down at random. along x, it flies the way it lies from the wall's
 * position. each tumbles, and their sparks are staggered over eight steps.
 * then send BLASTS blasts out along z from the collision box's center, less
 * those particle_alloc can't give */
static void arm_wall_break(struct task *task)
{
    struct task_work *work;
    NJS_OBJECT *root;
    NJS_OBJECT *node;
    struct task *child;
    struct task_work *piece;
    struct task_motion *motion;
    NJS_POINT3 center;
    int index;
    NJS_ARGB color;
    struct particle *particle;

    root = &arm_wall_pieces_object;
    index = 0;
    work = task->work;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    for (node = root->child; node != NULL; node = node->sibling) {
        Float x;
        Float y;
        Float z;
        Float scale;
        Float squared;
        Float force;
        Float *from;
        Float *to;

        child = task_create(TASK_WORK | TASK_MOTION, 3, arm_wall_piece_step);
        piece = child->work;
        motion = child->motion;
        njCalcPoint(NULL, (NJS_POINT3 *)node->pos, &piece->pos);
        /* @hack spelling: a cursor over node->pos, for the image's
         * post-increment loads (0x0c90c684, 0x0c90c68c); indexed, 475 of 546
         * words differ */
        from = node->pos;
        x = *from++;
        y = *from++;
        z = *from;
        squared = SQUARE(x) + SQUARE(y) + SQUARE(z);
        scale = _builtin_fsrra(squared);
        x *= scale;
        force = RANDOM(1.0F, 3.0F);
        x *= force;
        y = RANDOM(-0.5F, 0.5F) * 2.0F;
        z = RANDOM(3.0F, 9.0F);
        /* @hack spelling: a cursor from the end of motion->speed, for the
         * image's pre-decrement stores (0x0c90c6f4 to 0x0c90c6fe); through the
         * members, 92 of 546 words differ */
        to = VECTOR_FLOATS(&motion->speed) + 3;
        *--to = z;
        *--to = y;
        *--to = x;
        njCalcVector(NULL, &motion->speed, &motion->speed);
        piece->tick = index++ & 7;
        PIECE(piece)->frame = (int)(rand() * (8.0F * RAND_SCALE));
        PIECE(piece)->model = node->model;
        piece->ang.y = work->ang.y;
        motion->phase = -(int)(rand() * (768.0F * RAND_SCALE)) - 256;
    }
    color = arm_wall_blast_color;
    njCalcPoint(NULL, &work->collision->shapes->center, &center);
    for (index = 0; index < BLASTS; index++) {
        particle = particle_alloc(explosion_pool.pool, arm_wall_blast_step);
        if (particle != NULL) {
            particle->frame = rand() * (2.0F * RAND_SCALE);
            particle->pos = center;
            particle->speed.x = RANDOM(-0.5F, 0.5F) * 3.0F;
            particle->speed.y = RANDOM(-0.5F, 0.5F) * 3.0F;
            particle->speed.z = RANDOM(4.0F, 10.0F);
            njCalcVector(NULL, &particle->speed, &particle->speed);
            particle->time = 0.0F;
            particle->life = BLAST_HOLD;
            particle->ang = 0;
            particle->scale = RANDOM(0.2F, 0.4F);
            particle->color = color;
        }
    }
    njPopMatrix(1);
}

static void arm_wall_draw_spark(struct task_work *work)
{
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    arm_wall_spark.p.x = work->pos.x;
    /* @hack type: y and z stored through byte offsets into the sprite, so
     * SHC takes each one's address from the pool, at 0x0c90cc28 to
     * 0x0c90cc30; through the members, 11 of 53 words differ */
    *(Float *)((char *)&arm_wall_spark + sizeof(Float)) = work->pos.y;
    *(Float *)((char *)&arm_wall_spark + (2 * sizeof(Float))) = work->pos.z;
    sprite_set_depth(-32.0F);
    sprite_draw_3d(&arm_wall_spark, PIECE(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR |
                       NJD_SPRITE_ANGLE);
    sprite_set_depth(0.0F);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* a blast holds for BLAST_HOLD_MAX + 1 steps, then fades, and ends once its
 * alpha falls below 0 */
static void arm_wall_blast_step(struct particle *particle,
                                struct particle_pool *pool)
{
    Float *speed;
    Float *pos;
    Float vx;
    Float vy;
    Float vz;
    Float px;
    Float py;
    Float pz;

    switch (particle->life) {
    case BLAST_HOLD:
        particle->time += 1.0F;
        if (particle->time > BLAST_HOLD_MAX) {
            particle->life = BLAST_FADE;
        }
        break;
    case BLAST_FADE:
        particle->color.a -= 0.05F;
        if (particle->color.a < 0.0F) {
            particle_kill(particle);
            return;
        }
        break;
    }
    /* @hack spelling: cursors over speed's and pos's floats, for the image's
     * post-increment loads and pre-decrement stores (0x0c90cbd6 to
     * 0x0c90cc00); through the members, 24 of 60 words differ */
    speed = VECTOR_FLOATS(&particle->speed);
    vx = *speed++;
    vy = *speed++;
    vz = *speed++;
    vz *= 0.99F;
    /* @hack spelling: pos set between vz's product and vx's, which has SHC
     * multiply vy, vz and vx in turn (0x0c90cbe2 to 0x0c90cbe6); set just
     * before its loads, 2 of 60 words differ */
    pos = VECTOR_FLOATS(&particle->pos);
    vx *= 0.99F;
    vy *= 0.99F;
    *--speed = vz;
    *--speed = vy;
    *--speed = vx;
    px = *pos++;
    py = *pos++;
    pz = *pos++;
    pz += vz;
    px += vx;
    py += vy;
    *--pos = pz;
    *--pos = py;
    *--pos = px;
}
