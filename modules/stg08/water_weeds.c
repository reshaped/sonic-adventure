/* O MizukusaA and O MizukusaB, water weeds that sway */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "object/placement.h"
#include "stg08/water_weeds.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION water_weed_a_action;
extern NJS_ACTION water_weed_b_action;

/* a weed draws its action's frames 0 to WEED_FRAMES - 1 in turn, and then
 * starts over */
#define WEED_FRAMES 29.0F

struct water_weed_vars {
    Uint8 unk_00[4];
    Float frame; /* the frame the weed draws next */
};

#define VARS(work) ((struct water_weed_vars *)&(work)->vars)

static void water_weed_destroy(struct task *task)
{
}

static void water_weed_a_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &work->pos;
    NJS_VECTOR *scl = &work->scl;
    Float frame = VARS(work)->frame;
    Angle ang;

    if (!placement_try_leave(task)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScaleV(NULL, scl);
        draw_action(&water_weed_a_action, frame);
        njPopMatrix(1);
        frame += 1.0F;
        if (frame >= WEED_FRAMES) {
            frame = 0.0F;
        }
        VARS(work)->frame = frame;
    }
}

static void water_weed_b_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &work->pos;
    NJS_VECTOR *scl = &work->scl;
    Float frame = VARS(work)->frame;
    Angle ang;

    if (!placement_try_leave(task)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScaleV(NULL, scl);
        draw_action(&water_weed_b_action, frame);
        njPopMatrix(1);
        frame += 1.0F;
        if (frame >= WEED_FRAMES) {
            frame = 0.0F;
        }
        VARS(work)->frame = frame;
    }
}

/* kind 0 is O MizukusaA, and any other O MizukusaB. work->mode keeps the
 * kind, which the weeds never read */
static void water_weed_init(struct task *task, Sint8 kind)
{
    /* @hack spelling: draw set from step in each arm, which has SHC merge the
     * arms' tails and store both from R1 (0x0c905802 to 0x0c905808); set
     * once after them, it reads step back from the task and 8 of 15 words
     * differ */
    if (kind == 0) {
        task->step = water_weed_a_run;
        task->draw = task->step;
    } else {
        task->step = water_weed_b_run;
        task->draw = task->step;
    }
    task->work->mode = kind;
    task->destructor = water_weed_destroy;
}

void water_weed_a_step(struct task *task)
{
    water_weed_init(task, 0);
}

void water_weed_b_step(struct task *task)
{
    water_weed_init(task, 1);
}
