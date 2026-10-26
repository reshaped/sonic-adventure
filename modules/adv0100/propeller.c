/* O PROPELLER, a fan that turns about y once every 128 steps */
#include <ninja.h>

#include "adv0100/propeller.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL propeller_model;

/* the fan's turn each step, and its turn after the steps work->timer counts */
#define SPIN_SPEED 0x200
#define SPIN(work) (TASK_TIMER(work) * SPIN_SPEED)

enum propeller_mode { PROPELLER_INIT, PROPELLER_RUN };

static void propeller_init(struct task *task);
static void propeller_draw(struct task *task);
static void propeller_destroy(struct task *task);

/* draw the fan, register its collision and advance its turn. while the set
 * editor holds the task, label the editor's angle rows too */
void egg_carrier_propeller_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case PROPELLER_INIT:
            propeller_init(task);
            break;
        case PROPELLER_RUN:
            propeller_draw(task);
            collision_register(task->work);
            ++task->work->timer;
            break;
        /* @hack redundant: a default that returns, where the switch's end would
         * do the same, which leaves the BRA's delay slot at 0x0c92a098
         * empty after the timer's store; without it, SHC fills the slot
         * with the store and 46 of 68 words differ */
        default:
            return;
        }
    } else {
        switch (work->mode) {
        case PROPELLER_INIT:
            propeller_init(task);
            break;
        case PROPELLER_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            propeller_draw(task);
            collision_register(task->work);
            task->work->timer++;
            break;
        }
    }
}

static void propeller_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = propeller_draw;
    task->destructor = propeller_destroy;
    task->work->timer = 0;
    work->mode = PROPELLER_RUN;
}

static void propeller_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y + SPIN(work), ang);
    draw_model(&propeller_model);
    njPopMatrix(1);
}

static void propeller_destroy(struct task *task)
{
}
