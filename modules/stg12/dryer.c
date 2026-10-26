/* O KANSO, a dryer that only draws */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/dryer.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT dryer_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum dryer_mode { DRYER_INIT, DRYER_RUN };

static void dryer_init(struct task *task);
static void dryer_draw(struct task *task);
static void dryer_destroy(struct task *task);

/* no code reads this shape, but the image holds it */
static struct collision_shape dryer_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    17.0F,
    1.5F,
    0.0F,
    {0},
    0,
    NULL,
};

void dryer_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DRYER_INIT:
            dryer_init(task);
            break;
        case DRYER_RUN:
            dryer_draw(task);
            dryer_destroy(task);
            break;
        /* @hack redundant: an empty default, which leaves a NOP in the slot of
         * DRYER_RUN's BRA past the switch, at 0x0c9094de. without it, the copy
         * of task into R4 fills the slot, and 40 of 66 words differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case DRYER_INIT:
            dryer_init(task);
            break;
        case DRYER_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            dryer_draw(task);
            dryer_destroy(task);
            break;
        }
    }
}

static void dryer_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = dryer_draw;
    task->destructor = dryer_destroy;
    work->mode = DRYER_RUN;
}

static void dryer_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&dryer_object);
        njPopMatrix(1);
    }
}

static void dryer_destroy(struct task *task)
{
    /* @hack redundant: a read of the work, which keeps the task passed in, MOV
     * R14,R4 at 0x0c9094dc and 0x0c90951a, where SHC expands this in
     * dryer_step; without it, 41 of 66 words differ */
    (void)task->work;
}
