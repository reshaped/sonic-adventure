/* O CONV_STOP swinging bar over the conveyor belts that hurts players */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "object/placement.h"
#include "stg10/conveyor_stop.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION conveyor_stop_action;

/* the stopper is destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
#define REACH 150.0F /* the collision record's radius */

#define FRAME_SPEED 0.4F /* the action's frames a step */
#define BAR_NODE 2 /* the bar's node in the action, counted from 0 */

struct stop_vars {
    Float frame;
};

#define VARS(work) ((struct stop_vars *)&(work)->vars)

enum stop_mode { STOP_INIT, STOP_RUN };

static void conveyor_stop_init(struct task *task);
static void conveyor_stop_reset(struct task *task);
static void conveyor_stop_swing(struct task *task);
static void conveyor_stop_draw(struct task *task);
static void conveyor_stop_destroy(struct task *task);

/* the bar's box, which hurts players */
static struct collision_shape conveyor_stop_shape = {
    SHAPE_RECTANGLE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
    SHAPE_HURTS_PLAYERS | GUARD(2) | ATTACK(2),
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    45.0F,
    44.5F,
    10.0F,
    {0},
    0,
    NULL,
};

/* the shape's center in the bar's local space */
static NJS_POINT3 conveyor_stop_bar_center = {0.0F, 19.0F, 5.0F};

/* the bar node's matrix in the action's current pose */
static NJS_MATRIX conveyor_stop_matrix;

/* the set editor labels all three angles, though only angle y turns the
 * stopper */
void conveyor_stop_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case STOP_INIT:
            conveyor_stop_init(task);
            break;
        case STOP_RUN:
            conveyor_stop_draw(task);
            conveyor_stop_swing(task);
            break;
        }
    } else {
        switch (work->mode) {
        case STOP_INIT:
            conveyor_stop_init(task);
            break;
        case STOP_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            conveyor_stop_draw(task);
            conveyor_stop_swing(task);
            break;
        }
    }
}

static void conveyor_stop_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = conveyor_stop_draw;
    task->destructor = conveyor_stop_destroy;
    conveyor_stop_reset(task);
    work->mode = STOP_RUN;
}

static void conveyor_stop_reset(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &conveyor_stop_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = REACH;
    VARS(work)->frame = 0.0F;
    work->collision->shapes->ang.y = work->ang.y;
}

/* advance the action, wrapping at its end, and put the bar's box where the
 * pose puts the bar */
static void conveyor_stop_swing(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    Angle ang;

    VARS(work)->frame += FRAME_SPEED;
    action = &conveyor_stop_action;
    if (VARS(work)->frame >= action->motion->nbFrame) {
        VARS(work)->frame -= action->motion->nbFrame;
    }
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    pose_action(NULL, &conveyor_stop_action, VARS(work)->frame, BAR_NODE,
                &conveyor_stop_matrix);
    njCalcPoint(&conveyor_stop_matrix, &conveyor_stop_bar_center,
                &work->collision->shapes->center);
    njPopMatrix(1);
    collision_register(work);
}

static void conveyor_stop_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(&conveyor_stop_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void conveyor_stop_destroy(struct task *task)
{
}
