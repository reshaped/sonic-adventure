/* C SWITCH, an invisible box that turns a switch on once a player touches it */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/switch.h"
#include "stg12/switch.h"
#include "system/task.h"
#include "system/util.h"

enum shelter_switch_mode { SWITCH_INIT, SWITCH_WAIT };

/* a box only players touch. each box's init sizes its copy of the shape
 * from the set entry's scale */
static struct collision_shape shelter_switch_shape = {
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

static void shelter_switch_draw_box(struct task_work *work)
{
    Float x = work->scl.x * 0.2F;
    Float y = work->scl.y * 0.2F;
    Float z = work->scl.z * 0.2F;
    Angle ang;

    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + work->scl.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, x, y, z);
    draw_set_material(0.05F, 0.05F, 0.0F, 0.0F);
    draw_tree(marker_cube_object);
    njPopMatrix(1);
}

void shelter_switch_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;

    if (edit_mode) {
        shelter_switch_draw_box(work);
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> SWITCH ID");
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> NO USE");
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SIZE X");
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> SIZE Y");
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> SIZE Z");
        return;
    }
    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SWITCH_INIT:
        /* the switch to turn on, which the set entry's angle x names */
        work->tick = work->ang.x;
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, &shelter_switch_shape, COLLISION_OBJECT, 1, 1);
        shape = work->collision->shapes;
        shape->center.y = work->scl.y;
        shape->radius = work->scl.x;
        shape->size_y = work->scl.y;
        shape->size_z = work->scl.z;
        work->mode = SWITCH_WAIT;
        break;
    case SWITCH_WAIT:
        if (work->collision->flags & COLLISION_HIT) {
            switch_set(work->tick, TRUE);
            task_kill(task);
            return;
        }
        break;
    }
    collision_register(work);
}

void shelter_touch_box_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;

    if (edit_mode) {
        shelter_switch_draw_box(work);
        return;
    }
    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SWITCH_INIT:
        work->tick = work->ang.x;
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, &shelter_switch_shape, COLLISION_OBJECT, 1, 1);
        shape = work->collision->shapes;
        shape->center.y = work->scl.y;
        shape->radius = work->scl.x;
        shape->size_y = work->scl.y;
        shape->size_z = work->scl.z;
        work->mode = SWITCH_WAIT;
        break;
    case SWITCH_WAIT:
        if (work->collision->flags & COLLISION_HIT) {
            task_kill(task);
            return;
        }
        break;
    }
    collision_register(work);
}
