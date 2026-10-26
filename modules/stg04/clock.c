/* O Tokei clock on a column with a ticking pendulum */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg04/clock.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL highway_clock_model;
extern NJS_MODEL highway_clock_pendulum_model;

/* the pendulum hangs PENDULUM_HEIGHT up the clock, and swings PENDULUM_SWING
 * angle units, about 5 degrees, either way. its phase gains PENDULUM_STEP a
 * step, a swing there and back in 64 steps */
#define PENDULUM_HEIGHT 45.0F
#define PENDULUM_SWING 896.0F
#define PENDULUM_STEP 0x400

/* sound effect as the pendulum passes the middle */
#define SE_TICK 117

/* CLOCK_SWINGING_BACK, a flag in work->flags, is set as the phase passes
 * QUARTER_TURN and cleared past THREE_QUARTER_TURN. past THREE_QUARTER_TURN it
 * flips each step. only the tests that set and clear it read it */
#define CLOCK_SWINGING_BACK 0x100
#define QUARTER_TURN NJM_DEG_ANG(90)
#define THREE_QUARTER_TURN NJM_DEG_ANG(270)

struct clock_vars {
    Uint8 unk_00[4];
    Angle swing; /* the pendulum's turn about x */
};

#define VARS(work) ((struct clock_vars *)&(work)->vars)

static void highway_clock_destroy(struct task *task);
static void highway_clock_run(struct task *task);

static struct collision_shape highway_clock_shapes[3] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.0F, 0.0F},
     11.0F,
     2.0F,
     11.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 14.0F, 0.0F},
     8.0F,
     12.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 38.5F, 0.0F},
     7.0F,
     17.5F,
     0.0F,
     {0},
     0,
     NULL},
};

static void highway_clock_destroy(struct task *task)
{
}

/* the clock's step and draw */
static void highway_clock_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(&highway_clock_model);
    njTranslate(NULL, 0.0F, PENDULUM_HEIGHT, 0.0F);
    TURN(njRotateX, VARS(work)->swing);
    draw_model(&highway_clock_pendulum_model);
    njPopMatrix(1);
    work->timer += PENDULUM_STEP;
    VARS(work)->swing = (Angle)(njSin(TASK_TIMER(work)) * PENDULUM_SWING);
    if (!(work->flags & CLOCK_SWINGING_BACK)) {
        if (TASK_TIMER(work) > QUARTER_TURN) {
            work->flags |= CLOCK_SWINGING_BACK;
        }
    } else {
        if (TASK_TIMER(work) > THREE_QUARTER_TURN) {
            work->flags &= ~CLOCK_SWINGING_BACK;
        }
    }
    if (!(TASK_TIMER(work) & 0x7fff)) {
        sound_play_at(SE_TICK, 0, 0, -5, work->pos.x, work->pos.y, work->pos.z);
    }
}

void highway_clock_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, highway_clock_shapes, COLLISION_OBJECT, 1, 3);
    work->timer = 0;
    work->flags &= ~CLOCK_SWINGING_BACK;
    task->destructor = highway_clock_destroy;
    task->step = highway_clock_run;
    task->draw = highway_clock_run;
}
