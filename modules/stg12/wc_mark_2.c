/* O WCMARK2, a toilet sign whose model differs from O WCMARK1's only in its
 * texture */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/wc_mark_2.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT wc_mark_2_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum wc_mark_2_mode { MARK_INIT, MARK_RUN };

static void wc_mark_2_init(struct task *task);
static void wc_mark_2_draw(struct task *task);
static void wc_mark_2_destroy(struct task *task);

/* no code reads this shape, but the image holds it, as this file's only
 * data */
static struct collision_shape wc_mark_2_shape = {
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

void wc_mark_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case MARK_INIT:
            wc_mark_2_init(task);
            break;
        case MARK_RUN:
            wc_mark_2_draw(task);
            wc_mark_2_destroy(task);
            break;
        /* @hack redundant: an empty default, which ends the run case with a BRA
         * past the switch and a NOP in its slot (0x0c90979e, 0x0c9097a0);
         * without it, the case's last MOV takes the slot, and 40 of 66 words
         * differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case MARK_INIT:
            wc_mark_2_init(task);
            break;
        case MARK_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            wc_mark_2_draw(task);
            wc_mark_2_destroy(task);
            break;
        }
    }
}

static void wc_mark_2_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = wc_mark_2_draw;
    task->destructor = wc_mark_2_destroy;
    work->mode = MARK_RUN;
}

static void wc_mark_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&wc_mark_2_object);
        njPopMatrix(1);
    }
}

static void wc_mark_2_destroy(struct task *task)
{
    /* @hack redundant: a read of task->work, which leaves a copy of task in R4
     * where wc_mark_2_step calls the destructor (MOV R14,R4 at 0x0c90979c
     * and 0x0c9097da); with an empty body, the step runs 4 bytes shorter and
     * 41 of its 66 words differ */
    (void)task->work;
}
