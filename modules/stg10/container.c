/* O CONTAINER solid box whose size scale x sets */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg10/container.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL container_model;

/* the box is destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

#define SCALE_MIN 1.0F
#define SCALE_MAX 15.0F

struct container_vars {
    Uint8 unk_00[8];
    Float scale; /* a tenth of scale x, held from SCALE_MIN to SCALE_MAX */
};

#define VARS(work) ((struct container_vars *)&(work)->vars)

enum container_mode { CONTAINER_INIT, CONTAINER_RUN };

static void container_init(struct task *task);
static void container_scale(struct task *task);
static void final_container_draw(struct task *task);
static void final_container_destroy(struct task *task);

static struct collision_shape final_container_shape = {
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

/* @hack spelling: collision_register through an inline function, which
 * reads task->work again at each call and keeps task at @R15 (0x0c9009c4,
 * 0x0c900a10). called with final_container_step's own work, 76 of its 80
 * words differ */
static void container_register(struct task *task)
{
    collision_register(task->work);
}

void final_container_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case CONTAINER_INIT:
            container_init(task);
            break;
        case CONTAINER_RUN:
            final_container_draw(task);
            container_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case CONTAINER_INIT:
            container_init(task);
            break;
        case CONTAINER_RUN:
            container_scale(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X),
                        "-> SCALE(*10) : (Scale = %f)", VARS(work)->scale);
            final_container_draw(task);
            container_register(task);
            break;
        }
    }
}

static void container_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = final_container_draw;
    task->destructor = final_container_destroy;
    collision_init(task, &final_container_shape, COLLISION_OBJECT, 1, 1);
    container_scale(task);
    work->mode = CONTAINER_RUN;
}

/* set the box's scale from scale x. its shape takes final_container_shape's
 * sizes times the scale, and the record's radius is 20 plus 20 times it */
static void container_scale(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;

    VARS(work)->scale = work->scl.x * 0.1F;
    if (VARS(work)->scale < SCALE_MIN) {
        VARS(work)->scale = SCALE_MIN;
    } else if (VARS(work)->scale > SCALE_MAX) {
        VARS(work)->scale = SCALE_MAX;
    }
    /* @hack spelling: the radius stored through work, which loads
     * work->collision again (MOV.L @(56,R4),R3 at 0x0c900ad0); through
     * collision, 36 of container_scale's 60 words differ */
    work->collision->radius = (VARS(work)->scale * 20.0F) + 20.0F;
    collision->shapes->center.y = VARS(work)->scale * 6.0F;
    collision->shapes->radius = VARS(work)->scale * 6.0F;
    collision->shapes->size_y = VARS(work)->scale * 6.0F;
    collision->shapes->size_z = VARS(work)->scale * 8.0F;
}

static void final_container_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        scale = VARS(work)->scale;
        njScale(NULL, scale, scale, scale);
        draw_model(&container_model);
        njPopMatrix(1);
    }
}

static void final_container_destroy(struct task *task)
{
}
