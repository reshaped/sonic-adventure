/* O PALM palm tree on the deck, whose capsule up its trunk pushes nothing */
#include <ninja.h>

#include "adv0100/palm.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT palm_object;

enum palm_mode { PALM_INIT, PALM_RUN };

static void palm_draw(struct task *task);
static void palm_destroy(struct task *task);
static void palm_draw_work(struct task_work *work);

static struct collision_shape palm_shapes[] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 50.0F, 0.0F},
     1.0F,
     50.0F,
     0.0F,
     {0},
     0,
     NULL},
};

void egg_carrier_palm_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PALM_INIT:
        if (!placement_editing(task)) {
            collision_init(task, palm_shapes, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = palm_destroy;
        task->draw = palm_draw;
        work->mode = PALM_RUN;
        break;
    case PALM_RUN:
        break;
    }
    palm_draw_work(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void palm_draw(struct task *task)
{
    palm_draw_work(task->work);
}

static void palm_destroy(struct task *task)
{
}

static void palm_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&palm_object);
    njPopMatrix(1);
}
