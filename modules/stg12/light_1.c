/* O LIGHT1: a standing lamp, solid at its base */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/light_1.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_light_1_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum shelter_light_1_mode { LIGHT_INIT, LIGHT_RUN };

static void shelter_light_1_init(struct task *task);
static void shelter_light_1_draw(struct task *task);
static void shelter_light_1_destroy(struct task *task);

static struct collision_shape shelter_light_1_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 2.5F, 0.0F},
    8.0F,
    2.5F,
    0.0F,
    {0},
    0,
    NULL,
};

/* @hack spelling: the registration in a helper, which SHC expands at both of
 * shelter_light_1_step's calls. the step then saves only R14 and R13
 * (0x0c9008a0, 0x0c9008a4). with collision_register(work) written in the step,
 * it saves R12 too, the object's code runs 52 bytes long, and 66 of the step's
 * 69 words differ */
static void shelter_light_1_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void shelter_light_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIGHT_INIT:
            shelter_light_1_init(task);
            break;
        case LIGHT_RUN:
            shelter_light_1_draw(task);
            shelter_light_1_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case LIGHT_INIT:
            shelter_light_1_init(task);
            break;
        case LIGHT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            shelter_light_1_draw(task);
            shelter_light_1_register(task);
            break;
        }
    }
}

static void shelter_light_1_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shelter_light_1_draw;
    task->destructor = shelter_light_1_destroy;
    collision_init(task, &shelter_light_1_shape, COLLISION_OBJECT, 1, 1);
    work->mode = LIGHT_RUN;
}

static void shelter_light_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&shelter_light_1_object);
        njPopMatrix(1);
    }
}

static void shelter_light_1_destroy(struct task *task)
{
}
