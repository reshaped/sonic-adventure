/* O NEON1 to O NEON4 neon signs whose tubes light in turn */
#include <ninja.h>

#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg04/neon.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT neon_1_object;
extern NJS_OBJECT neon_2_object;
extern NJS_OBJECT neon_3_object;
extern NJS_OBJECT neon_4_object;
extern NJS_TEXLIST neon_1_texlist;
extern NJS_TEXLIST neon_2_texlist;
extern NJS_TEXLIST neon_3_texlist;
extern NJS_TEXLIST neon_4_texlist;

/* which of the four signs the task is, 0 to 3 */
#define NEON_SIGN(work) ((work)->tick)

static void neon_run(struct task *task);
static void neon_draw(struct task *task);
static void neon_destroy(struct task *task);

static void neon_run(struct task *task)
{
    if (!placement_try_leave(task)) {
        neon_draw(task);
        TASK_TIMER(task->work)++;
    }
}

void neon_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        task->destructor = neon_destroy;
        task->draw = neon_draw;
        task->step = neon_run;
        NEON_SIGN(work) = 0;
        MODEL(work)->object = &neon_1_object;
        MODEL(work)->texlist = &neon_1_texlist;
        NEON_SIGN(work) = 0;
    }
}

void neon_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        task->destructor = neon_destroy;
        task->draw = neon_draw;
        task->step = neon_run;
        NEON_SIGN(work) = 0;
        MODEL(work)->object = &neon_2_object;
        MODEL(work)->texlist = &neon_2_texlist;
        NEON_SIGN(work) = 1;
    }
}

void neon_3_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        task->destructor = neon_destroy;
        task->draw = neon_draw;
        task->step = neon_run;
        NEON_SIGN(work) = 0;
        MODEL(work)->object = &neon_3_object;
        MODEL(work)->texlist = &neon_3_texlist;
        NEON_SIGN(work) = 2;
    }
}

void neon_4_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        task->destructor = neon_destroy;
        task->draw = neon_draw;
        task->step = neon_run;
        NEON_SIGN(work) = 0;
        MODEL(work)->object = &neon_4_object;
        MODEL(work)->texlist = &neon_4_texlist;
        NEON_SIGN(work) = 3;
    }
}

/* each sign switches textures on its materials at timer 1, 60, 121 and 181.
 * a draw at 0 or 120 leaves them as they are. a draw past 240 sets the timer
 * to 0. neon_run then counts it to 1, so a cycle lasts 241 steps */
static void neon_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MATERIAL *mats = MODEL(work)->object->model->mats;
    Angle ang;

    if (TASK_TIMER(work) > 0 && TASK_TIMER(work) < 120) {
        switch (NEON_SIGN(work)) {
        case 0:
            if (TASK_TIMER(work) < 60) {
                mats[1].attr_texId = mats[2].attr_texId = 6;
            } else {
                mats[1].attr_texId = mats[2].attr_texId = 1;
            }
            mats[3].attr_texId = 5;
            mats[4].attr_texId = 4;
            break;
        case 1:
            if (TASK_TIMER(work) < 60) {
                mats[0].attr_texId = 3;
            } else {
                mats[0].attr_texId = 1;
            }
            mats[2].attr_texId = 4;
            break;
        case 2:
            if (TASK_TIMER(work) < 60) {
                mats[0].attr_texId = 4;
            } else {
                mats[0].attr_texId = 2;
            }
            mats[3].attr_texId = 3;
            break;
        case 3:
            if (TASK_TIMER(work) < 60) {
                mats[1].attr_texId = 4;
            } else {
                mats[1].attr_texId = 1;
            }
            mats[2].attr_texId = 5;
            break;
        }
    } else if (TASK_TIMER(work) > 120) {
        switch (NEON_SIGN(work)) {
        case 0:
            mats[3].attr_texId = 3;
            mats[4].attr_texId = 2;
            if (TASK_TIMER(work) > 180) {
                mats[1].attr_texId = mats[2].attr_texId = 6;
            } else {
                mats[1].attr_texId = mats[2].attr_texId = 1;
            }
            break;
        case 1:
            if (TASK_TIMER(work) > 180) {
                mats[0].attr_texId = 3;
            } else {
                mats[0].attr_texId = 1;
            }
            mats[2].attr_texId = 2;
            break;
        case 2:
            if (TASK_TIMER(work) > 180) {
                mats[0].attr_texId = 4;
            } else {
                mats[0].attr_texId = 2;
            }
            mats[3].attr_texId = 5;
            break;
        case 3:
            if (TASK_TIMER(work) > 180) {
                mats[1].attr_texId = 4;
            } else {
                mats[1].attr_texId = 1;
            }
            mats[2].attr_texId = 2;
            break;
        }
        if (TASK_TIMER(work) > 240) {
            work->timer = 0;
        }
    }
    njSetTexture(MODEL(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(MODEL(work)->object->model);
    njPopMatrix(1);
}

static void neon_destroy(struct task *task)
{
}
