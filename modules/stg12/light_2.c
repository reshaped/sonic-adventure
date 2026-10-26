/* O LIGHT2: a standing lamp */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/light_2.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_light_2_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum shelter_light_2_mode { LIGHT_INIT, LIGHT_RUN };

static void shelter_light_2_init(struct task *task);
static void shelter_light_2_draw(struct task *task);
static void shelter_light_2_destroy(struct task *task);

/* a shape no code reads, whose bytes the image holds */
static struct collision_shape shelter_light_2_shape = {
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

void shelter_light_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIGHT_INIT:
            shelter_light_2_init(task);
            break;
        case LIGHT_RUN:
            shelter_light_2_draw(task);
            shelter_light_2_destroy(task);
            break;
        /* @hack redundant: an empty default, which leaves the expanded
         * shelter_light_2_destroy's MOV R14,R4 at 0x0c900a5c ahead of the BRA
         * rather than in its delay slot; without it, 40 of 66 words differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case LIGHT_INIT:
            shelter_light_2_init(task);
            break;
        case LIGHT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            shelter_light_2_draw(task);
            shelter_light_2_destroy(task);
            break;
        }
    }
}

static void shelter_light_2_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shelter_light_2_draw;
    task->destructor = shelter_light_2_destroy;
    work->mode = LIGHT_RUN;
}

static void shelter_light_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&shelter_light_2_object);
        njPopMatrix(1);
    }
}

/* the destructor, which the step also runs after each draw. it frees
 * nothing */
static void shelter_light_2_destroy(struct task *task)
{
    /* @hack redundant: a read of task->work, which keeps the copy of task into
     * R4 at each of the step's two expansions of this function (0x0c900a5c,
     * 0x0c900a9a); without it, 41 of shelter_light_2_step's 66 words differ */
    (void)task->work;
}
