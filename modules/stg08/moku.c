/* O MOKU A to O MOKU C, O DRY A and O DRY B: scenery that is only drawn */
#include <ninja.h>
#include <sg_xpt.h>

#include "object/placement.h"
#include "stg08/moku.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT moku_a_object;
extern NJS_OBJECT moku_b_object;
extern NJS_OBJECT moku_c_object;
extern NJS_OBJECT dry_a_object;
extern NJS_OBJECT dry_b_object;

struct moku_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *object;
};

#define VARS(work) ((struct moku_vars *)&(work)->vars)

static void moku_destroy(struct task *task)
{
}

static void moku_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njEasyDrawObject(VARS(work)->object);
    njPopMatrix(1);
}

static void moku_run(struct task *task)
{
    if (!placement_try_leave(task)) {
        moku_draw(task);
    }
}

static void moku_init(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;

    task->destructor = moku_destroy;
    task->step = moku_run;
    task->draw = moku_draw;
    VARS(work)->object = object;
}

void moku_a_step(struct task *task)
{
    moku_init(task, &moku_a_object);
}

void moku_b_step(struct task *task)
{
    moku_init(task, &moku_b_object);
}

void moku_c_step(struct task *task)
{
    moku_init(task, &moku_c_object);
}

void dry_a_step(struct task *task)
{
    moku_init(task, &dry_a_object);
}

void dry_b_step(struct task *task)
{
    moku_init(task, &dry_b_object);
}
