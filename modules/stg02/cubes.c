/* O_CUBES, O_CUBEM, O_CUBEL, O_UKISIM, O TATES and O TATEL: models that
 * stand solid. O BIGFLA: a model in the collision world */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg02/cubes.h"
#include "system/task.h"

extern NJS_OBJECT windy_small_cube_object;
extern NJS_OBJECT windy_medium_cube_object;
extern NJS_OBJECT windy_large_cube_object;
extern NJS_OBJECT windy_floating_island_object;
extern NJS_OBJECT windy_bigfla_object;
extern NJS_OBJECT windy_tate_large_object;
extern NJS_OBJECT windy_tate_small_object;

/* a box's shape, from its center and its half sizes. it pushes every group
 * of kinds, and no group's shape can push it */
/* clang-format off */
#define BOX(x, y, z, half_x, half_y, half_z)                                   \
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {(x), (y), (z)},      \
     (half_x), (half_y), (half_z), {0, 0, 0}, 0, NULL}
/* clang-format on */
static struct collision_shape windy_small_cube_shape =
    BOX(0.0F, 5.0F, 0.0F, 4.0F, 4.0F, 4.0F);
static struct collision_shape windy_medium_cube_shape =
    BOX(0.0F, 5.0F, 0.0F, 9.0F, 5.0F, 5.0F);
static struct collision_shape windy_large_cube_shape =
    BOX(0.0F, 10.0F, 0.0F, 9.0F, 10.0F, 4.0F);
static struct collision_shape windy_tate_small_shape =
    BOX(0.0F, 10.0F, 0.0F, 13.0F, 12.0F, 5.0F);
static struct collision_shape windy_tate_large_shapes[3] = {
    BOX(0.0F, 10.0F, 0.0F, 13.0F, 12.0F, 5.0F),
    BOX(-23.0F, 10.0F, 0.0F, 13.0F, 15.0F, 5.0F),
    BOX(23.0F, 10.0F, 0.0F, 13.0F, 15.0F, 5.0F),
};
static struct collision_shape windy_floating_island_shapes[2] = {
    BOX(0.0F, -15.0F, 0.0F, 27.0F, 15.0F, 23.0F),
    BOX(0.0F, -43.0F, 0.0F, 8.0F, 12.0F, 8.0F),
};

void windy_small_cube_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_small_cube_object;
    if (!placement_editing(task)) {
        collision_init(task, &windy_small_cube_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void windy_medium_cube_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_medium_cube_object;
    if (!placement_editing(task)) {
        collision_init(task, &windy_medium_cube_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void windy_large_cube_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_large_cube_object;
    if (!placement_editing(task)) {
        collision_init(task, &windy_large_cube_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void windy_floating_island_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_floating_island_object;
    if (!placement_editing(task)) {
        collision_init(task, windy_floating_island_shapes, COLLISION_OBJECT, 1,
                       2);
    }
    model_start_solid(task);
}

void windy_bigfla_step(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &windy_bigfla_object;
    model_start_world(task);
}

void windy_tate_large_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_tate_large_object;
    if (!placement_editing(task)) {
        collision_init(task, windy_tate_large_shapes, COLLISION_OBJECT, 1, 3);
    }
    model_start_solid(task);
}

void windy_tate_small_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_tate_small_object;
    if (!placement_editing(task)) {
        collision_init(task, &windy_tate_small_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}
