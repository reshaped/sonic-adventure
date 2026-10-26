/* O LIGHT4: a standing lamp whose parts loop their 200-frame motion, a
 * frame each step */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "stg12/light_4.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: declared an array, not the one NJS_ACTION it is, so SHC
 * pools its address at 0x0c9091a0 and reads motion through it, MOV.L
 * @(4,R3),R2 at 0x0c90917a; declared as one, 3 of shelter_light_4_animate's
 * 24 words and a pool word differ */
extern NJS_ACTION shelter_light_4_action[];

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum shelter_light_4_mode { LIGHT_INIT, LIGHT_RUN };

struct shelter_light_4_vars {
    Float frame;
};

#define VARS(work) ((struct shelter_light_4_vars *)&(work)->vars)

static void shelter_light_4_init(struct task *task);
static void shelter_light_4_draw(struct task *task);
static void shelter_light_4_destroy(struct task *task);
static void shelter_light_4_animate(struct task *task);

/* a shape no code reads, whose bytes the image holds */
static struct collision_shape shelter_light_4_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, -1.0F},
    23.0F,
    20.5F,
    3.0F,
    {0},
    0,
    NULL,
};

void shelter_light_4_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIGHT_INIT:
            shelter_light_4_init(task);
            break;
        case LIGHT_RUN:
            shelter_light_4_draw(task);
            shelter_light_4_animate(task);
            break;
        }
    } else {
        switch (work->mode) {
        case LIGHT_INIT:
            shelter_light_4_init(task);
            break;
        case LIGHT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            shelter_light_4_draw(task);
            shelter_light_4_destroy(task);
            break;
        }
    }
}

static void shelter_light_4_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shelter_light_4_draw;
    task->destructor = shelter_light_4_destroy;
    VARS(work)->frame = 0.0F;
    work->mode = LIGHT_RUN;
}

static void shelter_light_4_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(shelter_light_4_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

/* the destructor, which the step also runs after each draw while the set
 * editor holds the lamp. it frees nothing */
static void shelter_light_4_destroy(struct task *task)
{
    /* @hack redundant: a read of task->work, which keeps the copy of task into
     * R4 at 0x0c909078, where the step expands this function; without it, 11
     * of shelter_light_4_step's 65 words differ */
    (void)task->work;
}

static void shelter_light_4_animate(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame += 1.0F;
    if (VARS(work)->frame >= shelter_light_4_action->motion->nbFrame) {
        VARS(work)->frame = 0.0F;
    }
}
