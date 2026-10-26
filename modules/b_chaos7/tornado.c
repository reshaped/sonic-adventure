/* Perfect Chaos's tornadoes, which move out from where he stands */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/boss.h"
#include "b_chaos7/chaos7_textures.h"
#include "b_chaos7/tornado.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "system/frame.h"
#include "system/task.h"

extern NJS_OBJECT tornado_object;
extern NJS_OBJECT tornado_funnel_0_object;
extern NJS_OBJECT tornado_funnel_1_object;
extern NJS_OBJECT tornado_funnel_2_object;

/* a tornado's record, at task->extra */
struct tornado {
    NJS_POINT3 origin; /* work->pos at its first step */
    Float distance; /* how far along its path from origin */
    Angle sway_phase;
    Float frame; /* the frame of tornado_action it draws */
};

/* a tornado's scale at full size. while it grows, tornado_step's switch adds
 * TORNADO_GROWTH to the scale each step. while it shrinks, the switch takes
 * as much */
#define TORNADO_SIZE 2.0F
#define TORNADO_GROWTH (TORNADO_SIZE / 30.0F)
/* tornado_action's length in frames. the tornado plays it a frame a step,
 * round and round */
#define TORNADO_ACTION_FRAMES 30.0F

/* each step takes ANG_X_STEP from work->ang.x and adds ANG_Z_STEP to
 * work->ang.z. the draw sets funnels 0 and 2's angle about y to work->ang.x,
 * and funnel 1's to work->ang.z. but draw_action poses the funnels by
 * tornado_action's keys alone, and never reads those angles */
#define ANG_X_STEP 0x1000
#define ANG_Z_STEP 0x800

struct tornado_vars {
    Uint8 unk_00[4];
    Sint32 stay_left; /* counts down from tornado_stay */
};

#define VARS(work) ((struct tornado_vars *)&(work)->vars)
#define TORNADO_SCALE(work) ((work)->scl.x)
enum tornado_submode { TORNADO_GROWING, TORNADO_STAYING, TORNADO_SHRINKING };

#pragma section HEAD
static Float tornado_speed = 6.0F; /* units out along its path a step */
/* the steps it spends in TORNADO_STAYING, less 1 */
static Sint32 tornado_stay = 600;
static Float tornado_sway = 120.0F; /* units to either side of its path */
/* how far the sway's phase turns a step */
static Angle tornado_sway_turn = 910;
#pragma section

/* tornado_keys_0 to _2 turn funnels 0 to 2 about y. over the action's 30
 * frames, funnel 0 turns once one way. funnels 1 and 2 turn twice the other
 * way */
static NJS_MKEY_A tornado_keys_0[30] = {
#include "b_chaos7/tornado/tornado_keys_0.inc"
};
static NJS_MKEY_A tornado_keys_1[30] = {
#include "b_chaos7/tornado/tornado_keys_1.inc"
};
static NJS_MKEY_A tornado_keys_2[30] = {
#include "b_chaos7/tornado/tornado_keys_2.inc"
};
static NJS_MDATA3 tornado_mdata[4] = {
#include "b_chaos7/tornado/tornado_mdata.inc"
};
static NJS_MOTION tornado_motion = {
#include "b_chaos7/tornado/tornado_motion.inc"
};
static NJS_ACTION tornado_action = {
#include "b_chaos7/tornado/tornado_action.inc"
};

/* an upright cylinder 75 in radius and 400 tall, centered on the tornado's
 * position. it keeps its size as the tornado's scale grows */
