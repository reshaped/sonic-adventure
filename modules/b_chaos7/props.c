/* O CORN, O SIGNAL and O TIRE: the Perfect Chaos fight's props, which are only
 * drawn, and five more that nothing places */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/props.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT cone_object;
extern NJS_OBJECT prop_1_object;
extern NJS_OBJECT prop_2_object;
extern NJS_OBJECT prop_3_object;
extern NJS_OBJECT prop_4_object;
extern NJS_OBJECT prop_5_object;
extern NJS_OBJECT signal_object;
extern NJS_OBJECT tire_object;

/* a prop destroys itself once neither the camera nor player 0 is within 4010.
 * the range is squared, as placement_try_leave_by takes it */
#define PROP_RANGE (4010.0F * 4010.0F)

/* the prop's row of prop_objects, which each kind's step sets before
 * chaos7_prop_step runs */
#define PROP_KIND(work) ((work)->tick)
enum prop_kind_row {
    KIND_CONE,
    KIND_PROP_1,
    KIND_PROP_2,
    KIND_PROP_3,
    KIND_PROP_4,
    KIND_PROP_5,
    KIND_SIGNAL,
    KIND_TIRE
};

/* nothing sets PROP_GONE */
enum prop_mode { PROP_INIT, PROP_STANDING, PROP_GONE };

static NJS_OBJECT *prop_objects[8] = {
    &cone_object,   &prop_1_object, &prop_2_object, &prop_3_object,
    &prop_4_object, &prop_5_object, &signal_object, &tire_object,
};

static void chaos7_prop_destroy(struct task *task)
{
    task_kill(task);
}

static void chaos7_prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

/* the step every kind runs once it has set PROP_KIND. it draws the prop turned
 * by its z and y angles. in the set editor, it labels the three angle rows
 * ROTATION */
static void chaos7_prop_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 kind = PROP_KIND(work);
    /* @hack spelling: the table read through a pointer to the prop's row, which
     * SHC forms with ADD R5,R4 at 0x0c91c4e6; without it, SHC indexes the
     * table with MOV.L @(R0,R5),R3, and 14 of 104 words differ */
    NJS_OBJECT **object;

    if (placement_try_leave_by(task, PROP_RANGE)) {
        return;
    }
    if (edit_mode) {
        object = &prop_objects[kind];
        MODEL(work)->object = *object;
        chaos7_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), ">");
        }
    } else {
        switch (work->mode) {
        case PROP_INIT:
            work->mode = PROP_STANDING;
            work->scl.x = work->scl.y = work->scl.z = 0.0F;
            object = &prop_objects[kind];
            MODEL(work)->object = *object;
            task->destructor = chaos7_prop_destroy;
            task->draw = chaos7_prop_draw;
            break;
        case PROP_STANDING:
            chaos7_prop_draw(task);
            break;
        case PROP_GONE:
        default:
            placement_kill(task);
            break;
        }
    }
}

void chaos7_cone_step(struct task *task)
{
    struct task_work *work = task->work;

    PROP_KIND(work) = KIND_CONE;
    chaos7_prop_step(task);
}

void chaos7_prop_1_step(struct task *task)
{
    struct task_work *work = task->work;

    PROP_KIND(work) = KIND_PROP_1;
    chaos7_prop_step(task);
}

void chaos7_prop_2_step(struct task *task)
{
    struct task_work *work = task->work;

    PROP_KIND(work) = KIND_PROP_2;
    chaos7_prop_step(task);
}

void chaos7_prop_3_step(struct task *task)
{
    struct task_work *work = task->work;

    PROP_KIND(work) = KIND_PROP_3;
    chaos7_prop_step(task);
}

void chaos7_prop_4_step(struct task *task)
{
    struct task_work *work = task->work;

    PROP_KIND(work) = KIND_PROP_4;
    chaos7_prop_step(task);
}

void chaos7_prop_5_step(struct task *task)
{
    struct task_work *work = task->work;

    PROP_KIND(work) = KIND_PROP_5;
    chaos7_prop_step(task);
}

void chaos7_signal_step(struct task *task)
{
    struct task_work *work = task->work;

    PROP_KIND(work) = KIND_SIGNAL;
    chaos7_prop_step(task);
}

void chaos7_tire_step(struct task *task)
{
    struct task_work *work = task->work;

    PROP_KIND(work) = KIND_TIRE;
    chaos7_prop_step(task);
}
