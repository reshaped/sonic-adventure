/* Gate, which rises GATE_RISE above where the set file puts it and sinks
 * back, over and over */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg07/gate.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL ruin_gate_model;

struct gate_vars {
    Float rest; /* the height it starts at, the lowest it goes */
};

#define VARS(work) ((struct gate_vars *)&(work)->vars)

/* the gate moves GATE_SPEED each time it runs, up to GATE_RISE above its
 * rest */
#define GATE_SPEED 1.0F
#define GATE_RISE 45.0F

/* in the gate's work->flags: the way it moves. the gate clears the flags'
 * high byte as it starts, and again as it reaches either end. a run that
 * finds neither bit set doesn't move the gate. it sets GATE_RISING if the
 * gate is at its rest, and GATE_SINKING if not */
#define GATE_RISING 0x100
#define GATE_SINKING 0x200

/* clang-format off */
static struct collision_shape ruin_gate_shape = {
    SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 25.0F, 0.0F},
    2.0F, 25.0F, 30.0F, {0}, 0, NULL,
};
/* clang-format on */

static void ruin_gate_destroy(struct task *task)
{
}

/* move the gate and draw it. unless the set editor holds the gate, register
 * its collision first. this is the task's draw too, so the gate moves while
 * the game is paused */
static void ruin_gate_run(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    if (work->flags & (GATE_RISING | GATE_SINKING)) {
        if (work->flags & GATE_RISING) {
            work->pos.y += GATE_SPEED;
            if (work->pos.y > VARS(work)->rest + GATE_RISE) {
                work->pos.y = VARS(work)->rest + GATE_RISE;
                work->flags = (Uint8)work->flags;
            }
        } else {
            work->pos.y -= GATE_SPEED;
            if (work->pos.y < VARS(work)->rest) {
                work->pos.y = VARS(work)->rest;
                work->flags = (Uint8)work->flags;
            }
        }
    } else {
        if (work->pos.y == VARS(work)->rest) {
            /* @hack spelling: the clear bit added, ADD at 0x0c901ab0; or'd,
             * it takes an OR, and 1 of 137 words differ */
            work->flags += GATE_RISING;
        } else {
            /* @hack spelling: the clear bit added, ADD at 0x0c901ab6; or'd,
             * it takes an OR, and 1 of 137 words differ */
            work->flags += GATE_SINKING;
        }
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_model(&ruin_gate_model);
    njPopMatrix(1);
}

void ruin_gate_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &ruin_gate_shape, COLLISION_OBJECT, 1, 1);
    work->flags = (Uint8)work->flags;
    VARS(work)->rest = work->pos.y;
    task->step = ruin_gate_run;
    task->draw = ruin_gate_run;
    task->destructor = ruin_gate_destroy;
}
