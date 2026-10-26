/* EV0024.PRS's falling wreckage: a task that drops pieces at an interval */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "ev0024/wreckage.h"
#include "ev0024/wreckage_drop.h"
#include "system/task.h"

/* the values of a drop task's work->mode */
enum drop_mode {
    DROP_WAIT, /* counting steps[COUNTDOWN] down to 0 */
    DROP_NOW /* drop a piece, and wait again */
};

struct drop_vars {
    /* @hack type: the interval and the countdown as one array. SHC then
     * copies one into the other through the array's address (ADD #8,R4 at
     * 0x0cb81922). as two fields, the object runs 8 bytes short, and 118
     * of the 267 words of the five functions that use drop_vars differ */
    Sint16 steps[2];
    Uint8 unk_04[4];
    /* the params drop_piece sets for each piece. drop_destroy frees them */
    struct wreckage_params *params;
};

/* the indexes of drop_vars.steps: the interval, and the countdown to the
 * next piece */
#define INTERVAL 0
#define COUNTDOWN 1

#define VARS(work) ((struct drop_vars *)&(work)->vars)

/* a drop task has no draw, so its scale is free. work->scl.x holds the
 * pieces' scale, and SPREAD and SPEED take scl.y and scl.z */
/* a piece starts at most this far from pos on x and on z */
#define SPREAD(work) ((work)->scl.y)
/* a piece starts with at most this speed either way on each axis */
#define SPEED(work) ((work)->scl.z)

static void drop_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->params != NULL) {
        heap_free(VARS(work)->params);
        VARS(work)->params = NULL;
    }
}

static void drop_piece(struct task_work *work)
{
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Float scale;
    struct wreckage_params *params = VARS(work)->params;

    pos.x = work->pos.x + ((Float)rand() * SPREAD(work) * (1.0F / 16384.0F)) -
            SPREAD(work);
    pos.y = work->pos.y;
    pos.z = work->pos.z + ((Float)rand() * SPREAD(work) * (1.0F / 16384.0F)) -
            SPREAD(work);
    speed.x = ((Float)rand() * SPEED(work) * (1.0F / 16384.0F)) - SPEED(work);
    speed.y = ((Float)rand() * SPEED(work) * (1.0F / 16384.0F)) - SPEED(work);
    speed.z = ((Float)rand() * SPEED(work) * (1.0F / 16384.0F)) - SPEED(work);
    scale = work->scl.x;
    params->model = (int)((Float)rand() * (1.0F / 4096.0F));
    params->flags = WRECKAGE_SMOKES | WRECKAGE_BOUNCES | WRECKAGE_SHADOW;
    params->interval = 5;
    params->life = 170;
    params->bounce = 0.5F;
    params->shadow = 0.7F;
    params->spin.x = 256;
    params->spin.y = 336;
    params->spin.z = 256;
    wreckage_create(&pos, &speed, scale, params);
}

static void drop_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case DROP_WAIT:
        if (VARS(work)->steps[COUNTDOWN] == 0) {
            work->mode = DROP_NOW;
        } else {
            --VARS(work)->steps[COUNTDOWN];
        }
        break;
    case DROP_NOW:
        drop_piece(work);
        VARS(work)->steps[COUNTDOWN] = VARS(work)->steps[INTERVAL];
        work->mode = DROP_WAIT;
        break;
    }
}

static void drop_init(struct task *task)
{
    struct task_work *work = task->work;
    struct wreckage_params *params = heap_alloc(sizeof(*params));

    if (params == NULL) {
        task_kill(task);
        return;
    }
    /* drop_piece sets every field again before each piece, so no piece
     * takes these values */
    params->model = 0;
    params->flags = WRECKAGE_SMOKES | WRECKAGE_BOUNCES | WRECKAGE_SHADOW;
    params->interval = 40;
    params->life = 120;
    params->bounce = 0.5F;
    params->shadow = 0.4F;
    params->spin.x = 1024;
    params->spin.y = 1024;
    params->spin.z = 1024;
    VARS(work)->steps[COUNTDOWN] = VARS(work)->steps[INTERVAL];
    VARS(work)->params = params;
    task->step = drop_step;
    task->destructor = drop_destroy;
}

struct task *wreckage_drop_create(Float x, Float y, Float z, Float speed,
                                  Float scale, Float spread, Sint16 interval)
{
    struct task *task = task_create(TASK_WORK, 3, drop_init);

    if (task != NULL) {
        struct task_work *work = task->work;

        work->mode = DROP_WAIT;
        work->pos.x = x;
        work->pos.y = y;
        work->pos.z = z;
        work->scl.x = scale;
        SPREAD(work) = spread;
        SPEED(work) = speed;
        VARS(work)->steps[INTERVAL] = interval;
    }
    return task;
}
