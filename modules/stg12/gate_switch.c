/* O GATE SW: a gate that rises out of the way once its switch is on */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/switch.h"
#include "stg12/gate_switch.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT gate_2_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the gate's unscaled half width and half height */
#define GATE_WIDTH 41.0F
#define GATE_HEIGHT 31.0F

#define RISE_SPEED 0.2F

enum gate_switch_mode { GATE_SHUT, GATE_RISING, GATE_OPEN };

struct gate_switch_vars {
    Float top; /* the y the gate rises to, its own height above its start */
    int switch_id;
};

#define VARS(work) ((struct gate_switch_vars *)&(work)->vars)

static void gate_switch_start(struct task *task);
static void gate_switch_run(struct task *task);
static void gate_switch_draw(struct task *task);
static void gate_switch_destroy(struct task *task);

/* a box the gate's size, which gate_switch_start resizes by the set entry's
 * scale */
static struct collision_shape gate_switch_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, GATE_HEIGHT, 0.0F},
    GATE_WIDTH,
    GATE_HEIGHT,
    1.2F,
    {0},
    0,
    NULL,
};

void gate_switch_step(struct task *task)
{
    struct task_work *work = task->work;

    gate_switch_start(task);
    work->mode = GATE_SHUT;
    task->step = gate_switch_run;
    task->draw = gate_switch_draw;
    task->destructor = gate_switch_destroy;
}

static void gate_switch_start(struct task *task)
{
    struct task_work *work = task->work;
    Float width;
    Float height;
    Float y;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    width = (work->scl.x + 1.0F) * GATE_WIDTH;
    height = (work->scl.y + 1.0F) * GATE_HEIGHT;
    y = work->pos.y;
    VARS(work)->switch_id = (int)njAbs(work->scl.z);
    VARS(work)->top = (height * 2.0F) + y;
    if (!placement_editing(task)) {
        collision_init(task, &gate_switch_shape, COLLISION_OBJECT, 1, 1);
        work->collision->shapes->radius = width;
        work->collision->shapes->size_y = height + 2.5F;
        work->collision->shapes->center.y = height;
        work->collision->radius =
            njSqrt((width * width) + (height * height)) + 20.0F;
    }
}

static void gate_switch_run(struct task *task)
{
    struct task_work *work = task->work;
    int id;

    if (placement_editing(task)) {
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCALE  X");
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> SCALE  Y");
        id = (int)njAbs(work->scl.z);
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> SWITCH ID:%d", id);
    } else {
        switch (work->mode) {
        case GATE_SHUT:
            if (switch_is_on(VARS(work)->switch_id)) {
                work->mode = GATE_RISING;
            }
            break;
        case GATE_RISING:
            work->pos.y += RISE_SPEED;
            if (work->pos.y >= VARS(work)->top) {
                work->pos.y = VARS(work)->top;
                work->mode = GATE_OPEN;
            }
            break;
        case GATE_OPEN:
            break;
        }
        /* @hack spelling: task->work read again, MOV.L @(32,R13),R4 at
         * 0x0c91492a, which keeps task in R13 and debug_print's address in R12;
         * through work, the two trade and 10 of 79 words differ */
        collision_register(task->work);
    }
    gate_switch_draw(task);
}

static void gate_switch_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F, 1.0F);
        draw_tree(&gate_2_object);
        njPopMatrix(1);
    }
}

static void gate_switch_destroy(struct task *task)
{
}
