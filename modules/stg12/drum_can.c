/* O DRUMCAN, a solid oil drum */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/drum_can.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT drum_can_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum drum_can_mode { DRUM_INIT, DRUM_RUN };

static void drum_can_init(struct task *task);
static void drum_can_draw(struct task *task);
static void drum_can_destroy(struct task *task);

static struct collision_shape drum_can_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 8.0F, 0.0F},
    8.5F,
    8.5F,
    0.0F,
    {0},
    0,
    NULL,
};

/* @hack spelling: the registration as a helper SHC expands, which reads the
 * work from task again at each call, as MOV.L @(32,R4),R4 at 0x0c90709c
 * shows; written in place, the step runs 14 bytes long and 66 of its 69
 * words differ */
static void drum_can_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void drum_can_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DRUM_INIT:
            drum_can_init(task);
            break;
        case DRUM_RUN:
            drum_can_draw(task);
            drum_can_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case DRUM_INIT:
            drum_can_init(task);
            break;
        case DRUM_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            drum_can_draw(task);
            drum_can_register(task);
            break;
        }
    }
}

static void drum_can_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = drum_can_draw;
    task->destructor = drum_can_destroy;
    collision_init(task, &drum_can_shape, COLLISION_OBJECT, 1, 1);
    work->mode = DRUM_RUN;
}

static void drum_can_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&drum_can_object);
        njPopMatrix(1);
    }
}

static void drum_can_destroy(struct task *task)
{
}
