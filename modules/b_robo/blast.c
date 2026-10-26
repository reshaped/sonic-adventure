/* a blast like the Egg Walker's, which nothing makes. with BLAST_HURTS,
 * its ball hurts players until it first meets one or starts to fade */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm2/blast.h"
#include "b_robo/banks.h"
#include "b_robo/blast.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/beam.h"
#include "enemy/enemy.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the ball's object, as an offset into the model bank, and its scale at a
 * size of 1 */
#define BALL_OBJECT 0x12d08
#define BALL_SCALE (1.0F / 63.0F)
/* the ball's textures, which it cycles through, showing each four steps */
#define BALL_TEXTURES 7
/* the ball's alpha until it fades */
#define BALL_ALPHA 0.4F

/* the part of its growth the ball keeps each step once grown. FADE_SHRINK
 * is the part of its size it keeps each step as it fades */
#define GROWTH_DRAG 0.95F
#define FADE_SHRINK 0.995F

/* how far past the ball's full size the blast's collision radius reaches */
#define REACH 40.0F

/* a blast's record, at its task's extra */
struct blast {
    Sint32 steps; /* the steps since the blast's start */
    NJS_POINT3 pos; /* params.pos points here with BLAST_COPY_POS */
    /* the turn beam_phase takes each step: half a turn over the steps from
     * beam_start to beam_end */
    Angle beam_turn;
    Angle beam_phase;
    Float beam_reach;
    Float size;
    Float growth; /* the size the ball last gained in a step */
    Float alpha;
    struct blast_params params;
};

/* the ball's sphere, which hurts players, can't be hurt and can't be
 * pushed */
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
/* each beam lasts 20 steps before it fades. it runs from opaque white at
 * its foot to clear at its tip */
static struct beam_params blast_beam_params = {
    20, {0.3F, 2.0F}, {0xffffffff, 0x00ffffff}, 0x5b, 0.0F, 1.3F,
};

static void blast_draw(struct task *task);

/* move the swing on a step. while the swing's sine is above 0, make a beam
 * from pos, the sine times beam_length long */
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

/* from grow_start, the ball grows evenly to size by grow_end, then on more
 * slowly. past fade_start, it shrinks, fades and stops hurting. it stops
 * hurting too once it meets a player. its texture in the model bank moves
 * on every fourth step */
static void blast_grow(struct task *task)
{
    struct task_work *work = task->work;
    struct blast *blast = task->extra;
    struct blast_params *params = &blast->params;
    Float was;
    Float fade;

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
        }
        if (work->collision->flags & COLLISION_HIT) {
            if (work->collision->hit->kind == COLLISION_PLAYER) {
                params->flags &= ~BLAST_HURTS;
            }
        }
        ((NJS_OBJECT *)(erobo_models + BALL_OBJECT))->model->mats->attr_texId =
            (blast->steps >> 2) % BALL_TEXTURES;
        if (blast->steps > params->fade_start) {
            fade = 1.0F - ((Float)(blast->steps - params->fade_start) /
                           (params->end - params->fade_start));
            blast->alpha = fade * BALL_ALPHA;
        }
    }
}

/* run the blast a step: its beams while they last, then its ball. the
 * blast goes once both are over */
static void blast_run(struct task *task)
{
    struct blast *blast = task->extra;
    struct task_work *work = task->work;
    struct blast_params *params = &blast->params;
    Sint32 steps;

    ++blast->steps;
    steps = blast->steps;
    if (steps >= params->beam_end && steps >= params->end) {
        task_kill(task);
        return;
    }
    if (blast->steps > params->beam_start && blast->steps < params->beam_end) {
        blast_beams(task);
    }
    blast_grow(task);
    blast_draw(task);
    if (params->flags & BLAST_HURTS) {
        VECTOR_COPY(&work->pos, params->pos);
        collision_register(work);
    }
}

/* on a frame's first step, draw the ball, and its mirror image below it */
static void blast_draw(struct task *task)
{
    struct blast *blast = task->extra;
    struct blast_params *params = &blast->params;
    Float scale;

    njSetTexture(erobo_textures);
    scale = blast->size * BALL_SCALE;
    if (frame_step == 0) {
        njPushMatrix(NULL);
        draw_control_save();
        draw_attr_save();
        njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                    NJD_CONTROL_3D_CONSTANT_MATERIAL);
        draw_set_material(1.0F, blast->alpha, blast->alpha, blast->alpha);
        njSetConstantAttr(~0U, NJD_FLAG_IGNORE_LIGHT);
        njTranslateV(NULL, params->pos);
        njScale(NULL, scale, scale, scale);
        njDrawObject((NJS_OBJECT *)(erobo_models + BALL_OBJECT));
        njScale(NULL, 1.0F, -1.0F, 1.0F);
        njDrawObject((NJS_OBJECT *)(erobo_models + BALL_OBJECT));
        draw_attr_restore();
        draw_control_restore();
        njPopMatrix(1);
    }
}

void erobo_blast_create(struct blast_params *params)
{
    struct task *task;
    struct blast *blast;

    if ((task = task_create(TASK_WORK, 4, blast_run)) != NULL) {
        blast = heap_alloc(sizeof(struct blast));
        if (blast != NULL) {
            collision_init(task, blast_shape, COLLISION_ENEMY, 1, 1);
            task->work->collision->radius = params->size + REACH;
            enemy_alloc_record(task, task->work);
            task->extra = blast;
            blast->steps = 0;
            blast->beam_turn = 0x8000 / (params->beam_end - params->beam_start);
            blast->beam_phase = 0;
            blast->beam_reach = 0.0F;
            blast->size = 0.0F;
            blast->alpha = BALL_ALPHA;
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
