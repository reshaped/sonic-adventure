/* O HasiHasi: a solid post, sized by its set entry */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg08/hasi_hasi.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT hasi_hasi_object;

static void hasi_hasi_init(struct task *task);
static void hasi_hasi_destroy(struct task *task);
static void hasi_hasi_run(struct task *task);
static void hasi_hasi_draw(struct task *task);

/* a capsule up the post. object_scale_shapes scales the record's copy of it
 * by one more than work->scl */
static struct collision_shape hasi_hasi_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {-1.0F, 19.75F, -0.100000024F},
    3.0F,
    16.75F,
    0.0F,
    {0},
    0,
    NULL,
};

void hasi_hasi_step(struct task *task)
{
    /* @hack spelling: post, a copy of task, which keeps task in R14 across
     * the range check (MOV R4,R14 at 0x0c90e108); through task, it goes to
     * the frame and 10 of 14 words differ */
    struct task *post = task;

    if (placement_try_leave(post)) {
        return;
    }
    hasi_hasi_init(post);
}

static void hasi_hasi_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, &hasi_hasi_shape, COLLISION_OBJECT, 1, 1);
        object_scale_shapes(work);
    }
    task->draw = hasi_hasi_draw;
    task->step = hasi_hasi_run;
    task->destructor = hasi_hasi_destroy;
}

static void hasi_hasi_destroy(struct task *task)
{
}

static void hasi_hasi_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        hasi_hasi_draw(task);
        if (!placement_editing(task)) {
            collision_register(work);
        }
    }
}

static void hasi_hasi_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F,
                work->scl.z + 1.0F);
        draw_tree(&hasi_hasi_object);
        njPopMatrix(1);
    }
}
