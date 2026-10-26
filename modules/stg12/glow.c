/* O HIKARI: a hanging lamp */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/glow.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_glow_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum shelter_glow_mode { GLOW_INIT, GLOW_RUN };

static void shelter_glow_init(struct task *task);
static void shelter_glow_draw(struct task *task);
static void shelter_glow_destroy(struct task *task);

/* a shape no code reads, whose bytes the image holds */
static struct collision_shape shelter_glow_shape = {
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

void shelter_glow_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case GLOW_INIT:
            shelter_glow_init(task);
            break;
        case GLOW_RUN:
            shelter_glow_draw(task);
            shelter_glow_destroy(task);
            break;
        /* @hack redundant: an empty default, which leaves the expanded
         * shelter_glow_destroy's MOV R14,R4 at 0x0c9091fc ahead of the BRA
         * rather than in its delay slot; without it, 40 of 66 words differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case GLOW_INIT:
            shelter_glow_init(task);
            break;
        case GLOW_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            shelter_glow_draw(task);
            shelter_glow_destroy(task);
            break;
        }
    }
}

static void shelter_glow_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shelter_glow_draw;
    task->destructor = shelter_glow_destroy;
    work->mode = GLOW_RUN;
}

static void shelter_glow_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&shelter_glow_object);
        njPopMatrix(1);
    }
}

/* the destructor, which the step also runs after each draw. it frees
 * nothing */
static void shelter_glow_destroy(struct task *task)
{
    /* @hack redundant: a read of task->work, which keeps the copy of task into
     * R4 at each of the step's two expansions of this function (0x0c9091fc,
     * 0x0c90923a); without it, 41 of shelter_glow_step's 66 words differ */
    (void)task->work;
}
