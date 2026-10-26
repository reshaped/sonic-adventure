/* the wheel the object list names O ROTARY, which turns a third of a turn
 * each time ROTARY_TURN is set in its event flag */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/rotary.h"
#include "draw/model.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address and reads model through it (MOV.L @(4,R4),R4 at 0x0c92585c);
 * declared as one, 2 of rotary_draw's 34 words and a pool word differ */
extern NJS_OBJECT rotary_object[];

/* a turn's angle, a third of a full one, and the angle it makes each step,
 * a 256th */
#define TURN_ANGLE 0x5555
#define TURN_SPEED 0x100

struct rotary_vars {
    Uint32 to; /* the angle the turn ends at */
    Uint8 unk_04[4];
    Uint32 turned; /* the wheel's angle past its set angle, which only grows */
};

#define VARS(work) ((struct rotary_vars *)&(work)->vars)

enum rotary_mode { ROTARY_INIT, ROTARY_WAIT, ROTARY_TURNING };

static void rotary_draw(struct task_work *work);

void egg_carrier_rotary_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case ROTARY_INIT:
        event_flags[FLAG_ROTARY] = 0;
        work->mode = ROTARY_WAIT;
        break;
    case ROTARY_WAIT:
        if (event_flags[FLAG_ROTARY] & ROTARY_TURN) {
            event_flags[FLAG_ROTARY] &= ~ROTARY_DONE;
            VARS(work)->to = VARS(work)->turned + TURN_ANGLE;
            work->mode = ROTARY_TURNING;
        }
        break;
    case ROTARY_TURNING:
        VARS(work)->turned += TURN_SPEED;
        if (VARS(work)->turned < VARS(work)->to) {
            break;
        }
        VARS(work)->turned = VARS(work)->to;
        event_flags[FLAG_ROTARY] &= ~ROTARY_TURN;
        event_flags[FLAG_ROTARY] |= ROTARY_DONE;
        work->mode = ROTARY_WAIT;
        break;
    }
    rotary_draw(work);
}

static void rotary_draw(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y + VARS(work)->turned, ang);
    draw_model(rotary_object->model);
    njPopMatrix(1);
}
