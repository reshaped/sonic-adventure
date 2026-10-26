/* O FENS: a section of fence, 48 wide */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/fence.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_fence_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum shelter_fence_mode { FENCE_INIT, FENCE_RUN };

static void shelter_fence_init(struct task *task);
static void shelter_fence_draw(struct task *task);
static void shelter_fence_destroy(struct task *task);

static struct collision_shape shelter_fence_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    24.0F,
    20.5F,
    2.2F,
    {0},
    0,
    NULL,
};

/* @hack spelling: the registration in a helper, which SHC expands at both of
 * shelter_fence_step's calls. the step then saves only R14 and R13 (0x0c909320,
 * 0x0c909324). with collision_register(work) written in the step, it saves R12
 * too, the object's code runs 52 bytes long, and 66 of the step's 69 words
 * differ */
static void shelter_fence_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void shelter_fence_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case FENCE_INIT:
                shelter_fence_init(task);
                break;
            case FENCE_RUN:
                shelter_fence_draw(task);
                shelter_fence_register(task);
                break;
            }
        } else {
            switch (work->mode) {
            case FENCE_INIT:
                shelter_fence_init(task);
                break;
            case FENCE_RUN:
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
                shelter_fence_draw(task);
                shelter_fence_register(task);
                break;
            }
        }
    }
}

static void shelter_fence_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shelter_fence_draw;
    task->destructor = shelter_fence_destroy;
    collision_init(task, &shelter_fence_shape, COLLISION_OBJECT, 1, 1);
    work->mode = FENCE_RUN;
}

static void shelter_fence_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&shelter_fence_object);
        njPopMatrix(1);
    }
}

static void shelter_fence_destroy(struct task *task)
{
}
