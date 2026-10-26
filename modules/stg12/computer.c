/* O COMPUTER, a solid computer whose parts loop a motion */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "stg12/computer.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the action declared as an array, so SHC pools only its address,
 * at 0x0c907854, and computer_animate reads its motion through it at
 * 0x0c907818. declared as one, SHC pools the motion field's address too. then 4
 * of computer_animate's 24 words differ, as do 3 of the 35 words of its pool
 * and pad */
extern NJS_ACTION computer_action[];
extern NJS_OBJECT computer_world_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum computer_mode { COMPUTER_INIT, COMPUTER_RUN };

struct computer_vars {
    Float frame;
    Uint8 unk_04[4];
    NJS_OBJECT *body; /* the computer's object in the collision world */
};

#define VARS(work) ((struct computer_vars *)&(work)->vars)

static void computer_init(struct task *task);
static void computer_draw(struct task *task);
static void computer_destroy(struct task *task);
static void computer_animate(struct task *task);
static void computer_edit(struct task *task);

void computer_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case COMPUTER_INIT:
            computer_init(task);
            break;
        case COMPUTER_RUN:
            computer_draw(task);
            computer_animate(task);
            break;
        }
    } else {
        switch (work->mode) {
        case COMPUTER_INIT:
            computer_init(task);
            break;
        case COMPUTER_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            computer_draw(task);
            computer_edit(task);
            break;
        }
    }
}

static void computer_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *body;

    task->draw = computer_draw;
    task->destructor = computer_destroy;
    VARS(work)->body = body = world_object_alloc();
    body->evalflags = computer_world_object.evalflags;
    body->scl[0] = 1.0F;
    body->scl[1] = 1.0F;
    body->scl[2] = 1.0F;
    body->ang[0] = work->ang.x;
    body->ang[1] = work->ang.y;
    body->ang[2] = work->ang.z;
    body->pos[0] = work->pos.x + computer_world_object.pos[0];
    body->pos[1] = work->pos.y + computer_world_object.pos[1];
    body->pos[2] = work->pos.z + computer_world_object.pos[2];
    body->model = computer_world_object.model;
    body->child = NULL;
    body->sibling = NULL;
    world_add(WORLD_SOLID, task, body);
    VARS(work)->frame = 0.0F;
    work->mode = COMPUTER_RUN;
}

static void computer_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(computer_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void computer_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->body);
    world_object_free(VARS(work)->body);
}

static void computer_animate(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame += 1.0F;
    if (VARS(work)->frame >= computer_action->motion->nbFrame) {
        VARS(work)->frame = 0.0F;
    }
}

/* the set editor's part of the step, which does nothing */
static void computer_edit(struct task *task)
{
    /* @hack redundant: a read of the work, which keeps the task passed in, MOV
     * R14,R4 at 0x0c9076b8, where SHC expands this in computer_step; without
     * it, 9 of 65 words differ */
    (void)task->work;
}
