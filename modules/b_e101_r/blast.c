/* the blasts E-101 mkII's beam sets off. a blast's ball grows to its size,
 * swells on as its growth slows, then shrinks as it fades. it is drawn
 * twice, the second time mirrored in y about its center. puffs burst round
 * the ball until it fades. for a while, beams swing out from the center,
 * and more puffs burst about it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_e101_r/blast.h"
#include "b_e101_r/files.h"
#include "b_egm2/blast.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/beam.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* byte offset of the ball's object in E101R.NB */
#define BALL_OBJECT 0xaacc

/* a blast's record, in task->extra. the Egg Walker's blasts keep the same
 * layout */
struct blast {
    Sint32 steps; /* the steps the blast has run, this one included */
    NJS_POINT3 pos; /* params.pos points here with BLAST_COPY_POS */
    Angle beam_turn; /* beam_phase's turn each step */
    Angle beam_phase;
    Float beam_reach;
    Float size;
    Float growth; /* the size the ball gains each step */
    Float brightness; /* the gray the ball is drawn in */
    struct blast_params params;
};

/* the scale of the ball's model for each unit of its size */
#define BALL_SCALE (1.0F / 63.0F)
/* the ball's texture shows e101_r_texlist's first BALL_FRAMES textures in
 * turn, each for 4 steps */
#define BALL_FRAMES 7
/* the ball's brightness until it fades */
#define BALL_BRIGHTNESS 0.4F
/* how far the collision record's reach runs past params->size */
#define REACH_MARGIN 40.0F

/* from grow_end, the ball keeps this share of its growth each step */
#define GROWTH_DRAG 0.95F
/* past fade_start, the ball keeps this share of its size each step */
#define FADE_SHRINK 0.995F

/* while the beams swing, a puff bursts about the center every PUFF_EVERY
 * steps. its size is the sine of beam_phase plus PUFF_SIZE. it lies at
 * random in a cube about the center. the cube's side is PUFF_SPREAD times
 * the puff's size plus PUFF_NEAR. the puff bursts at PUFF_SCALE times its
 * size */
#define PUFF_EVERY 4
#define PUFF_SIZE 0.1F
#define PUFF_SPREAD 15.0F
#define PUFF_NEAR 5.0F
#define PUFF_SCALE 0.45F
/* from grow_start until the ball fades, a puff at SCATTER_SCALE also bursts
 * every PUFF_EVERY steps. it lies on the ball's surface, in a random
 * direction: the ball's size is its radius */
#define SCATTER_SCALE 0.5F

/* a scattered puff's offset from its center, before blast_scatter turns it
 * and scales it to the radius */
static const NJS_POINT3 blast_scatter_dir = {0.0F, 0.0F, 1.0F};

static struct collision_shape blast_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_GUARD | SHAPE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     0.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};
/* the beams' params. beam_create copies them, so blast_beams sets length
 * and max_speed in place before each call */
static struct beam_params blast_beam_params = {
    20, {0.3F, 2.0F}, {0xffffffff, 0x00ffffff}, 0x5b, 0.0F, 1.3F,
};

static void blast_draw(struct task *task);

static void blast_beams(struct task *task)
{
    struct blast *blast = task->extra;
    struct blast_params *params = &blast->params;

    blast->beam_phase += blast->beam_turn;
    blast->beam_reach = njSin(blast->beam_phase) * params->beam_length;
    if (blast->beam_reach > 0.0F) {
        blast_beam_params.length = blast->beam_reach;
        blast_beam_params.max_speed = params->beam_speed;
        beam_create(params->pos, &blast_beam_params);
    }
}

static void blast_puff(struct task *task)
{
    struct blast *blast = task->extra;
    struct blast_params *params = &blast->params;
    Float size = njSin(blast->beam_phase) + PUFF_SIZE;
    Float spread = (size * PUFF_SPREAD) + PUFF_NEAR;
    NJS_POINT3 pos;

    pos.x = params->pos->x + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    pos.y = params->pos->y + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    pos.z = params->pos->z + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    bomb_create(&pos, size * PUFF_SCALE);
}

/* burst a puff at scale, radius from pos in a random direction */
static void blast_scatter(NJS_POINT3 *pos, Float radius, Float scale)
{
    NJS_POINT3 offset;

    njPushMatrix(&_unit_matrix_);
    /* TURN reads its angle once to test it against 0, and again to turn by
     * it. each turn so calls rand() once for the test. unless that gives 0,
     * it calls rand() again for the angle it turns by */
    TURN(njRotateX, (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE));
    TURN(njRotateY, (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE));
    njScale(NULL, radius, radius, radius);
    njCalcPoint(NULL, (NJS_POINT3 *)&blast_scatter_dir, &offset);
    njPopMatrix(1);
    VECTOR_SUM(&offset, &offset, pos);
    bomb_create(&offset, scale);
}

