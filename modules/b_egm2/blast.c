/* the stage's blasts: a ball of fire that swells, swells on ever slower,
 * and fades. beams swing out of it for a while, with puffs about it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm2/baku_textures.h"
#include "b_egm2/blast.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/beam.h"
#include "effect/bomb.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: one NJS_OBJECT declared as an array, under which SHC pools
 * its address and reads model at @(4,R1) (MOV.L @(4,R1),R3 at
 * 0x0c907464); declared as one NJS_OBJECT, it pools model's own address,
 * and summed over blast_grow and blast_draw, 5 of 192 words differ */
extern NJS_OBJECT blast_ball_object[1];

#define HALF_TURN 0x8000

/* the blast's record, which its task's extra points to */
struct blast {
    Sint32 steps; /* the steps run, this one included */
    NJS_POINT3 pos; /* params.pos points here with BLAST_COPY_POS */
    /* how far beam_phase turns a step: HALF_TURN over beam_end -
     * beam_start steps */
    Angle beam_turn;
    Angle beam_phase;
    Float beam_reach; /* this step's beam length */
    Float size; /* the ball's radius */
    Float growth; /* the size the ball adds a step */
    Float alpha;
    struct blast_params params;
};

/* the ball's model is a dome of radius about 63. drawn at size times
 * BALL_SCALE, its radius is about size. blast_draw draws it twice, the
 * second time upside down, to close the ball beneath */
#define BALL_SCALE (1.0F / 63.0F)
/* the ball cycles through egm2_baku_texlist's BALL_TEXTURES textures, a
 * texture every second step */
#define BALL_TEXTURES 8

/* from grow_end, the growth keeps GROWTH_DRAG of itself each step. after
 * fade_start, the size keeps FADE_SHRINK of itself each step */
#define GROWTH_DRAG 0.95F
#define FADE_SHRINK 0.995F

/* while the beams swing, a puff bursts every PUFF_EVERY steps. its size is
 * PUFF_SMALLEST plus the sine of beam_phase, so it swells and shrinks with
 * the beams. it lands at random in a cube about the center. the cube's side
 * is PUFF_SPREAD times the puff's size plus PUFF_MARGIN. bomb_create gets
 * PUFF_SCALE times the puff's size */
#define PUFF_EVERY 4
#define PUFF_SMALLEST 0.1F
#define PUFF_SPREAD 15.0F
#define PUFF_MARGIN 5.0F
#define PUFF_SCALE 0.45F

/* the ball's sphere, whose radius follows the ball's size until grow_end */
static struct collision_shape blast_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_PLAYERS | SHAPE_ATTACK,
     PUSH_PLAYERS,
     {0.0F, 0.0F, 0.0F},
     0.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};
/* blast_beams sets length and max_speed before each beam */
static struct beam_params blast_beam_params = {
    20, {0.3F, 2.0F}, {0xffffffff, 0x00ffffff}, 0x5b, 0.0F, 1.3F,
};

/* unread */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const NJS_POINT3 dat_0c908b78 = {0.0F, 0.0F, 1.0F};

static void blast_run(struct task *task);
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
    Float size = njSin(blast->beam_phase) + PUFF_SMALLEST;
    Float spread = (size * PUFF_SPREAD) + PUFF_MARGIN;
    NJS_POINT3 pos;

    pos.x = params->pos->x + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    pos.y = params->pos->y + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    pos.z = params->pos->z + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    bomb_create(&pos, size * PUFF_SCALE);
}

/* update the ball from grow_start on: its size and sphere, its texture,
 * and its alpha */
static void blast_grow(struct task *task)
{
    struct task_work *work = task->work;
    struct blast *blast = task->extra;
    struct blast_params *params = &blast->params;
    Float was;

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
        /* @hack redundant: the else's empty test of steps % PUFF_EVERY, whose
         * remainder SHC still calls _modls for and tests (JSR at 0x0c90745c,
         * TST R0,R0 at 0x0c907460); without it, 40 of 100 words differ */
        if (blast->steps > params->fade_start) {
            blast->size *= FADE_SHRINK;
        } else if (blast->steps % PUFF_EVERY == 0) {
        }
        blast_ball_object->model->mats->attr_texId =
            (blast->steps >> 1) % BALL_TEXTURES;
        if (blast->steps > params->fade_start) {
            blast->alpha = 1.0F - ((Float)(blast->steps - params->fade_start) /
                                   (params->end - params->fade_start));
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
    }
    if (blast->steps > params->beam_start && blast->steps < params->beam_end) {
        blast_beams(task);
        if (blast->steps % PUFF_EVERY == 0) {
            blast_puff(task);
        }
    }
    if (params->flags & BLAST_HURTS) {
        VECTOR_COPY(&work->pos, params->pos);
        collision_register(work);
    }
    blast_grow(task);
    blast_draw(task);
}

static void blast_draw(struct task *task)
{
    struct blast *blast = task->extra;
    struct blast_params *params = &blast->params;
    Float scale;

    if (frame_step != 0) {
        return;
    }
    njSetTexture(&egm2_baku_texlist);
    scale = blast->size * BALL_SCALE;
    njPushMatrix(NULL);
    draw_control_save();
    draw_attr_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_set_material(0.0F, blast->alpha, blast->alpha, blast->alpha);
    draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
    njTranslateV(NULL, params->pos);
    njScale(NULL, scale, scale, scale);
    njDrawObject(blast_ball_object);
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    njDrawObject(blast_ball_object);
    draw_attr_restore();
    draw_control_restore();
    njPopMatrix(1);
}

void egg_walker_blast_create(struct blast_params *params)
{
    struct task *task;
    struct blast *blast;

    /* @hack spelling: the task stored as it's tested, which tests the result
     * in R0 and copies it to R13 in the branch's slot (TST R0,R0 at
     * 0x0c907634, MOV R0,R13 at 0x0c907638); as two statements, SHC tests
     * R13 and 3 of 103 words differ */
    if ((task = task_create(TASK_WORK, 4, blast_run)) != NULL) {
        blast = heap_alloc(sizeof(struct blast));
        if (blast != NULL) {
            collision_init(task, blast_shape, COLLISION_ENEMY, 1, 1);
            task->extra = blast;
            blast->steps = 0;
            blast->beam_turn =
                HALF_TURN / (params->beam_end - params->beam_start);
            blast->beam_phase = 0;
            blast->beam_reach = 0.0F;
            blast->size = 0.0F;
            blast->alpha = 1.0F;
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
