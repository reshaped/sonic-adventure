/* O Hakidashi outlet model with a solid box at its foot */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg10/outlet.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT outlet_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum outlet_mode { OUTLET_INIT, OUTLET_RUN };

static void outlet_init(struct task *task);
static void outlet_draw(struct task *task);
static void outlet_destroy(struct task *task);

static struct collision_shape outlet_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 6.0F, 0.0F},
    6.0F,
    6.0F,
    8.0F,
    {0},
    0,
    NULL,
};

/* @hack spelling: collision_register in a helper SHC expands at each call,
 * which keeps task in a stack slot (MOV.L R14,@R15 at 0x0c910d04). with
 * collision_register(work) written in the step, the call becomes a jump and
 * 63 of outlet_step's 78 words differ */
static void outlet_register(struct task *task)
{
    collision_register(task->work);
}

/* the set editor labels all three angles, though only angle y turns the
 * outlet */
void outlet_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case OUTLET_INIT:
            outlet_init(task);
            break;
        case OUTLET_RUN:
            outlet_draw(task);
            outlet_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case OUTLET_INIT:
            outlet_init(task);
            break;
        case OUTLET_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            outlet_draw(task);
            outlet_register(task);
            break;
        }
    }
}

static void outlet_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = outlet_draw;
    task->destructor = outlet_destroy;
    collision_init(task, &outlet_shape, COLLISION_OBJECT, 1, 1);
    work->mode = OUTLET_RUN;
}

static void outlet_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_tree(&outlet_object);
        njPopMatrix(1);
    }
}

static void outlet_destroy(struct task *task)
{
}
