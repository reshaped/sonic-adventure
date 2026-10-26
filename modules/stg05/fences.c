/* the O SAKU, O SAKU2, MT_TSAKU and MT_SOTO fences, the O ROUYA prison door
 * and the MT_TBOX crate. each draws at one more than its set entry's
 * scale */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg05/fences.h"
#include "system/task.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT mountain_fence_object;
extern NJS_OBJECT mountain_fence_2_object;
extern NJS_OBJECT prison_door_object;
extern NJS_OBJECT mountain_crate_object;
extern NJS_OBJECT iron_fence_object;

/* O SAKU2's two posts and its rail */
static struct collision_shape mountain_fence_2_shapes[3] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-10.0F, 7.0F, 0.0F},
     2.0F,
     8.0F,
     5.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 8.0F, 0.0F},
     10.0F,
     3.0F,
     2.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {10.0F, 7.0F, 0.0F},
     2.0F,
     9.0F,
     5.0F,
     {0, 0, 0},
     0,
     NULL},
};

static struct collision_shape mountain_crate_shape = {SHAPE_RECTANGLE,
                                                      0,
                                                      PUSH_ALL |
                                                          PUSH_FIRM(PUSH_ALL),
                                                      {0.0F, 0.0F, 0.0F},
                                                      5.0F,
                                                      5.0F,
                                                      5.0F,
                                                      {0, 0, 0},
                                                      0,
                                                      NULL};
static struct collision_shape prison_door_shape = {SHAPE_RECTANGLE,
                                                   0,
                                                   PUSH_ALL |
                                                       PUSH_FIRM(PUSH_ALL),
                                                   {0.0F, 0.0F, 0.0F},
                                                   30.0F,
                                                   30.0F,
                                                   5.0F,
                                                   {0, 0, 0},
                                                   0,
                                                   NULL};
static struct collision_shape iron_fence_shape = {SHAPE_RECTANGLE,
                                                  0,
                                                  PUSH_ALL |
                                                      PUSH_FIRM(PUSH_ALL),
                                                  {0.0F, 0.0F, 0.0F},
                                                  48.0F,
                                                  13.0F,
                                                  5.0F,
                                                  {0, 0, 0},
                                                  0,
                                                  NULL};
static struct collision_shape mountain_fence_shape = {SHAPE_RECTANGLE,
                                                      0,
                                                      PUSH_ALL |
                                                          PUSH_FIRM(PUSH_ALL),
                                                      {10.0F, 0.0F, 0.0F},
                                                      50.0F,
                                                      10.0F,
                                                      5.0F,
                                                      {0, 0, 0},
                                                      0,
                                                      NULL};

void mountain_fence_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &mountain_fence_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, &mountain_fence_shape, COLLISION_OBJECT, 1, 1);
        object_scale_shapes(work);
    }
    model_start_solid_scaled(task);
}

void mountain_fence_2_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &mountain_fence_2_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, mountain_fence_2_shapes, COLLISION_OBJECT, 1, 3);
        object_scale_shapes(work);
    }
    model_start_solid_scaled(task);
}

void prison_door_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &prison_door_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, &prison_door_shape, COLLISION_OBJECT, 1, 1);
        object_scale_shapes(work);
    }
    model_start_solid_scaled(task);
}

/* MT_SOTO: O SAKU's fence, with no collision */
void mountain_fence_outside_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &mountain_fence_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    model_start_scaled(task);
}

void mountain_crate_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &mountain_crate_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, &mountain_crate_shape, COLLISION_OBJECT, 1, 1);
        object_scale_shapes(work);
    }
    model_start_solid_scaled(task);
}

void iron_fence_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &iron_fence_object;
    MODEL(work)->texlist = &obj_mountain_texlist;
    if (!placement_editing(task)) {
        collision_init(task, &iron_fence_shape, COLLISION_OBJECT, 1, 1);
        object_scale_shapes(work);
    }
    model_start_solid_scaled(task);
}
