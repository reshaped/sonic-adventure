/* O BED, a beach lounger with a raised back */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg01/bed.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT beach_bed_object;

static void beach_bed_step(struct task *task);
static void beach_bed_draw(struct task *task);
static void beach_bed_destroy(struct task *task);

/* the seat, and the back, turned 315 degrees about z: a tilt of 45 degrees */
static struct collision_shape beach_bed_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {4.5F, 2.0F, 0.0F},
     7.0F,
     2.0F,
     5.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE2,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.5F, 7.0F, 0.0F},
     3.5F,
     2.0F,
     5.0F,
     {0, 0, NJM_DEG_ANG(315)},
     0,
     NULL},
};

void beach_bed_init(struct task *task)
{
    if (!placement_editing(task)) {
        collision_init(task, beach_bed_shapes, COLLISION_OBJECT, 1, 2);
    }
    task->step = beach_bed_step;
    task->draw = beach_bed_draw;
    task->destructor = beach_bed_destroy;
}

static void beach_bed_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
        }
        beach_bed_draw(task);
    }
}

static void beach_bed_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&beach_bed_object);
    njPopMatrix(1);
}

static void beach_bed_destroy(struct task *task)
{
}
