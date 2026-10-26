/* O CRECT2, O CCYL2 and O CCAP: a solid box, cylinder and capsule. only the
 * set editor draws them. the box turns with its task's y angle, and the
 * others with all three of its angles */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg09/invisible.h"
#include "system/task.h"
#include "system/util.h"

/* a shape starts within the range its object kind gives. once started, it
 * is destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)

/* while TRUE, the shapes draw outside the set editor too. it is static, and
 * this file never sets it */
static Bool invisible_show = FALSE;
/* each kind's template, which its objects' records copy. in the set editor,
 * each object's run step sizes the template from its own scale and draws it */
/* @hack type: an array of one, not the single shape it is. the run step keeps
 * its address in R13 while it sizes it (0x0c91d68c). as a single shape, the
 * address takes R4, and 9 of invisible_rectangle2_run's 78 words differ */
static struct collision_shape invisible_rectangle2_shape[1] = {
    {
        SHAPE_RECTANGLE2 | SHAPE_NO_PUSH,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        1.0F,
        1.0F,
        1.0F,
        {0},
        0,
        NULL,
    },
};
/* @hack type: an array of one, as invisible_rectangle2_shape is, kept in R13
 * (0x0c91d728). as a single shape, 9 of invisible_cylinder2_run's 78 words
 * differ */
static struct collision_shape invisible_cylinder2_shape[1] = {
    {
        SHAPE_CYLINDER2 | SHAPE_NO_PUSH,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        1.0F,
        1.0F,
        1.0F,
        {0},
        0,
        NULL,
    },
};
/* @hack type: an array of one, as invisible_rectangle2_shape is, kept in R13
 * (0x0c91d82c). as a single shape, 9 of invisible_capsule_run's 78 words
 * differ */
static struct collision_shape invisible_capsule_shape[1] = {
    {
        SHAPE_CAPSULE | SHAPE_NO_PUSH,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        1.0F,
        1.0F,
        1.0F,
        {0},
        0,
        NULL,
    },
};

/* each function below draws shape at pos with the markers. the run steps
 * call them in the set editor, or while invisible_show is TRUE */

static void invisible_draw_cylinder2(struct task_work *work,
                                     struct collision_shape *shape,
                                     NJS_POINT3 *pos)
{
    Float radius = shape->radius * MARKER_FIT;
    Float height = shape->size_y * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    if (shape->flags & SHAPE_TURN_YZX) {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateY, work->ang.y);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
        }
        TURN(njRotateY, shape->ang.y);
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
    } else {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, work->ang.y);
        }
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
        TURN(njRotateY, shape->ang.y);
    }
    njScale(NULL, radius, height, radius);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);
}

static void invisible_draw_rectangle2(struct task_work *work,
                                      struct collision_shape *shape,
                                      NJS_POINT3 *pos)
{
    Float x = shape->radius * MARKER_FIT;
    Float y = shape->size_y * MARKER_FIT;
    Float z = shape->size_z * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    if ((shape->flags & SHAPE_IN_WORLD) || (shape->flags & SHAPE_UNTURNED)) {
        TURN(njRotateY, shape->ang.y);
    } else {
        TURN(njRotateY, work->ang.y + shape->ang.y);
    }
    TURN(njRotateZ, shape->ang.z);
    /* the cube is half the size of the other markers */
    njScale(NULL, x * 2.0F, y * 2.0F, z * 2.0F);
    draw_tree(marker_cube_object);
    njPopMatrix(1);
}

static void invisible_draw_capsule(struct task_work *work,
                                   struct collision_shape *shape,
                                   NJS_POINT3 *pos)
{
    Float radius = shape->radius * MARKER_FIT;
    Float height = shape->size_y * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    if (shape->flags & SHAPE_TURN_YZX) {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateY, work->ang.y);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
        }
        TURN(njRotateY, shape->ang.y);
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
    } else {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, work->ang.y);
        }
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
        TURN(njRotateY, shape->ang.y);
    }
    njPushMatrix(NULL);
    njScale(NULL, radius, height, radius);
    draw_tree(&marker_tube_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, shape->size_y, 0.0F);
    njScale(NULL, radius, radius, radius);
    draw_tree(&marker_dome_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, -shape->size_y, 0.0F);
    njScale(NULL, radius, radius, radius);
    njRotateX(NULL, NJM_DEG_ANG(180));
    draw_tree(&marker_dome_object);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void invisible_rectangle2_run(struct task *task);
static void invisible_cylinder2_run(struct task *task);
static void invisible_capsule_run(struct task *task);

void invisible_rectangle2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        task->step = invisible_rectangle2_run;
        if (!placement_editing(task)) {
            collision_init(task, invisible_rectangle2_shape, COLLISION_OBJECT,
                           1, 1);
            work->collision->shapes->radius = work->scl.x;
            work->collision->shapes->size_y = work->scl.y;
            work->collision->shapes->size_z = work->scl.z;
            collision_set_radius(work->collision);
        }
    }
}

void invisible_cylinder2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        task->step = invisible_cylinder2_run;
        if (!placement_editing(task)) {
            collision_init(task, invisible_cylinder2_shape, COLLISION_OBJECT, 1,
                           1);
            work->collision->shapes->radius = work->scl.x;
            work->collision->shapes->size_y = work->scl.y;
            work->collision->shapes->size_z = work->scl.z;
            collision_set_radius(work->collision);
        }
    }
}

void invisible_capsule_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        task->step = invisible_capsule_run;
        if (!placement_editing(task)) {
            collision_init(task, invisible_capsule_shape, COLLISION_OBJECT, 1,
                           1);
            work->collision->shapes->radius = work->scl.x;
            work->collision->shapes->size_y = work->scl.y;
            work->collision->shapes->size_z = work->scl.z;
            collision_set_radius(work->collision);
        }
    }
}

static void invisible_rectangle2_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Y), "-> angY ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Z), "-> angZ ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> sclX ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> sclY ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> sclZ ");
        invisible_rectangle2_shape->radius = work->scl.x;
        invisible_rectangle2_shape->size_y = work->scl.y;
        invisible_rectangle2_shape->size_z = work->scl.z;
        invisible_draw_rectangle2(work, invisible_rectangle2_shape, &work->pos);
        return;
    }
    if (invisible_show) {
        invisible_draw_rectangle2(work, work->collision->shapes, &work->pos);
    }
    collision_register(work);
}

static void invisible_cylinder2_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_X), "-> angX ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Y), "-> angY ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Z), "-> angZ ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> rad ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> height ");
        invisible_cylinder2_shape->radius = work->scl.x;
        invisible_cylinder2_shape->size_y = work->scl.y;
        invisible_cylinder2_shape->size_z = work->scl.z;
        invisible_draw_cylinder2(work, invisible_cylinder2_shape, &work->pos);
        return;
    }
    if (invisible_show) {
        invisible_draw_cylinder2(work, work->collision->shapes, &work->pos);
    }
    collision_register(work);
}

static void invisible_capsule_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_X), "-> angX ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Y), "-> angY ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Z), "-> angZ ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> rad ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> height ");
        invisible_capsule_shape->radius = work->scl.x;
        invisible_capsule_shape->size_y = work->scl.y;
        invisible_capsule_shape->size_z = work->scl.z;
        invisible_draw_capsule(work, invisible_capsule_shape, &work->pos);
        return;
    }
    if (invisible_show) {
        invisible_draw_capsule(work, work->collision->shapes, &work->pos);
    }
    collision_register(work);
}
