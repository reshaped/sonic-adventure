/* O_POLE, Chaos 0's solid pole, 130 high. its shape is an upright cylinder
 * of radius 1 from its pos up. the shape pushes all of collision.h's push
 * groups, and none of them pushes it */
#include <ninja.h>

#include "b_chaos0/pole.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT chaos0_pole_object;

/* clang-format off */
static struct collision_shape pole_shape = {
    SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 65.0F, 0.0F},
    1.0F, 65.0F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

static void pole_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(&chaos0_pole_object);
    njPopMatrix(1);
}

static void pole_run(struct task *task)
{
    if (!placement_try_leave(task)) {
        /* @hack spelling: pole_draw called in each branch, which makes the set
         * editor's branch a tail call to it (BRA at 0x0c90534a). called once
         * ahead of the test, 20 of 26 words differ */
        if (placement_editing(task)) {
            pole_draw(task);
        } else {
            pole_draw(task);
            collision_register(task->work);
        }
    }
}

static void pole_destroy(struct task *task)
{
}

void chaos0_pole_step(struct task *task)
{
    if (!placement_try_leave(task)) {
        task->destructor = pole_destroy;
        collision_init(task, &pole_shape, COLLISION_OBJECT, 1, 1);
        task->step = pole_run;
        task->draw = pole_draw;
    }
}
