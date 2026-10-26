/* O KAZARI2, an ornament 43 units tall */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/ornament_2.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT ornament_2_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum ornament_2_mode { ORNAMENT_INIT, ORNAMENT_RUN };

static void ornament_2_init(struct task *task);
static void ornament_2_draw(struct task *task);
static void ornament_2_destroy(struct task *task);

static struct collision_shape ornament_2_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 14.5F, 0.0F},
    8.0F,
    14.5F,
    0.0F,
    {0},
    0,
    NULL,
};

/* @hack spelling: collision_register through an inline function, which reads
 * task->work again at each call (MOV.L @(32,R4),R4 at 0x0c90739c and
 * 0x0c9073e0). called with ornament_2_step's own work, the step saves R12
 * and jumps to collision_register. it then runs 14 bytes longer, and 66 of
 * its 69 words differ */
static void ornament_2_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void ornament_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case ORNAMENT_INIT:
                ornament_2_init(task);
                break;
            case ORNAMENT_RUN:
                ornament_2_draw(task);
                ornament_2_register(task);
                break;
            }
        } else {
            switch (work->mode) {
            case ORNAMENT_INIT:
                ornament_2_init(task);
                break;
            case ORNAMENT_RUN:
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
                ornament_2_draw(task);
                ornament_2_register(task);
                break;
            }
        }
    }
}

static void ornament_2_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = ornament_2_draw;
    task->destructor = ornament_2_destroy;
    collision_init(task, &ornament_2_shape, COLLISION_OBJECT, 1, 1);
    work->mode = ORNAMENT_RUN;
}

static void ornament_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&ornament_2_object);
        njPopMatrix(1);
    }
}

static void ornament_2_destroy(struct task *task)
{
}
