/* debris pieces that tumble and bounce, trailing smoke or flame */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/ground.h"
#include "draw/tree.h"
#include "stg05/debris.h"
#include "stg05/puff.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT debris_0_object;
extern NJS_OBJECT debris_1_object;
extern NJS_OBJECT debris_2_object;
extern NJS_OBJECT debris_3_object;
extern NJS_OBJECT debris_4_object;
extern NJS_OBJECT debris_5_object;

struct debris_vars {
    Uint8 unk_00[4];
    Sint16 life; /* the steps the piece has left */
    Sint8 since_puff; /* the steps since its last puff */
    Sint8 model; /* a copy of its kind's model */
    struct debris_kind *kind;
};

#define VARS(work) ((struct debris_vars *)&(work)->vars)

/* in a piece's work->flags. this file never sets DEBRIS_GONE, but a kind's
 * start or step can */
#define DEBRIS_BOUNCED 0x100 /* it bounced this step */
#define DEBRIS_GONE 0x200 /* its next step kills it */

/* the pieces' models, indexed by a kind's model. the last two are NULL */
static NJS_OBJECT *debris_objects[8] = {
    &debris_0_object, &debris_1_object, &debris_2_object,
    &debris_3_object, &debris_4_object, &debris_5_object,
};

static void debris_start(struct task *task);
static void debris_step(struct task *task);
static void debris_destroy(struct task *task);
static void debris_smoke(struct task_work *work, struct debris_motion *motion);
static void debris_flame(struct task_work *work, struct debris_motion *motion);

/* a random spin of up to limit either way */
static int debris_random_spin(int limit)
{
    return (0.5F - (rand() * RAND_SCALE)) * limit * 2.0F;
}

void debris_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale,
                   struct debris_kind *kind)
{
    struct task *task = task_create(TASK_MOTION | TASK_WORK, 3, debris_start);
    struct task_work *work;
    struct debris_motion *motion;
    Uint8 model;

    if (task != NULL) {
        work = task->work;
        motion = (struct debris_motion *)task->motion;
        model = kind->model;
        VARS(work)->life = kind->life;
        VARS(work)->model = model;
        VARS(work)->kind = kind;
        work->pos = *pos;
        work->scl.x = scale;
        work->scl.y = scale;
        work->scl.z = scale;
        motion->speed = *speed;
        motion->spin.x = debris_random_spin(kind->spin.x);
        motion->spin.y = debris_random_spin(kind->spin.y);
        motion->spin.z = debris_random_spin(kind->spin.z);
        motion->radius = scale * debris_objects[model]->model->r;
    }
}

static void debris_start(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_kind *kind = VARS(work)->kind;

    if (kind->start != NULL) {
        kind->start(work, (struct debris_motion *)task->motion);
    }
    task->step = debris_step;
    task->destructor = debris_destroy;
}

