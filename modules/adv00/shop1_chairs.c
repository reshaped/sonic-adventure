/* O S1 CHAIR row of five chairs in the first shop */
#include <ninja.h>

#include "adv00/shop1_chairs.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT shop1_chair_object;

#define CHAIRS 5
#define CHAIR_SPACING 17.0F

static void shop1_chairs_step(struct task *task);

/* a cylinder for each chair, and a sixth a chair's spacing past the row.
 * collision_init takes CHAIRS of them, so the sixth goes unused */
/* clang-format off */
static struct collision_shape shop1_chairs_shapes[CHAIRS + 1] = {
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 4.0F, 0.0F},
     2.0F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 4.0F, CHAIR_SPACING}, 2.0F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 4.0F, 2.0F * CHAIR_SPACING}, 2.0F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 4.0F, 3.0F * CHAIR_SPACING}, 2.0F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 4.0F, 4.0F * CHAIR_SPACING}, 2.0F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 4.0F, 5.0F * CHAIR_SPACING}, 2.0F, 4.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

void shop1_chairs_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = 0;
    if (!placement_editing(task)) {
        collision_init(task, shop1_chairs_shapes, COLLISION_OBJECT, 1, CHAIRS);
    }
    task->step = shop1_chairs_step;
    task->draw = shop1_chairs_step;
}

/* the task's step and its draw. unless the set editor holds the task, it
 * registers the chairs for this step's collision checks. it then draws them */
static void shop1_chairs_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    for (i = 0; i < CHAIRS; i++) {
        draw_tree(&shop1_chair_object);
        njTranslate(NULL, 0.0F, 0.0F, CHAIR_SPACING);
    }
    njPopMatrix(1);
}
