/* O SASAE1C, a support strut 37 units tall, solid only at its foot */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/support_1c.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT support_1c_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum support_1c_mode { SUPPORT_INIT, SUPPORT_RUN };

static void support_1c_init(struct task *task);
static void support_1c_draw(struct task *task);
static void support_1c_destroy(struct task *task);

static struct collision_shape support_1c_shape = {
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

/* @hack spelling: collision_register through an inline function, which reads
 * task->work again at each call (MOV.L @(32,R4),R4 at 0x0c9033bc and
 * 0x0c903400). called with support_1c_step's own work, the step saves R12
 * and jumps to collision_register. it then runs 14 bytes longer, and 66 of
 * its 69 words differ */
static void support_1c_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void support_1c_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case SUPPORT_INIT:
                support_1c_init(task);
                break;
            case SUPPORT_RUN:
                support_1c_draw(task);
                support_1c_register(task);
                break;
            }
        } else {
            switch (work->mode) {
            case SUPPORT_INIT:
                support_1c_init(task);
                break;
            case SUPPORT_RUN:
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
                support_1c_draw(task);
                support_1c_register(task);
                break;
            }
        }
    }
}

static void support_1c_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = support_1c_draw;
    task->destructor = support_1c_destroy;
    collision_init(task, &support_1c_shape, COLLISION_OBJECT, 1, 1);
    work->mode = SUPPORT_RUN;
}

static void support_1c_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&support_1c_object);
        njPopMatrix(1);
    }
}

static void support_1c_destroy(struct task *task)
{
}