static void debris_step(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_kind *kind = VARS(work)->kind;
    struct debris_motion *motion = (struct debris_motion *)task->motion;
    int flags = kind->flags;
    struct angles slope;
    NJS_POINT3 shadow;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int model = VARS(work)->model;
    Angle ang;
    Float ground;

    if (work->flags & DEBRIS_GONE) {
        task_kill(task);
        return;
    }
    /* @bug logic: the test never finds DEBRIS_FOREVER. life is a Sint16,
     * so it holds the kind's 0xffff as -1, which the CMP/EQ with 0x0000ffff
     * at 0x0c900610 never matches. the piece counts down from -1, through
     * the wrap, and lasts 65535 steps */
    /* NOLINTNEXTLINE(clang-diagnostic-tautological-*) */
    if (VARS(work)->life != DEBRIS_FOREVER) {
        if (VARS(work)->life == 0) {
            task_kill(task);
            return;
        }
        --VARS(work)->life;
    }
    motion->speed.x += -motion->speed.x * 0.03F;
    motion->speed.y += (-motion->speed.y * 0.01F) - 0.1F;
    motion->speed.z += -motion->speed.z * 0.03F;
    VECTOR_ADD(&work->pos, &motion->speed);
    work->ang.x += motion->spin.x;
    work->ang.y += motion->spin.y;
    work->ang.z += motion->spin.z;
    work->flags &= ~DEBRIS_BOUNCED;
    ground =
        ground_below(work->pos.x, work->pos.y + 10.0F, work->pos.z, &slope);
    if ((flags & DEBRIS_BOUNCES) && ground > work->pos.y &&
        motion->speed.y < 0.0F) {
        work->pos.y = ground;
        motion->speed.x *= 0.9F;
        motion->speed.y *= -kind->bounce;
        motion->speed.z *= 0.9F;
        motion->spin.x = motion->spin.x * 0.5F;
        motion->spin.y = motion->spin.y * 0.5F;
        motion->spin.z = motion->spin.z * 0.5F;
        work->flags |= DEBRIS_BOUNCED;
    }
    if (flags & DEBRIS_SHADOW) {
        shadow.x = work->pos.x;
        shadow.y = ground + SHADOW_LIFT;
        shadow.z = work->pos.z;
        shadow_draw_at(&slope, &shadow, work->scl.x);
    }
    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
    draw_tree(debris_objects[model]);
    njPopMatrix(1);
    if (flags & DEBRIS_PUFFS) {
        ++VARS(work)->since_puff;
        if (VARS(work)->since_puff == kind->puff_steps) {
            switch (kind->puff) {
            case DEBRIS_SMOKE:
                debris_smoke(work, motion);
                break;
            case DEBRIS_FLAME:
                debris_flame(work, motion);
                break;
            }
            VARS(work)->since_puff = 0;
        }
    }
    if (kind->step != NULL) {
        kind->step(work, motion);
    }
}

static void debris_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct debris_kind *kind = VARS(work)->kind;

    if (kind->destroy != NULL) {
        kind->destroy(work, (struct debris_motion *)task->motion);
    }
}

/* a smoke puff stays where the piece left it, and grows until its frames
 * run out */
static void debris_smoke_step(struct task *task)
{
    struct mountain_puff *puff = (struct mountain_puff *)task->work;
    int last = mountain_puff_frames() - 1;

    if ((int)puff->frame > last) {
        task_kill(task);
        return;
    }
    puff->frame += puff->rate;
    puff->scale += 0.02F;
}

static void debris_smoke(struct task_work *work, struct debris_motion *motion)
{
    struct task *task = mountain_puff_create(PUFF_SMOKE, debris_smoke_step);
    struct mountain_puff *puff;

    if (task != NULL) {
        puff = (struct mountain_puff *)task->work;
        puff->frame = rand() * (1.0F / 8192.0F);
        puff->ang = RANDOM_ANGLE();
        puff->scale = RANDOM(0.02F, 0.04F) * motion->radius;
        puff->pos = work->pos;
        puff->rate = (rand() * (0.3F * RAND_SCALE)) + 0.5F;
    }
}

/* a flame starts at half the piece's speed. it slows and drifts upward
 * until its frames run out */
static void debris_flame_step(struct task *task)
{
    struct mountain_puff *puff = (struct mountain_puff *)task->work;
    int last = mountain_puff_frames() - 1;

    if ((int)puff->frame > last) {
        task_kill(task);
        return;
    }
    puff->frame += puff->rate;
    puff->speed.x += -puff->speed.x * 0.02F;
    puff->speed.y += (-puff->speed.y * 0.02F) + 0.05F;
    puff->speed.z += -puff->speed.z * 0.02F;
    VECTOR_ADD(&puff->pos, &puff->speed);
}

static void debris_flame(struct task_work *work, struct debris_motion *motion)
{
    struct task *task = mountain_puff_create(PUFF_FLAME, debris_flame_step);
    struct mountain_puff *puff;

    if (task != NULL) {
        puff = (struct mountain_puff *)task->work;
        puff->frame = rand() * (1.0F / 8192.0F);
        puff->ang = ((rand() * RAND_SCALE) - 0.5F) * 4096.0F;
        puff->scale = RANDOM(0.02F, 0.04F) * motion->radius;
        puff->pos = work->pos;
        puff->rate = RANDOM(0.3F, 0.6F);
        VECTOR_SCALE(&puff->speed, &motion->speed, 0.5F);
    }
}
