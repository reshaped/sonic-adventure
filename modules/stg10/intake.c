/* O Suikomi intake model with a solid box at its foot */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg10/intake.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT intake_object;

enum intake_mode { INTAKE_INIT, INTAKE_RUN };

static void intake_init(struct task *task);
static void intake_draw(struct task *task);
static void intake_destroy(struct task *task);

static struct collision_shape intake_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 6.0F, 0.0F},
    6.0F,
    6.0F,
    8.0F,
    {0},
    0,
    NULL,
};

/* @hack spelling: collision_register in a helper SHC expands at each call,
 * which keeps task in a stack slot (MOV.L R14,@R15 at 0x0c910b60). with
 * collision_register(work) written in the step, the call becomes a jump: the
 * object's code runs 12 bytes short and 62 of intake_step's 76 words
 * differ */
static void intake_register(struct task *task)
{
    collision_register(task->work);
}

/* the intake does nothing while the camera is beyond its placement's leave
 * range, about 400. the set editor labels all three angles, though only angle
 * y turns the intake */
void intake_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_far_from_camera(task)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case INTAKE_INIT:
            intake_init(task);
            break;
        case INTAKE_RUN:
            intake_draw(task);
            intake_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case INTAKE_INIT:
            intake_init(task);
            break;
        case INTAKE_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            intake_draw(task);
            intake_register(task);
            break;
        }
    }
}

static void intake_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = intake_draw;
    task->destructor = intake_destroy;
    collision_init(task, &intake_shape, COLLISION_OBJECT, 1, 1);
    work->mode = INTAKE_RUN;
}

static void intake_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_tree(&intake_object);
        njPopMatrix(1);
    }
}

static void intake_destroy(struct task *task)
{
}
