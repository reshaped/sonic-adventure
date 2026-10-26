/* O Light1 lamp turned about all three axes */
#include <ninja.h>

#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg10/light1.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT light1_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum light1_mode { LIGHT_INIT, LIGHT_RUN };

static void light1_init(struct task *task);
static void light1_draw(struct task *task);
static void light1_destroy(struct task *task);

/* the lamp has no collision record, so this is empty */
/* @hack redundant: calls of an empty helper, which SHC expands to nothing.
 * with a call after it, light1_draw stays a call (BSR at 0x0c913058 and
 * 0x0c913094). without the calls, SHC jumps to light1_draw instead: the
 * object's code runs 52 bytes long and 40 of light1_step's 64 words
 * differ */
static void light1_register(struct task *task)
{
}

void light1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIGHT_INIT:
            light1_init(task);
            break;
        case LIGHT_RUN:
            light1_draw(task);
            light1_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case LIGHT_INIT:
            light1_init(task);
            break;
        case LIGHT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            light1_draw(task);
            light1_register(task);
            break;
        }
    }
}

static void light1_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = light1_draw;
    task->destructor = light1_destroy;
    work->mode = LIGHT_RUN;
}

static void light1_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateZ, work->ang.z);
        draw_tree(&light1_object);
        njPopMatrix(1);
    }
}

static void light1_destroy(struct task *task)
{
}
