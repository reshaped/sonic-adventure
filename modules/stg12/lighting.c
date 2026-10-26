/* O SYOUMEI: a light fitting */
#include <ninja.h>

#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/lighting.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT lighting_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum lighting_mode { LIGHTING_INIT, LIGHTING_RUN };

static void lighting_init(struct task *task);
static void lighting_draw(struct task *task);
static void lighting_destroy(struct task *task);

void lighting_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIGHTING_INIT:
            lighting_init(task);
            break;
        case LIGHTING_RUN:
            lighting_draw(task);
            lighting_destroy(task);
            break;
        /* @hack redundant: an empty default, which leaves the expanded
         * lighting_destroy's MOV R14,R4 at 0x0c90751c ahead of the BRA
         * rather than in its delay slot; without it, 40 of 66 words differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case LIGHTING_INIT:
            lighting_init(task);
            break;
        case LIGHTING_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            lighting_draw(task);
            lighting_destroy(task);
            break;
        }
    }
}

static void lighting_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = lighting_draw;
    task->destructor = lighting_destroy;
    work->mode = LIGHTING_RUN;
}

static void lighting_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&lighting_object);
        njPopMatrix(1);
    }
}

/* the destructor, which the step also runs after each draw. it frees
 * nothing */
static void lighting_destroy(struct task *task)
{
    /* @hack redundant: a read of task->work, which keeps the copy of task into
     * R4 at each of the step's two expansions of this function (0x0c90751c,
     * 0x0c90755a); without it, 41 of lighting_step's 66 words differ */
    (void)task->work;
}
