/* O DOSEI, a ringed planet whose top turns about y once every 128 steps */
#include <ninja.h>

#include "adv0100/saturn.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL saturn_model;
extern NJS_MODEL saturn_top_model;

/* the top's turn each step, and its turn after the steps work->timer counts */
#define TOP_SPEED 0x200
#define TOP_TURN(work) (TASK_TIMER(work) * TOP_SPEED)

enum saturn_mode { SATURN_INIT, SATURN_RUN };

static void saturn_init(struct task *task);
static void saturn_draw(struct task *task);
static void saturn_collision_init(struct task *task);
static void saturn_destroy(struct task *task);

static struct collision_shape saturn_shapes[] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 14.0F, 0.0F},
     9.0F,
     14.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* draw the planet, register its collision and advance its top's turn. while
 * the set editor holds the task, label the editor's angle rows too */
void egg_carrier_saturn_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case SATURN_INIT:
            saturn_init(task);
            break;
        case SATURN_RUN:
            saturn_draw(task);
            collision_register(task->work);
            ++task->work->timer;
            break;
        /* @hack redundant: a default that returns, where the switch's end would
         * do the same, which leaves the BRA's delay slot at 0x0c925218
         * empty after the timer's store; without it, SHC fills the slot
         * with the store and 46 of 68 words differ */
        default:
            return;
        }
    } else {
        switch (work->mode) {
        case SATURN_INIT:
            saturn_init(task);
            break;
        case SATURN_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            saturn_draw(task);
            collision_register(task->work);
            ++task->work->timer;
            break;
        }
    }
}

static void saturn_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = saturn_draw;
    task->destructor = saturn_destroy;
    saturn_collision_init(task);
    work->mode = SATURN_RUN;
}

static void saturn_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&saturn_model);
    TURN_VIA(njRotateY, TOP_TURN(work), ang);
    draw_model(&saturn_top_model);
    njPopMatrix(1);
}

static void saturn_collision_init(struct task *task)
{
    collision_init(task, saturn_shapes, COLLISION_OBJECT, 1, 1);
    task->work->timer = 0;
}

static void saturn_destroy(struct task *task)
{
}
