/* O Light2 lamp turned about all three axes, with a solid box at its foot */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg10/light2.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT light2_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum light2_mode { LIGHT_INIT, LIGHT_RUN };

static void light2_init(struct task *task);
static void light2_draw(struct task *task);
static void light2_destroy(struct task *task);

static struct collision_shape light2_shape = {
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
 * which keeps task in a stack slot (MOV.L R14,@R15 at 0x0c9131e4). with
 * collision_register(work) written in the step, the call becomes a jump and
 * 63 of light2_step's 78 words differ */
static void light2_register(struct task *task)
{
    collision_register(task->work);
}

void light2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIGHT_INIT:
            light2_init(task);
            break;
        case LIGHT_RUN:
            light2_draw(task);
            light2_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case LIGHT_INIT:
            light2_init(task);
            break;
        case LIGHT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            light2_draw(task);
            light2_register(task);
            break;
        }
    }
}

static void light2_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = light2_draw;
    task->destructor = light2_destroy;
    collision_init(task, &light2_shape, COLLISION_OBJECT, 1, 1);
    work->mode = LIGHT_RUN;
}

static void light2_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateZ, work->ang.z);
        draw_tree(&light2_object);
        njPopMatrix(1);
    }
}

static void light2_destroy(struct task *task)
{
}
