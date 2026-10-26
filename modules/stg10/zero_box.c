/* C APP ZERO and C KILL ZERO invisible boxes that cue and remove Zero */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "enemy/zero_handle.h"
#include "object/placement.h"
#include "stg10/zero_appear.h"
#include "stg10/zero_box.h"
#include "system/task.h"
#include "system/util.h"

enum zero_box_mode { BOX_INIT, BOX_RUN };

/* the number of the flag a C APP ZERO sets, which its set entry gives as
 * angle x. C KILL ZERO stores it the same way and never reads it */
#define CUE(work) ((work)->tick)

/* the box's shape, which pushes nothing and which only players and attacks
 * touch. its base sits at the box's position. both steps size it by the set
 * entry's scales as they start */
static struct collision_shape zero_box_shape = {
    SHAPE_RECTANGLE | SHAPE_IGNORE_ENEMIES | SHAPE_IGNORE_OBJECTS |
        SHAPE_IGNORE_OTHERS,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 10.0F, 0.0F},
    5.0F,
    10.0F,
    2.0F,
    {0},
    0,
    NULL,
};

/* draw a cube where the box's shape lies, for the set editor */
static void zero_box_draw(struct task_work *work)
{
    Float x = work->scl.x * CUBE_FIT;
    Float y = work->scl.y * CUBE_FIT;
    Float z = work->scl.z * CUBE_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + work->scl.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, x, y, z);
    draw_set_material(0.05F, 0.05F, 0.0F, 0.0F);
    draw_tree(marker_cube_object);
    njPopMatrix(1);
}

void zero_appear_box_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;

    if (edit_mode) {
        zero_box_draw(work);
        return;
    }
    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BOX_INIT:
        CUE(work) = work->ang.x;
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, &zero_box_shape, COLLISION_OBJECT, 1, 1);
        shape = work->collision->shapes;
        shape->center.y = work->scl.y;
        shape->radius = work->scl.x;
        shape->size_y = work->scl.y;
        shape->size_z = work->scl.z;
        work->mode = BOX_RUN;
        break;
    case BOX_RUN:
        if (work->collision->flags & COLLISION_HIT) {
            zero_appear_set(CUE(work));
            task_kill(task);
            return;
        }
        break;
    }
    collision_register(work);
}

void zero_kill_box_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;

    if (edit_mode) {
        zero_box_draw(work);
        return;
    }
    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BOX_INIT:
        CUE(work) = work->ang.x;
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, &zero_box_shape, COLLISION_OBJECT, 1, 1);
        shape = work->collision->shapes;
        shape->center.y = work->scl.y;
        shape->radius = work->scl.x;
        shape->size_y = work->scl.y;
        shape->size_z = work->scl.z;
        work->mode = BOX_RUN;
        break;
    case BOX_RUN:
        if (work->collision->flags & COLLISION_HIT) {
            zero_kill();
            task_kill(task);
            return;
        }
        break;
    }
    collision_register(work);
}
