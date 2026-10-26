/* O TUKIKOU, an air vent cover 15 units square that hangs below its place */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/vent.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT vent_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum vent_mode { VENT_INIT, VENT_RUN };

static void vent_init(struct task *task);
static void vent_draw(struct task *task);
static void vent_destroy(struct task *task);

static struct collision_shape vent_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, -3.0F, 0.0F},
    6.0F,
    6.0F,
    1.0F,
    {0},
    0,
    NULL,
};

/* @hack spelling: collision_register through an inline function, which reads
 * task->work again at each call (MOV.L @(32,R4),R4 at 0x0c90515c and
 * 0x0c9051a0). called with vent_step's own work, the step saves R12 and
 * jumps to collision_register. it then runs 14 bytes longer, and 66 of its
 * 69 words differ */
static void vent_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void vent_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case VENT_INIT:
                vent_init(task);
                break;
            case VENT_RUN:
                vent_draw(task);
                vent_register(task);
                break;
            }
        } else {
            switch (work->mode) {
            case VENT_INIT:
                vent_init(task);
                break;
            case VENT_RUN:
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
                vent_draw(task);
                vent_register(task);
                break;
            }
        }
    }
}

static void vent_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = vent_draw;
    task->destructor = vent_destroy;
    collision_init(task, &vent_shape, COLLISION_OBJECT, 1, 1);
    work->mode = VENT_RUN;
}

static void vent_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&vent_object);
        njPopMatrix(1);
    }
}

static void vent_destroy(struct task *task)
{
}
