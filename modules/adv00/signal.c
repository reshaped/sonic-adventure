/* O SIGNAL solid signal, which cycles through three timed phases */
#include <ninja.h>

#include "adv00/signal.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: signal_object as an array of one, which has SHC load its
 * address into R6 once, before the edit_mode test, for both branches
 * (MOV.L @(0x0c904680),R6 at 0x0c9045de). as one object, 31 of 111 words
 * differ */
extern NJS_OBJECT signal_object[1];

enum signal_mode {
    SIGNAL_START,
    SIGNAL_PHASE_1,
    SIGNAL_PHASE_2,
    SIGNAL_PHASE_3
};

struct signal_vars {
    NJS_OBJECT *object; /* the model it draws */
    NJS_OBJECT *source; /* the model the step points object at */
};

#define VARS(work) ((struct signal_vars *)&(work)->vars)

/* each phase runs until its timer, which counts its steps, passes its end */
#define PHASE_1_END 1600
#define PHASE_2_END 200
#define PHASE_3_END 1800

/* clang-format off */
static struct collision_shape signal_shape[] = {
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 0.0F, 0.0F},
     1.0F, 31.6F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static void signal_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

static void signal_destroy(struct task *task)
{
    task_kill(task);
}

/* while the set editor is open, draw the signal, setting its model first if
 * it has none. otherwise start the signal on the first step, then run its
 * phases in turn */
void signal_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        if (!VARS(work)->source) {
            VARS(work)->source = signal_object;
        }
        if (!VARS(work)->object) {
            VARS(work)->object = VARS(work)->source;
        }
        signal_draw(task);
    } else {
        switch (work->mode) {
        case SIGNAL_START:
            work->mode = SIGNAL_PHASE_1;
            VARS(work)->source = signal_object;
            VARS(work)->object = VARS(work)->source;
            collision_init(task, signal_shape, COLLISION_OBJECT, 1, 1);
            task->destructor = signal_destroy;
            task->draw = signal_draw;
            break;
        case SIGNAL_PHASE_1:
            if (++TASK_TIMER(work) > PHASE_1_END) {
                work->mode = SIGNAL_PHASE_2;
                TASK_TIMER(work) = 0;
            }
            collision_register(work);
            signal_draw(task);
            break;
        case SIGNAL_PHASE_2:
            if (++TASK_TIMER(work) > PHASE_2_END) {
                work->mode = SIGNAL_PHASE_3;
                TASK_TIMER(work) = 0;
            }
            collision_register(work);
            signal_draw(task);
            break;
        case SIGNAL_PHASE_3:
            if (++TASK_TIMER(work) > PHASE_3_END) {
                work->mode = SIGNAL_PHASE_1;
                TASK_TIMER(work) = 0;
            }
            collision_register(work);
            signal_draw(task);
            break;
        }
    }
}
