/* O DMGSPHERE: an invisible sphere that hurts players */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/placement.h"
#include "stg09/damage_sphere.h"
#include "stg09/helpers.h"
#include "system/task.h"

/* the task destroys itself beyond about 600 of the camera and player 0.
 * placement_try_leave_by takes the range squared */
#define RANGE ((600.0F * 600.0F) + 10.0F)

static struct collision_shape damage_sphere_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    SHAPE_HURTS_PLAYERS | GUARD(2) | ATTACK(3),
    PUSH_ALL,
    {0.0F, 0.0F, 0.0F},
    1.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void damage_sphere_run(struct task *task);

void damage_sphere_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            collision_init(task, &damage_sphere_shape, COLLISION_OBJECT, 1, 1);
            work->collision->shapes->radius = _builtin_fabsf(work->scl.x);
            collision_set_radius(work->collision);
        }
        task->step = damage_sphere_run;
    }
}

static void damage_sphere_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> SphereScale");
            casino_draw_sphere(&work->pos, work->scl.x);
            return;
        }
        collision_register(work);
    }
}
