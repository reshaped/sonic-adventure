/* CC3, the pinball tables' camera, which eases between nine views */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "stg09/pinball.h"
#include "stg09/pinball_camera.h"
#include "system/task.h"

/* the share of the way to the view the camera moves each step */
#define VIEW_EASE 0.125F
/* work->tick for no view, so that the first run takes pinball_view's */
#define VIEW_NONE 0xff
/* a view's coordinates: its place's three and its target's */
#define VIEW_COORDINATES 6

/* a view's camera place, and the point it looks at */
struct table_view {
    NJS_POINT3 pos;
    NJS_POINT3 target;
};

/* the views pinball_view names, by number */
static struct table_view table_views[9] = {
#include "stg09/pinball_camera/table_views.inc"
};

/* CC3's step after the first. once pinball_view changes, the view's place
 * goes to the work's scl and its target to the vars' target. while
 * PINBALL_VIEW_MOVING is set, camera_work's place and target ease toward
 * them. work->tick holds the pinball_view the run last took */
static void pinball_camera_run(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 view = pinball_view;
    Uint8 arrived;
    Float step;

    if (work->tick != view) {
        work->tick = view;
        view &= ~PINBALL_VIEW_MOVING;
        work->scl.x = table_views[view].pos.x;
        work->scl.y = table_views[view].pos.y;
        work->scl.z = table_views[view].pos.z;
        CAMERA_VARS(work)->target.x = table_views[view].target.x;
        CAMERA_VARS(work)->target.y = table_views[view].target.y;
        CAMERA_VARS(work)->target.z = table_views[view].target.z;
    } else if (view & PINBALL_VIEW_MOVING) {
        arrived = 0;
        step = (camera_work->scl.x - work->scl.x) * VIEW_EASE;
        /* @bug logic: each of the six arrival tests compares its step with
         * camera_work->scl.x, whichever coordinate it eases (FCMP/EQ with the
         * word at camera_work + 44, 0x0c900cdc to 0x0c900dd4). near the view,
         * every step nears 0. scl.x nears the view's x, which is 9 or more
         * from 0 in every view. the tests fail there, so PINBALL_VIEW_MOVING
         * stays set */
        if (camera_work->scl.x != step) {
            camera_work->scl.x -= step;
        } else {
            camera_work->scl.x = work->scl.x;
            ++arrived;
        }
        step = (camera_work->scl.y - work->scl.y) * VIEW_EASE;
        if (camera_work->scl.x != step) {
            camera_work->scl.y -= step;
        } else {
            camera_work->scl.y = work->scl.y;
            ++arrived;
        }
        step = (camera_work->scl.z - work->scl.z) * VIEW_EASE;
        if (camera_work->scl.x != step) {
            camera_work->scl.z -= step;
        } else {
            camera_work->scl.z = work->scl.z;
            ++arrived;
        }
        step =
            (CAMERA_VARS(camera_work)->target.x - CAMERA_VARS(work)->target.x) *
            VIEW_EASE;
        if (camera_work->scl.x != step) {
            CAMERA_VARS(camera_work)->target.x -= step;
        } else {
            CAMERA_VARS(camera_work)->target.x = CAMERA_VARS(work)->target.x;
            ++arrived;
        }
        step =
            (CAMERA_VARS(camera_work)->target.y - CAMERA_VARS(work)->target.y) *
            VIEW_EASE;
        if (camera_work->scl.x != step) {
            CAMERA_VARS(camera_work)->target.y -= step;
        } else {
            CAMERA_VARS(camera_work)->target.y = CAMERA_VARS(work)->target.y;
            ++arrived;
        }
        step =
            (CAMERA_VARS(camera_work)->target.z - CAMERA_VARS(work)->target.z) *
            VIEW_EASE;
        if (camera_work->scl.x != step) {
            CAMERA_VARS(camera_work)->target.z -= step;
        } else {
            CAMERA_VARS(camera_work)->target.z = CAMERA_VARS(work)->target.z;
            ++arrived;
        }
        if (arrived == VIEW_COORDINATES) {
            pinball_view &= ~PINBALL_VIEW_MOVING;
            work->tick = pinball_view;
        }
    }
    task_step_children(task);
}

void pinball_camera_step(struct task *task)
{
    struct task_work *work = task->work;

    camera_override_mode(CAMERA_MODE_RUIN_WAKA1, 0);
    work->tick = VIEW_NONE;
    pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_TABLE;
    camera_work->scl.x = work->scl.x;
    camera_work->scl.y = work->scl.y;
    camera_work->scl.z = work->scl.z;
    CAMERA_VARS(camera_work)->target.x = work->pos.x;
    CAMERA_VARS(camera_work)->target.y = work->pos.y;
    CAMERA_VARS(camera_work)->target.z = work->pos.z;
    task->step = pinball_camera_run;
}
