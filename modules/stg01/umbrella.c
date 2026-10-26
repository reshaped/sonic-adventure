/* O KASA beach umbrella on a pole, turned only about y */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg01/umbrella.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT beach_umbrella_object;

static void beach_umbrella_step(struct task *task);
static void beach_umbrella_draw(struct task *task);
static void beach_umbrella_destroy(struct task *task);

/* the pole, a cylinder of radius 1 and 22 high, and the canopy, a sphere of
 * radius 10 centered 23 above the base */
static struct collision_shape beach_umbrella_shapes[2] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 11.0F, 0.0F},
     1.0F,
     11.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 23.0F, 0.0F},
     10.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* clear ang.x and ang.z, so only ang.y turns the umbrella */
void beach_umbrella_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    if (!placement_editing(task)) {
        collision_init(task, beach_umbrella_shapes, COLLISION_OBJECT, 1, 2);
    }
    task->step = beach_umbrella_step;
    task->draw = beach_umbrella_draw;
    task->destructor = beach_umbrella_destroy;
}

static void beach_umbrella_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
        }
        beach_umbrella_draw(task);
    }
}

static void beach_umbrella_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&beach_umbrella_object);
    njPopMatrix(1);
}

static void beach_umbrella_destroy(struct task *task)
{
}
