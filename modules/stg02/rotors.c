/* O SAKUC, O HANEA, O HANEB, O BIGFLO and O HASIE: solid models with
 * spinning parts */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg02/rotors.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* each of the first four is a fixed model with one child, the part that
 * spins */
extern NJS_OBJECT windy_fence_c_object;
extern NJS_OBJECT windy_propeller_a_object;
extern NJS_OBJECT windy_propeller_b_object;
extern NJS_OBJECT windy_big_flower_object;
extern NJS_OBJECT windy_hasie_object;
extern NJS_OBJECT windy_hasie_rotor_a_object;
extern NJS_OBJECT windy_hasie_rotor_b_object;

/* work's vars for the first four objects */
struct rotor_vars {
    Float angle; /* the child's, about axis, in degrees */
    Sint8 axis; /* enum rotor_axis */
    NJS_OBJECT *object;
};

/* work's vars for O HASIE */
struct hasie_vars {
    Float angle; /* the rotors', about y, in degrees */
};

#define VARS(work) ((struct rotor_vars *)&(work)->vars)
#define HASIE(work) ((struct hasie_vars *)&(work)->vars)

enum rotor_axis { ROTOR_X, ROTOR_Y, ROTOR_Z };

/* the modes. in ROTOR_STOP and ROTOR_END the steps do nothing, and no step
 * here sets either */
enum rotor_mode { ROTOR_START, ROTOR_RUN, ROTOR_STOP, ROTOR_END };

/* the squared ranges. an object destroys itself once the camera and player 0
 * are farther than 1000 from it. O HASIE waits until they are farther than
 * 1500 */
#define ROTOR_RANGE (1000.0F * 1000.0F)
#define HASIE_RANGE (1500.0F * 1500.0F)

/* the radii of the spheres draw_in_view tests. it finds a sphere in view
 * only on a frame's first step. the first four objects' steps spin, draw and
 * register them only then. O HASIE draws only then */
#define ROTOR_VIEW_RADIUS 40.0F
#define HASIE_VIEW_RADIUS 10.0F

static struct collision_shape windy_hasie_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 17.75F},
     2.0F,
     30.0F,
     2.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, -17.75F},
     2.0F,
     30.0F,
     2.0F,
     {0, 0, 0},
     0,
     NULL},
};

static struct collision_shape windy_fence_c_shape = {SHAPE_RECTANGLE,
                                                     0,
                                                     PUSH_ALL |
                                                         PUSH_FIRM(PUSH_ALL),
                                                     {17.0F, 0.0F, 0.0F},
                                                     20.0F,
                                                     12.0F,
                                                     2.0F,
                                                     {0, 0, 0},
                                                     0,
                                                     NULL};

static struct collision_shape windy_propeller_b_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    3.0F,
    28.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL};

static struct collision_shape windy_propeller_a_shapes[2] = {
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {10.0F, 0.0F, 0.0F},
     17.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {40.0F, 0.0F, 0.0F},
     13.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static struct collision_shape windy_big_flower_shape = {SHAPE_SPHERE,
                                                        0,
                                                        PUSH_ALL |
                                                            PUSH_FIRM(PUSH_ALL),
                                                        {0.0F, 0.0F, 0.0F},
                                                        75.0F,
                                                        0.0F,
                                                        0.0F,
                                                        {0, 0, 0},
                                                        0,
                                                        NULL};

static void windy_rotor_step(struct task *task);
static void windy_rotor_draw(struct task *task);
static void windy_hasie_draw(struct task *task);

void windy_fence_c_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->object = &windy_fence_c_object;
    collision_init(task, &windy_fence_c_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->axis = ROTOR_Y;
    task->step = windy_rotor_step;
    task->draw = windy_rotor_draw;
}

void windy_propeller_a_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->object = &windy_propeller_a_object;
    collision_init(task, windy_propeller_a_shapes, COLLISION_OBJECT, 1, 2);
    VARS(work)->axis = ROTOR_Z;
    task->step = windy_rotor_step;
    task->draw = windy_rotor_draw;
}

void windy_propeller_b_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->object = &windy_propeller_b_object;
    collision_init(task, &windy_propeller_b_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->axis = ROTOR_Y;
    task->step = windy_rotor_step;
    task->draw = windy_rotor_draw;
}

void windy_big_flower_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->object = &windy_big_flower_object;
    collision_init(task, &windy_big_flower_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->axis = ROTOR_Y;
    task->step = windy_rotor_step;
    task->draw = windy_rotor_draw;
}

static void windy_rotor_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, ROTOR_RANGE)) {
        return;
    }
    switch (work->mode) {
    case ROTOR_START:
        work->mode = ROTOR_RUN;
        break;
    case ROTOR_RUN:
        if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                         ROTOR_VIEW_RADIUS)) {
            VARS(work)->angle += work->scl.x;
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- ROTATE_SPEED");
            }
            windy_rotor_draw(task);
            collision_register(work);
        }
        break;
    case ROTOR_STOP:
        break;
    case ROTOR_END:
        break;
    }
}

static void windy_rotor_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    NJS_OBJECT *child = object->child;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(object->model);
    njPushMatrix(NULL);
    njTranslate(NULL, child->pos[0], child->pos[1], child->pos[2]);
    /* @bug logic: the child turns about z, x and z again by its own
     * position's coordinates, each truncated to an angle (FTRC at 0x0c90ab5a
     * for the first), where its angles belong. child->ang goes unread */
    TURN(njRotateZ, child->pos[2]);
    TURN(njRotateX, child->pos[0]);
    TURN(njRotateZ, child->pos[1]);
    switch (VARS(work)->axis) {
    case ROTOR_X:
        TURN(njRotateX, (Angle)(VARS(work)->angle * ANGLE_PER_DEGREE));
        break;
    case ROTOR_Y:
        TURN(njRotateY, (Angle)(VARS(work)->angle * ANGLE_PER_DEGREE));
        break;
    case ROTOR_Z:
        TURN(njRotateZ, (Angle)(VARS(work)->angle * ANGLE_PER_DEGREE));
        break;
    }
    draw_model(child->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

void windy_hasie_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, HASIE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case ROTOR_START:
        work->mode = ROTOR_RUN;
        task->draw = windy_hasie_draw;
        collision_init(task, windy_hasie_shapes, COLLISION_OBJECT, 1, 2);
        break;
    case ROTOR_RUN:
        HASIE(work)->angle += work->scl.x;
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- ROTATE_SPEED");
        }
        windy_hasie_draw(task);
        collision_register(work);
        break;
    case ROTOR_STOP:
        break;
    case ROTOR_END:
        break;
    }
}

static void windy_hasie_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *base;
    NJS_OBJECT *rotor_a;
    NJS_OBJECT *rotor_b;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                     HASIE_VIEW_RADIUS)) {
        base = &windy_hasie_object;
        rotor_a = &windy_hasie_rotor_a_object;
        rotor_b = &windy_hasie_rotor_b_object;
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        draw_model(base->model);
        njPushMatrix(NULL);
        njTranslate(NULL, rotor_a->pos[0], rotor_a->pos[1], rotor_a->pos[2]);
        TURN(njRotateY, (Angle)(HASIE(work)->angle * ANGLE_PER_DEGREE));
        draw_model(rotor_a->model);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, rotor_b->pos[0], rotor_b->pos[1], rotor_b->pos[2]);
        TURN(njRotateY, (Angle)(HASIE(work)->angle * ANGLE_PER_DEGREE));
        draw_model(rotor_b->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}
