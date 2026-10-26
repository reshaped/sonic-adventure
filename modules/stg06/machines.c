/* the machines O MEKA and O TUTU: two pieces of the deck's machinery. each
 * plays its action over and over while it is in view */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg06/machines.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION meka_action;
extern NJS_ACTION tutu_action;
extern NJS_OBJECT meka_object;
extern NJS_OBJECT tutu_object;

/* the square of the distance from the camera or player 0 within which a
 * machine stays */
#define MACHINE_RANGE (2010.0F * 2010.0F)
/* the radius draw_in_view tests a machine with */
#define MACHINE_VIEW 50.0F
/* the frame an O MEKA starts at when its set file's scale x is below 0. it
 * then plays out of step with one that starts at frame 0 */
#define MEKA_OFFSET 15.0F

/* in MACHINE_STOP a machine's step neither draws it nor plays its action, and
 * no step here sets it */
enum machine_mode { MACHINE_INIT, MACHINE_RUN, MACHINE_STOP };

/* draw the machine's action at its frame while it is in view. unless the set
 * editor holds the machine, step the frame on, back to 0 after the last.
 * task->draw runs this too, so the action plays while the game is paused */
static void machine_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, MACHINE_VIEW)) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        draw_action(ACTION(work)->action, ACTION(work)->frame);
        njPopMatrix(1);
        if (!placement_editing(task)) {
            /* @hack spelling: the frame stepped in the test; stepped ahead of
             * it, 14 of 74 words differ and the function ends 2 bytes late */
            if ((ACTION(work)->frame += 1.0F) >=
                (Float)ACTION(work)->action->motion->nbFrame) {
                ACTION(work)->frame = 0.0F;
            }
        }
    }
}

/* as the destructor, task_kill comes to nothing, since task_destroy then
 * frees the task */
static void machine_destroy(struct task *task)
{
    task_kill(task);
}

void meka_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, MACHINE_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&meka_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case MACHINE_INIT:
            work->mode = MACHINE_RUN;
            ACTION(work)->action = &meka_action;
            task->destructor = machine_destroy;
            task->draw = machine_draw;
            if (work->scl.x < 0.0F) {
                ACTION(work)->frame = MEKA_OFFSET;
            }
            break;
        case MACHINE_RUN:
            machine_draw(task);
            break;
        case MACHINE_STOP:
            break;
        }
    }
}

void tutu_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, MACHINE_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&tutu_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case MACHINE_INIT:
            work->mode = MACHINE_RUN;
            ACTION(work)->action = &tutu_action;
            task->destructor = machine_destroy;
            task->draw = machine_draw;
            break;
        case MACHINE_RUN:
            machine_draw(task);
            break;
        case MACHINE_STOP:
            break;
        }
    }
}
