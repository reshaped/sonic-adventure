/* O PARASOL sunshade by the pool, whose one thin cylinder pushes nothing */
#include <ninja.h>

#include "adv0100/parasol.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT parasol_object;

enum parasol_mode { PARASOL_INIT, PARASOL_RUN };

static void parasol_draw(struct task *task);
static void parasol_destroy(struct task *task);
static void parasol_draw_work(struct task_work *work);

static struct collision_shape parasol_shapes[] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     0.1F,
     5.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

void egg_carrier_parasol_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PARASOL_INIT:
        task->destructor = parasol_destroy;
        task->draw = parasol_draw;
        if (!placement_editing(task)) {
            collision_init(task, parasol_shapes, COLLISION_OBJECT, 1, 1);
        }
        work->mode = PARASOL_RUN;
        break;
    case PARASOL_RUN:
        parasol_draw_work(work);
        break;
    }
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void parasol_draw(struct task *task)
{
    struct task_work *work = task->work;

    parasol_draw_work(work);
}

static void parasol_destroy(struct task *task)
{
}

static void parasol_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&parasol_object);
    njPopMatrix(1);
}
