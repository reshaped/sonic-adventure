/* EV0024.PRS's wreckage: a piece that flies and spins, and can bounce,
 * smoke and cast a shadow */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/ground.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "ev0024/wreckage.h"
#include "system/task.h"
#include "system/util.h"

/* bits of a piece's work->flags. this file never sets WRECKAGE_GONE or
 * reads WRECKAGE_BOUNCED */
#define WRECKAGE_BOUNCED 0x100 /* the piece bounced this step */
#define WRECKAGE_GONE 0x200 /* kill the piece at its next step */

struct wreckage_vars {
    Uint8 unk_00[4];
    Sint16 life; /* the steps the piece has left to move */
    Uint8 unk_06[2];
    /* the piece's own copy of its params, which wreckage_destroy frees */
    struct wreckage_params *params;
};

#define VARS(work) ((struct wreckage_vars *)&(work)->vars)

struct wreckage_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    struct angles spin;
};

#define MOTION(task) ((struct wreckage_motion *)(task)->motion)

extern NJS_TEXLIST ev0024_wreckage_texlist;
extern NJS_OBJECT ev0024_wreckage_0;
extern NJS_OBJECT ev0024_wreckage_1;
extern NJS_OBJECT ev0024_wreckage_2;
extern NJS_OBJECT ev0024_wreckage_3;
extern NJS_OBJECT ev0024_wreckage_4;
extern NJS_OBJECT ev0024_wreckage_5;
extern NJS_OBJECT ev0024_wreckage_6;
extern NJS_OBJECT ev0024_wreckage_7;

static NJS_OBJECT *wreckage_objects[] = {
    &ev0024_wreckage_0, &ev0024_wreckage_1, &ev0024_wreckage_2,
    &ev0024_wreckage_3, &ev0024_wreckage_4, &ev0024_wreckage_5,
    &ev0024_wreckage_6, &ev0024_wreckage_7,
};

/* every puff keeps these params, not a copy. wreckage_smoke sets their pos,
 * speed and scale before each puff */
static struct dust_params wreckage_puff = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static void wreckage_smoke(struct task_work *work,
                           struct wreckage_motion *motion);

static void wreckage_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->params != NULL) {
        heap_free(VARS(work)->params);
        VARS(work)->params = NULL;
    }
}

static void wreckage_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct wreckage_params *params = VARS(work)->params;
    int model = params->model;
    Angle ang;

    njSetTexture(&ev0024_wreckage_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
    draw_tree(wreckage_objects[model]);
    njPopMatrix(1);
}

static void wreckage_step(struct task *task)
{
    struct task_work *work = task->work;
    struct wreckage_params *params = VARS(work)->params;
    int flags = params->flags;
    struct wreckage_motion *motion = MOTION(task);
    struct angles slope;
    NJS_POINT3 shadow;
    Float ground;

    if (work->flags & WRECKAGE_GONE) {
        task_kill(task);
        return;
    }
    if (VARS(work)->life == 0) {
        task_kill(task);
        return;
    }
    --VARS(work)->life;
    motion->speed.x += -(motion->speed.x * 0.03F);
    motion->speed.y += -(motion->speed.y * 0.01F) - 0.1F;
    motion->speed.z += -(motion->speed.z * 0.03F);
    work->pos.x += motion->speed.x;
    work->pos.y += motion->speed.y;
    work->pos.z += motion->speed.z;
    work->ang.x += motion->spin.x;
    work->ang.y += motion->spin.y;
    work->ang.z += motion->spin.z;
    work->flags &= ~WRECKAGE_BOUNCED;
    ground =
        ground_below(work->pos.x, work->pos.y + 10.0F, work->pos.z, &slope);
    if ((flags & WRECKAGE_BOUNCES) && ground > work->pos.y &&
        motion->speed.y < 0.0F) {
        work->pos.y = ground;
        motion->speed.x *= 0.9F;
        motion->speed.y *= -params->bounce;
        motion->speed.z *= 0.9F;
        motion->spin.x = motion->spin.x * 0.5F;
        motion->spin.y = motion->spin.y * 0.5F;
        motion->spin.z = motion->spin.z * 0.5F;
        work->flags |= WRECKAGE_BOUNCED;
    }
    if (flags & WRECKAGE_SHADOW) {
        shadow.x = work->pos.x;
        shadow.y = ground + SHADOW_LIFT;
        shadow.z = work->pos.z;
        shadow_draw_at(&slope, &shadow, params->shadow);
    }
    if (flags & WRECKAGE_SMOKES) {
        if (work->timer == 0) {
            wreckage_smoke(work, motion);
            work->timer = params->interval;
        } else {
            --work->timer;
        }
    }
    wreckage_draw(task);
}

static void wreckage_init(struct task *task)
{
    task->step = wreckage_step;
    task->draw = wreckage_draw;
    task->destructor = wreckage_destroy;
}

void wreckage_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale,
                     struct wreckage_params *params)
{
    struct task *task;
    struct task_work *work;
    struct wreckage_motion *motion;
    struct wreckage_params *copy;

    task = task_create(TASK_MOTION | TASK_WORK, 3, wreckage_init);
    if (task != NULL) {
        work = task->work;
        motion = MOTION(task);
        copy = heap_alloc(sizeof(*copy));
        work->pos = *pos;
        work->scl.x = scale;
        work->scl.y = scale;
        work->scl.z = scale;
        VARS(work)->life = params->life;
        motion->speed = *speed;
        motion->spin.x =
            0.5F - ((Float)rand() * params->spin.x * (1.0F / 16384.0F));
        motion->spin.y =
            0.5F - ((Float)rand() * params->spin.y * (1.0F / 16384.0F));
        motion->spin.z =
            0.5F - ((Float)rand() * params->spin.z * (1.0F / 16384.0F));
        copy->model = params->model;
        copy->flags = params->flags;
        copy->interval = params->interval;
        copy->life = params->life;
        copy->bounce = params->bounce;
        copy->shadow = params->shadow;
        copy->spin = params->spin;
        VARS(work)->params = copy;
    }
}

/* make a puff of smoke at the piece, at a scale of 2. the puff moves
 * against the piece's speed, at a speed of 1 */
static void wreckage_smoke(struct task_work *work,
                           struct wreckage_motion *motion)
{
    NJS_POINT3 pos;
    NJS_VECTOR back;

    pos.x = work->pos.x;
    pos.y = work->pos.y;
    pos.z = work->pos.z;
    back.x = -motion->speed.x;
    back.y = -motion->speed.y;
    back.z = -motion->speed.z;
    njUnitVector(&back);
    wreckage_puff.pos.x = pos.x;
    wreckage_puff.pos.y = pos.y;
    wreckage_puff.pos.z = pos.z;
    wreckage_puff.speed.x = back.x;
    wreckage_puff.speed.y = back.y;
    wreckage_puff.speed.z = back.z;
    wreckage_puff.scale = 2.0F;
    dust_create_from(&wreckage_puff);
}
