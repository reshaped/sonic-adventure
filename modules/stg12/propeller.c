/* O PROPELLER, a propeller that loops its action over 100 frames */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "stg12/propeller.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION shelter_propeller_action;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the frames of the action's loop, which plays one a step */
#define FRAMES 100.0F

enum shelter_propeller_mode { PROPELLER_INIT, PROPELLER_RUN };

struct shelter_propeller_vars {
    Float frame;
};

#define VARS(work) ((struct shelter_propeller_vars *)&(work)->vars)

static void shelter_propeller_init(struct task *task);
static void shelter_propeller_draw(struct task *task);
static void shelter_propeller_destroy(struct task *task);
static void shelter_propeller_spin(struct task_work *work);

/* no code reads this shape, but the image holds it, as this file's only
 * data */
static struct collision_shape shelter_propeller_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    17.0F,
    1.5F,
    0.0F,
    {0},
    0,
    NULL,
};

void shelter_propeller_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case PROPELLER_INIT:
            shelter_propeller_init(task);
            break;
        case PROPELLER_RUN:
            shelter_propeller_spin(work);
            shelter_propeller_draw(task);
            shelter_propeller_destroy(task);
            break;
        /* @hack redundant: an empty default, which ends the run case with a BRA
         * past the switch and a NOP in its slot (0x0c9052dc, 0x0c9052de);
         * without it, the case's last MOV takes the slot, and 40 of 65 words
         * differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case PROPELLER_INIT:
            shelter_propeller_init(task);
            break;
        case PROPELLER_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            shelter_propeller_draw(task);
            shelter_propeller_destroy(task);
            break;
        }
    }
}

static void shelter_propeller_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shelter_propeller_draw;
    task->destructor = shelter_propeller_destroy;
    VARS(work)->frame = 0.0F;
    work->mode = PROPELLER_RUN;
}

static void shelter_propeller_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_action(&shelter_propeller_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void shelter_propeller_destroy(struct task *task)
{
    /* @hack redundant: a read of task->work, which leaves a copy of task in R4
     * where shelter_propeller_step calls the destructor (MOV R14,R4 at
     * 0x0c9052da and 0x0c905318); with an empty body, the step runs 4 bytes
     * shorter and 41 of its 65 words differ */
    (void)task->work;
}

static void shelter_propeller_spin(struct task_work *work)
{
    VARS(work)->frame += 1.0F;
    if (VARS(work)->frame >= FRAMES) {
        VARS(work)->frame = 0.0F;
    }
}