/* the ball's step, from params->grow_start on. until params->grow_end, the
 * ball grows by an even share of params->size each step. its shape's
 * radius grows with it. it then swells on by its last growth, which
 * GROWTH_DRAG slows. past params->fade_start, it shrinks to FADE_SHRINK of
 * its size each step. its brightness also falls from BALL_BRIGHTNESS to 0
 * by params->end. it stops hurting once it begins to fade or touches a
 * player */
static void blast_grow(struct task *task)
{
    struct blast *blast = task->extra;
    struct blast_params *params = &blast->params;
    struct task_work *work = task->work;
    Float was;
    Float fade; /* 1 at fade_start, falling to 0 at end */

    if (blast->steps >= params->grow_start) {
        if (blast->steps < params->grow_end) {
            was = blast->size;
            blast->size +=
                params->size / (params->grow_end - params->grow_start);
            blast->growth = blast->size - was;
            work->collision->shapes->radius = blast->size;
        } else {
            blast->size += blast->growth;
            blast->growth *= GROWTH_DRAG;
        }
        if (blast->steps > params->fade_start) {
            params->flags &= ~BLAST_HURTS;
            blast->size *= FADE_SHRINK;
        } else if (blast->steps % PUFF_EVERY == 0) {
            blast_scatter(params->pos, blast->size, SCATTER_SCALE);
        }
        if (work->collision->flags & COLLISION_HIT) {
            if (work->collision->hit->kind == COLLISION_PLAYER) {
                params->flags &= ~BLAST_HURTS;
            }
        }
        ((NJS_OBJECT *)(e101_r_models + BALL_OBJECT))->model->mats->attr_texId =
            (blast->steps >> 2) % BALL_FRAMES;
        if (blast->steps > params->fade_start) {
            fade = 1.0F - ((Float)(blast->steps - params->fade_start) /
                           (params->end - params->fade_start));
            blast->brightness = fade * BALL_BRIGHTNESS;
        }
    }
}

static void blast_run(struct task *task)
{
    struct blast *blast = task->extra;
    struct task_work *work = task->work;
    struct blast_params *params = &blast->params;

    ++blast->steps;
    if (blast->steps >= params->beam_end && blast->steps >= params->end) {
        task_kill(task);
        return;
    }
    if (blast->steps > params->beam_start && blast->steps < params->beam_end) {
        blast_beams(task);
        if (blast->steps % PUFF_EVERY == 0) {
            blast_puff(task);
        }
    }
    blast_grow(task);
    blast_draw(task);
    if (params->flags & BLAST_HURTS) {
        VECTOR_COPY(&work->pos, params->pos);
        collision_register(work);
    }
}

/* draw the ball unlit, in the gray of its brightness, and again mirrored
 * in y about its center. it draws only on a frame's first step */
static void blast_draw(struct task *task)
{
    struct blast *blast = task->extra;
    struct blast_params *params = &blast->params;
    Float scale;

    njSetTexture(e101_r_texlist);
    scale = blast->size * BALL_SCALE;
    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    draw_control_save();
    draw_attr_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_set_material(1.0F, blast->brightness, blast->brightness,
                      blast->brightness);
    njSetConstantAttr(0xffffffff, NJD_FLAG_IGNORE_LIGHT);
    njTranslateV(NULL, params->pos);
    njScale(NULL, scale, scale, scale);
    njDrawObject((NJS_OBJECT *)(e101_r_models + BALL_OBJECT));
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    njDrawObject((NJS_OBJECT *)(e101_r_models + BALL_OBJECT));
    draw_attr_restore();
    draw_control_restore();
    njPopMatrix(1);
}

void e101_mk2_blast_create(struct blast_params *params)
{
    struct task *task;
    struct blast *blast;

    if ((task = task_create(TASK_WORK, 4, blast_run)) != NULL) {
        blast = heap_alloc(sizeof(struct blast));
        if (blast != NULL) {
            collision_init(task, blast_shape, COLLISION_ENEMY, 1, 1);
            task->work->collision->radius = params->size + REACH_MARGIN;
            enemy_alloc_record(task, task->work);
            task->extra = blast;
            blast->steps = 0;
            blast->beam_turn =
                NJM_DEG_ANG(180) / (params->beam_end - params->beam_start);
            blast->beam_phase = 0;
            blast->beam_reach = 0.0F;
            blast->size = 0.0F;
            blast->brightness = BALL_BRIGHTNESS;
            blast->params = *params;
            if (params->flags & BLAST_COPY_POS) {
                blast->pos = *params->pos;
                blast->params.pos = &blast->pos;
            }
            task->draw = blast_draw;
        } else {
            task_destroy(task);
        }
    }
}
