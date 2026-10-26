/* O TWTESURI handrail by Twinkle Park, with a solid box along each side */
#include <ninja.h>

#include "adv00/twinkle_handrail.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT twinkle_handrail_object;

static void twinkle_handrail_step(struct task *task);
static void twinkle_handrail_draw(struct task *task);

/* clang-format off */
static struct collision_shape twinkle_handrail_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 5.5F, 18.0F},
     8.0F, 5.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 5.5F, -18.0F},
     8.0F, 5.5F, 1.0F, {0}, 0, NULL},
};
/* clang-format on */

void twinkle_handrail_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = 0;
    if (!placement_editing(task)) {
        collision_init(task, twinkle_handrail_shapes, COLLISION_OBJECT, 1, 2);
    }
    task->step = twinkle_handrail_step;
    task->draw = twinkle_handrail_draw;
}

static void twinkle_handrail_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
    }
    twinkle_handrail_draw(task);
}

static void twinkle_handrail_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&twinkle_handrail_object);
    njPopMatrix(1);
}
