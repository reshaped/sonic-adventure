/* O TATEKAN swaying pipe between two turning caps */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg10/texture.h"
#include "stg10/vertical_pipe.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT vertical_pipe_column_object;
extern NJS_OBJECT vertical_pipe_top_object;
extern NJS_OBJECT vertical_pipe_bottom_object;
extern NJS_OBJECT vertical_pipe_surface_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* each sway's phase turns this far a step, so a whole sway takes 256 steps */
#define SWAY_STEP 0x100
/* the top cap's height above the bottom cap at a scale y of 1 */
#define COLUMN_HEIGHT 22.0F
/* the column's base above the bottom cap. scale y stretches the column from
 * there */
#define COLUMN_BASE 4.0F

/* the pipe sways along y and z about home_y and home_z, where it was placed.
 * the sways' phases are its angles x and z. angle x also turns the bottom
 * cap, and angle z the top cap */
struct vertical_pipe_vars {
    Uint8 unk_00[4];
    Float home_z;
    Float home_y;
};

#define VARS(work) ((struct vertical_pipe_vars *)&(work)->vars)

enum vertical_pipe_mode { PIPE_INIT, PIPE_RUN };

static void vertical_pipe_init(struct task *task);
static void vertical_pipe_sway(struct task *task);
static void vertical_pipe_draw(struct task *task);
static void vertical_pipe_place(struct task *task);
static void vertical_pipe_destroy(struct task *task);

/* a cylinder 20 units across, reaching COLUMN_HEIGHT above and below the
 * bottom cap at any scale y. it pushes every group, and no group's shapes push
 * it */
static struct collision_shape vertical_pipe_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    10.0F,
    COLUMN_HEIGHT,
    0.0F,
    {0},
    0,
    NULL,
};

/* the set entry's angle x and scale x start and size the sway along y.
 * angle z and scale z do the same along z. scale y stretches the column and
 * raises the top cap to match */
void vertical_pipe_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case PIPE_INIT:
            vertical_pipe_init(task);
            break;
        case PIPE_RUN:
            vertical_pipe_draw(task);
            vertical_pipe_sway(task);
            break;
        }
    } else {
        switch (work->mode) {
        case PIPE_INIT:
            vertical_pipe_init(task);
            break;
        case PIPE_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> START Y POS");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> START Z POS");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> Y MOVE SIZE");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> OBJ SIZE");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> Z MOVE SIZE");
            vertical_pipe_draw(task);
            vertical_pipe_sway(task);
            break;
        }
    }
}

/* the surface shows Final Egg's animated texture, which the pipe holds until
 * vertical_pipe_destroy releases it */
static void vertical_pipe_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = vertical_pipe_draw;
    task->destructor = vertical_pipe_destroy;
    vertical_pipe_place(task);
    final_texture_hold();
    work->mode = PIPE_RUN;
}

static void vertical_pipe_sway(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x += SWAY_STEP;
    work->ang.z -= SWAY_STEP;
    work->pos.y = VARS(work)->home_y + (njSin(work->ang.x) * work->scl.x);
    work->pos.z = VARS(work)->home_z + (njSin(work->ang.z) * work->scl.z);
    collision_register(work);
}

static void vertical_pipe_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njPushMatrix(NULL);
        TURN(njRotateY, work->ang.x);
        draw_tree(&vertical_pipe_bottom_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, work->scl.y * COLUMN_HEIGHT, 0.0F);
        TURN(njRotateY, work->ang.z);
        draw_tree(&vertical_pipe_top_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, COLUMN_BASE, 0.0F);
        njScale(NULL, 1.0F, work->scl.y, 1.0F);
        draw_tree(&vertical_pipe_column_object);
        njSetTexture(&final_texture_texlist);
        draw_tree(&vertical_pipe_surface_object);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void vertical_pipe_place(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &vertical_pipe_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->home_y = work->pos.y;
    VARS(work)->home_z = work->pos.z;
}

static void vertical_pipe_destroy(struct task *task)
{
    final_texture_release();
}
