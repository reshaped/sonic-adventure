/* O WCMARK1, a toilet sign */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/wc_mark_1.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT wc_mark_1_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum wc_mark_1_mode { MARK_INIT, MARK_RUN };

static void wc_mark_1_init(struct task *task);
static void wc_mark_1_draw(struct task *task);
static void wc_mark_1_destroy(struct task *task);

/* no code reads this shape, but the image holds it, as this file's only
 * data */
static struct collision_shape wc_mark_1_shape = {
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

void wc_mark_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case MARK_INIT:
            wc_mark_1_init(task);
            break;
        case MARK_RUN:
            wc_mark_1_draw(task);
            wc_mark_1_destroy(task);
            break;
        /* @hack redundant: an empty default, which ends the run case with a BRA
         * past the switch and a NOP in its slot (0x0c90963e, 0x0c909640);
         * without it, the case's last MOV takes the slot, and 40 of 66 words
         * differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case MARK_INIT:
            wc_mark_1_init(task);
            break;
        case MARK_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            wc_mark_1_draw(task);
            wc_mark_1_destroy(task);
            break;
        }
    }
}

static void wc_mark_1_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = wc_mark_1_draw;
    task->destructor = wc_mark_1_destroy;
    work->mode = MARK_RUN;
}

static void wc_mark_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&wc_mark_1_object);
        njPopMatrix(1);
    }
}

static void wc_mark_1_destroy(struct task *task)
{
    /* @hack redundant: a read of task->work, which leaves a copy of task in R4
     * where wc_mark_1_step calls the destructor (MOV R14,R4 at 0x0c90963c
     * and 0x0c90967a); with an empty body, the step runs 4 bytes shorter and
     * 41 of its 66 words differ */
    (void)task->work;
}
