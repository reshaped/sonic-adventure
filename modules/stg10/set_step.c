/* O SETSTEP solid step */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg10/set_step.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL set_step_model;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum set_step_mode { STEP_INIT, STEP_RUN };

static void set_step_init(struct task *task);
static void set_step_draw(struct task *task);
static void set_step_destroy(struct task *task);

/* a box 70 units along x, 51 high and 200 along z, its top at the step's
 * position. it pushes every group, and no group's shapes push it */
static struct collision_shape set_step_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, -25.5F, 0.0F},
    35.0F,
    25.5F,
    100.0F,
    {0},
    0,
    NULL,
};

/* @hack spelling: the registration in a helper SHC expands. the step then
 * keeps task in its frame and calls collision_register (0x0c900c24,
 * 0x0c900c26). written in place, the call becomes a jump, the step runs 4
 * bytes short and 63 of its 78 words differ */
static void set_step_register(struct task *task)
{
    collision_register(task->work);
}

/* the set editor labels all three angles, though only angle y turns the
 * step */
void set_step_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case STEP_INIT:
            set_step_init(task);
            break;
        case STEP_RUN:
            set_step_draw(task);
            set_step_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case STEP_INIT:
            set_step_init(task);
            break;
        case STEP_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            set_step_draw(task);
            set_step_register(task);
            break;
        }
    }
}

static void set_step_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = set_step_draw;
    task->destructor = set_step_destroy;
    collision_init(task, &set_step_shape, COLLISION_OBJECT, 1, 1);
    work->mode = STEP_RUN;
}

static void set_step_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&set_step_model);
        njPopMatrix(1);
    }
}

static void set_step_destroy(struct task *task)
{
}