static struct collision_shape tornado_shape[1] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     ATTACK(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     75.0F,
     200.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void tornado_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct tornado *tornado = task->extra;
    Float scale;

    if (frame_step != 0) {
        return;
    }
    /* @hack spelling: each funnel reached through its address, which SHC pools
     * to store ang[1] at 24 past it (MOV.L R3,@(24,R2) at 0x0c9188dc);
     * through a member access, SHC pools ang[1]'s own address, and 3 of 57
     * words and 3 pool words differ */
    (&tornado_funnel_0_object)->ang[1] = work->ang.x;
    (&tornado_funnel_1_object)->ang[1] = work->ang.z;
    (&tornado_funnel_2_object)->ang[1] = work->ang.x;
    njSetTexture(&chaos7_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    scale = TORNADO_SCALE(work);
    njScale(NULL, scale, scale, scale);
    draw_action(&tornado_action, tornado->frame);
    njPopMatrix(1);
}

static void tornado_step(struct task *task)
{
    struct task_work *work = task->work;
    struct tornado *tornado = task->extra;
    Float sway;
    Angle across;
    /* @hack spelling: njSin called through a pointer, which SHC keeps in R11,
     * with across in R10 (MOV.L @(0x0c918b10),R11 at 0x0c918a44); without
     * it, the two trade, as do FR4 and FR5, and 14 of 193 words and 4 pool
     * words differ */
    Float (*sine)(Angle);

    switch (work->submode) {
    case TORNADO_GROWING:
        TORNADO_SCALE(work) += TORNADO_GROWTH;
        if (TORNADO_SCALE(work) >= TORNADO_SIZE) {
            work->submode = TORNADO_STAYING;
        }
        break;
    case TORNADO_STAYING:
        if (VARS(work)->stay_left-- == 0) {
            work->submode = TORNADO_SHRINKING;
        }
        break;
    case TORNADO_SHRINKING:
        TORNADO_SCALE(work) -= TORNADO_GROWTH;
        if (TORNADO_SCALE(work) <= 0.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    /* @bug logic: a growing tornado grows twice as fast, and a shrinking one
     * never shrinks to 0, so the switch never destroys it. the scale gains
     * TORNADO_GROWTH here in every submode, up to TORNADO_SIZE (FADD at
     * 0x0c9189aa). a shrinking one so gains back what the switch took, and
     * loses only what the sums round away */
    TORNADO_SCALE(work) = TORNADO_SCALE(work) + TORNADO_GROWTH < TORNADO_SIZE
                              ? TORNADO_SCALE(work) + TORNADO_GROWTH
                              : TORNADO_SIZE;
    work->ang.x -= ANG_X_STEP;
    work->ang.z += ANG_Z_STEP;
    tornado->distance += tornado_speed;
    work->pos = tornado->origin;
    work->pos.x += tornado->distance * njCos(work->ang.y);
    sine = njSin;
    work->pos.z += tornado->distance * sine(work->ang.y);
    if (work->mode == TORNADO_SWAYS) {
        across = work->ang.y + NJM_DEG_ANG(90);
        tornado->sway_phase += tornado_sway_turn;
        sway = tornado_sway * sine(tornado->sway_phase);
        work->pos.x += sway * njCos(across);
        work->pos.z += sway * sine(across);
    }
    tornado->frame += 1.0F;
    if (tornado->frame >= TORNADO_ACTION_FRAMES) {
        tornado->frame -= TORNADO_ACTION_FRAMES;
    }
    if (work->collision->flags & COLLISION_HIT) {
        if (work->collision->hit->kind == COLLISION_PLAYER) {
            chaos7_knock_player_back();
        }
    }
    collision_register(work);
    tornado_draw(task);
}

void chaos7_tornado_start(struct task *task)
{
    struct task_work *work = task->work;
    struct tornado *tornado;

    if ((task->extra = heap_calloc(1, sizeof(struct tornado))) == NULL) {
        task_destroy(task);
        return;
    }
    tornado = task->extra;
    tornado->origin = work->pos;
    tornado->distance = 0.0F;
    tornado->frame = 0.0F;
    TORNADO_SCALE(work) = 0.0F;
    work->ang.x = work->ang.z = 0;
    VARS(work)->stay_left = tornado_stay;
    task->step = tornado_step;
    task->draw = tornado_draw;
    /* no funnel has NJD_EVAL_UNIT_ANG to clear. draw_action, the tornado's
     * only draw of them, doesn't read it */
    /* @hack spelling: each funnel reached through its address, which loads the
     * 1 collision_init takes twice into R0 between the second clear and the
     * third (MOV #1,R0 at 0x0c918ba6); without it, 15 of 68 words differ */
    (&tornado_funnel_0_object)->evalflags &= ~NJD_EVAL_UNIT_ANG;
    (&tornado_funnel_1_object)->evalflags &= ~NJD_EVAL_UNIT_ANG;
    (&tornado_funnel_2_object)->evalflags &= ~NJD_EVAL_UNIT_ANG;
    collision_init(task, tornado_shape, COLLISION_OBJECT, 1, 1);
}
