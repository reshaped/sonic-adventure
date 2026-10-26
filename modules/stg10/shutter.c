/* O SHUTTER fixed shutter sized by its set entry */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg10/shutter.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shutter_object;

#define SHUTTER_WIDTH 61.0F /* shutter_object's width, along z */
#define SHUTTER_HEIGHT 110.66F /* shutter_object's height */

/* while the set editor holds the shutter, its width and height stay at this
 * or more */
#define SCALE_MIN 0.1F

/* how far the collision record reaches from the shutter */
#define REACH 100.0F

enum shutter_mode { SHUTTER_INIT, SHUTTER_RUN };

static void shutter_destroy(struct task *task);
static void shutter_draw(struct task *task);
static void shutter_draw_at(struct task_work *work);

/* a box 10 units thick, whose height and width the step sets from scale y and
 * x. it pushes every group, and items, enemies, pickups and rings can't push
 * it */
static struct collision_shape shutter_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 10.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* the shutter draws scale x units wide and scale y high. its box is as high,
 * and 2.4 times as wide */
void shutter_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        if (work->scl.x < SCALE_MIN) {
            work->scl.x = SCALE_MIN;
        }
        if (work->scl.y < SCALE_MIN) {
            work->scl.y = SCALE_MIN;
        }
        njPrintC(NJM_LOCATION(10, 31), "SCL X -> WIDTH");
        njPrintC(NJM_LOCATION(10, 32), "SCL Y -> HEIGHT");
        shutter_draw_at(work);
        return;
    }
    switch (work->mode) {
    case SHUTTER_INIT:
        work->ang.x = 0;
        work->ang.z = 0;
        task->destructor = shutter_destroy;
        task->draw = shutter_draw;
        if (!placement_editing(task)) {
            collision_init(task, &shutter_shape, COLLISION_OBJECT, 1, 1);
            shape = work->collision->shapes;
            shape->center.y = work->scl.y * 0.5F;
            shape->size_y = work->scl.y * 0.5F;
            shape->size_z = work->scl.x * 1.2F;
            work->collision->radius = REACH;
        }
        work->mode = SHUTTER_RUN;
        break;
    case SHUTTER_RUN:
        break;
    }
    shutter_draw_at(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void shutter_destroy(struct task *task)
{
}

static void shutter_draw(struct task *task)
{
    shutter_draw_at(task->work);
}

static void shutter_draw_at(struct task_work *work)
{
    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, 1.0F, work->scl.y * (1.0F / SHUTTER_HEIGHT),
                work->scl.x * (1.0F / SHUTTER_WIDTH));
        draw_tree(&shutter_object);
        njPopMatrix(1);
    }
}
