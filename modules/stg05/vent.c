/* O VENT, a solid metal vent that puffs a little dust each step */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg05/vent.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_MODEL mountain_vent_model;

/* no step sets VENT_IDLE, in which the step does nothing */
enum mountain_vent_mode { VENT_INIT, VENT_RUN, VENT_IDLE };

#define DRAW_RADIUS 50.0F

static struct collision_shape mountain_vent_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    5.0F,
    5.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void mountain_vent_puff(struct task *task);
static void mountain_vent_draw(struct task *task);
static void mountain_vent_destroy(struct task *task);

void mountain_vent_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case VENT_INIT:
        if (!placement_editing(task)) {
            collision_init(task, &mountain_vent_shape, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = mountain_vent_destroy;
        task->draw = mountain_vent_draw;
        work->mode = VENT_RUN;
        break;
    case VENT_RUN:
        if (!placement_editing(task)) {
            mountain_vent_puff(task);
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        mountain_vent_draw(task);
        break;
    case VENT_IDLE:
        break;
    }
}

static void mountain_vent_puff(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed = {0.0F, 0.0F, 0.0F};

    dust_create(&work->pos, &speed, 0.02F);
}

static void mountain_vent_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&mountain_vent_model);
        njPopMatrix(1);
    }
}

static void mountain_vent_destroy(struct task *task)
{
}
