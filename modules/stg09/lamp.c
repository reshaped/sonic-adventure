/* O SLB, O SLG, O SLR and O SLY lamps, which play their actions in a loop.
 * the four are blue, green, red and yellow */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/lamp.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_ACTION lamp_blue_action;
extern NJS_ACTION lamp_green_action;
extern NJS_ACTION lamp_red_action;
extern NJS_ACTION lamp_yellow_action;

/* destroyed beyond about 800 units of the camera and player 0 */
#define RANGE ((800.0F * 800.0F) + 10.0F)
/* the radius of the sphere about a lamp that draw_in_view tests */
#define VIEW_RADIUS 220.0F

struct lamp_vars {
    Float frame; /* the action's, back to 0 at its motion's last */
    NJS_ACTION *action;
};

#define VARS(work) ((struct lamp_vars *)&(work)->vars)
#define FRAME_RATE(work) ((work)->scl.y) /* added to frame each step */

static struct collision_shape lamp_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 4.0F, 0.0F},
    8.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void lamp_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(VARS(work)->action, VARS(work)->frame);
    njPopMatrix(1);
}

static void lamp_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (VARS(work)->frame >= VARS(work)->action->motion->nbFrame - 1) {
        VARS(work)->frame = 0.0F;
    }
    lamp_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "Mot Pitch");
        return;
    }
    collision_register(work);
    VARS(work)->frame += FRAME_RATE(work);
}

static void lamp_init(struct task *task, NJS_ACTION *action)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_init(task, &lamp_shape, COLLISION_OBJECT, 1, 1);
    }
    task->draw = lamp_draw;
    task->step = lamp_run;
    VARS(work)->frame = 0.0F;
    VARS(work)->action = action;
}

void lamp_blue_step(struct task *task)
{
    lamp_init(task, &lamp_blue_action);
}

void lamp_green_step(struct task *task)
{
    lamp_init(task, &lamp_green_action);
}

void lamp_red_step(struct task *task)
{
    lamp_init(task, &lamp_red_action);
}

void lamp_yellow_step(struct task *task)
{
    lamp_init(task, &lamp_yellow_action);
}
