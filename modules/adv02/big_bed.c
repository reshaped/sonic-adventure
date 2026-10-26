/* the bed the object list names O BIG BED, which Big can pick up and carry */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/big_bed.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/common.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT big_bed_object;

enum big_bed_mode { BED_INIT, BED_HELD, BED_DOWN };

/* FALSE until a player first lifts the bed, and TRUE from then on */
#define BED_LIFTED(work) ((work)->submode)

/* the bed draws RISE_UNLIFTED above its position until a player first lifts
 * it. it draws RISE_HELD above it while a player holds it, and at its
 * position otherwise and while an event has control */
#define RISE_UNLIFTED 10.0F
#define RISE_HELD (-5.0F)

struct big_bed_vars {
    Uint8 unk_00[8];
    Float rise; /* added to pos.y as the bed draws */
};

#define VARS(work) ((struct big_bed_vars *)&(work)->vars)

static void big_bed_draw(struct task *task);

static struct collision_shape big_bed_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     15.0F,
     10.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 10.0F, 0.0F},
     12.0F,
     5.0F,
     0.0F,
     {0},
     0,
     NULL},
};

void big_bed_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    if (placement_try_leave(task)) {
        return;
    }
    if (event_in_control()) {
        VARS(work)->rise = 0.0F;
        big_bed_draw(task);
        return;
    }
    switch (work->mode) {
    case BED_INIT:
        BED_LIFTED(work) = FALSE;
        collision_init(task, big_bed_shapes, COLLISION_OBJECT, 1, 2);
        carry_set_kind(work, motion, CARRY_KIND_LARGE);
        task->draw = big_bed_draw;
        work->mode = BED_DOWN;
        break;
    case BED_DOWN:
        carry_step_large(task);
        if (work->flags & CARRY_HELD) {
            BED_LIFTED(work) = TRUE;
            work->mode = BED_HELD;
        } else {
            VARS(work)->rise = !BED_LIFTED(work) ? RISE_UNLIFTED : 0.0F;
        }
        break;
    case BED_HELD:
        carry_step_large(task);
        if (work->flags & CARRY_HELD) {
            VARS(work)->rise = RISE_HELD;
        } else {
            work->mode = BED_DOWN;
        }
        break;
    }
    big_bed_draw(task);
}

static void big_bed_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->rise, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&big_bed_object);
    njPopMatrix(1);
}
