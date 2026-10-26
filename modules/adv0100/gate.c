/* O A GATE pair of deck gates that swing open and shut at the command in
 * FLAG_GATE */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/gate.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION gate_action;

#define GATE_OPEN_FRAME 19.99F /* the action frame with the gates open */
#define GATE_SPEED 0.4F /* the action frames the gates swing each step */
/* the action frame below which the gates are shut, and solid */
#define GATE_SHUT_FRAME 0.1F

struct gate_vars {
    Uint8 unk_00[8];
    Float frame; /* the action's frame, from 0, shut, to GATE_OPEN_FRAME */
};

#define VARS(work) ((struct gate_vars *)&(work)->vars)

enum gate_mode {
    GATE_INIT,
    GATE_SHUT_STILL,
    GATE_OPENING,
    GATE_SHUTTING,
    GATE_OPEN_STILL
};

static void gate_draw(struct task *task);
static void gate_destroy(struct task *task);
static void gate_draw_work(struct task_work *work);
static void gate_obey(struct task_work *work);

static struct collision_shape gate_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, -20.0F, 0.0F},
     60.0F,
     20.5F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

void egg_carrier_gate_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    gate_obey(work);
    switch (work->mode) {
    case GATE_INIT:
        if (!placement_editing(task)) {
            collision_init(task, gate_shapes, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = gate_destroy;
        task->draw = gate_draw;
        work->mode = GATE_SHUT_STILL;
        break;
    case GATE_SHUT_STILL:
        break;
    case GATE_OPENING:
        VARS(work)->frame += GATE_SPEED;
        if (VARS(work)->frame < GATE_OPEN_FRAME) {
            break;
        }
        VARS(work)->frame = GATE_OPEN_FRAME;
        work->mode = GATE_OPEN_STILL;
        break;
    case GATE_SHUTTING:
        VARS(work)->frame -= GATE_SPEED;
        if (VARS(work)->frame > 0.0F) {
            break;
        }
        VARS(work)->frame = 0.0F;
        work->mode = GATE_SHUT_STILL;
        break;
    case GATE_OPEN_STILL:
        break;
    }
    gate_draw_work(work);
    if (!placement_editing(task) && VARS(work)->frame < GATE_SHUT_FRAME) {
        collision_register(work);
    }
}

static void gate_draw(struct task *task)
{
    struct task_work *work = task->work;

    gate_draw_work(work);
}

static void gate_destroy(struct task *task)
{
}

static void gate_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(&gate_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* take the command in FLAG_GATE, if any: set the mode and frame it names, and
 * clear the flag. on the gates' first step, before GATE_INIT has run, the
 * command waits in the flag */
static void gate_obey(struct task_work *work)
{
    if (work->mode == GATE_INIT) {
        return;
    }
    switch (event_flags[FLAG_GATE]) {
    case GATE_NO_COMMAND:
        return;
    case GATE_SHUT_NOW:
        work->mode = GATE_SHUT_STILL;
        VARS(work)->frame = 0.0F;
        break;
    case GATE_OPEN_NOW:
        work->mode = GATE_OPEN_STILL;
        VARS(work)->frame = GATE_OPEN_FRAME;
        break;
    case GATE_SHUT:
        work->mode = GATE_SHUTTING;
        VARS(work)->frame = GATE_OPEN_FRAME;
        break;
    case GATE_OPEN:
        work->mode = GATE_OPENING;
        VARS(work)->frame = 0.0F;
        break;
    }
    event_flags[FLAG_GATE] = GATE_NO_COMMAND;
}
