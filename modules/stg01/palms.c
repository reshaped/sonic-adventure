/* YASI0 to YASI3: palms with solid trunks. the leaves of YASI0 to YASI2
 * turn to face the camera */
#include <ninja.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg01/palms.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL beach_palm_0_trunk_model;
extern NJS_OBJECT beach_palm_0_leaves_a_object;
extern NJS_OBJECT beach_palm_0_leaves_b_object;
extern NJS_MODEL beach_palm_1_trunk_model;
extern NJS_OBJECT beach_palm_1_leaves_a_object;
extern NJS_OBJECT beach_palm_1_leaves_b_object;
extern NJS_MODEL beach_palm_2_trunk_model;
extern NJS_OBJECT beach_palm_2_leaves_a_object;
extern NJS_OBJECT beach_palm_2_leaves_b_object;
extern NJS_MODEL beach_palm_3_model;

/* PALM_KILL's step kills the placement. no code sets it */
enum beach_palm_mode { PALM_INIT, PALM_RUN, PALM_KILL };

/* the trunks, capsules of radius 3 that turn with the palm */
static struct collision_shape beach_palm_0_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 20.0F, 0.0F},
    3.0F,
    20.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct collision_shape beach_palm_1_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 15.0F, 0.0F},
    3.0F,
    15.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct collision_shape beach_palm_2_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 20.0F, 0.0F},
    3.0F,
    15.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct collision_shape beach_palm_3_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 20.0F, 0.0F},
    3.0F,
    20.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

static void beach_palm_0_draw(struct task *task);
static void beach_palm_1_draw(struct task *task);
static void beach_palm_2_draw(struct task *task);
static void beach_palm_destroy(struct task *task);
static void beach_palm_3_draw(struct task *task);

void beach_palm_0_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PALM_INIT:
        if (!placement_editing(task)) {
            collision_init(task, &beach_palm_0_shape, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = beach_palm_destroy;
        task->draw = beach_palm_0_draw;
        work->mode = PALM_RUN;
        break;
    case PALM_RUN:
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        beach_palm_0_draw(task);
        break;
    case PALM_KILL:
        placement_kill(task);
        break;
    }
}

/* each cluster of leaves takes back the palm's turn about y. it then turns
 * by the camera's y and x angles to face the camera. on this palm alone,
 * the second cluster turns a further quarter turn about y */
static void beach_palm_0_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *leaves_a = &beach_palm_0_leaves_a_object;
    NJS_OBJECT *leaves_b = &beach_palm_0_leaves_b_object;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&beach_palm_0_trunk_model);
    njPushMatrix(NULL);
    njTranslate(NULL, leaves_a->pos[0], leaves_a->pos[1], leaves_a->pos[2]);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    draw_model(leaves_a->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, leaves_b->pos[0], leaves_b->pos[1], leaves_b->pos[2]);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    njRotateY(NULL, NJM_DEG_ANG(90));
    draw_model(leaves_b->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

void beach_palm_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PALM_INIT:
        if (!placement_editing(task)) {
            collision_init(task, &beach_palm_1_shape, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = beach_palm_destroy;
        task->draw = beach_palm_1_draw;
        work->mode = PALM_RUN;
        break;
    case PALM_RUN:
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        beach_palm_1_draw(task);
        break;
    case PALM_KILL:
        placement_kill(task);
        break;
    }
}

static void beach_palm_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *leaves_a = &beach_palm_1_leaves_a_object;
    NJS_OBJECT *leaves_b = &beach_palm_1_leaves_b_object;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&beach_palm_1_trunk_model);
    njPushMatrix(NULL);
    njTranslate(NULL, leaves_a->pos[0], leaves_a->pos[1], leaves_a->pos[2]);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    draw_model(leaves_a->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, leaves_b->pos[0], leaves_b->pos[1], leaves_b->pos[2]);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    draw_model(leaves_b->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

void beach_palm_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PALM_INIT:
        if (!placement_editing(task)) {
            collision_init(task, &beach_palm_2_shape, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = beach_palm_destroy;
        task->draw = beach_palm_2_draw;
        work->mode = PALM_RUN;
        break;
    case PALM_RUN:
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        beach_palm_2_draw(task);
        break;
    case PALM_KILL:
        placement_kill(task);
        break;
    }
}

static void beach_palm_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *leaves_a = &beach_palm_2_leaves_a_object;
    NJS_OBJECT *leaves_b = &beach_palm_2_leaves_b_object;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&beach_palm_2_trunk_model);
    njPushMatrix(NULL);
    njTranslate(NULL, leaves_a->pos[0], leaves_a->pos[1], leaves_a->pos[2]);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    draw_model(leaves_a->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, leaves_b->pos[0], leaves_b->pos[1], leaves_b->pos[2]);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    draw_model(leaves_b->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void beach_palm_destroy(struct task *task)
{
}

void beach_palm_3_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PALM_INIT:
        if (!placement_editing(task)) {
            collision_init(task, &beach_palm_3_shape, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = beach_palm_destroy;
        task->draw = beach_palm_3_draw;
        work->mode = PALM_RUN;
        break;
    case PALM_RUN:
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        beach_palm_3_draw(task);
        break;
    case PALM_KILL:
        placement_kill(task);
        break;
    }
}

static void beach_palm_3_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&beach_palm_3_model);
    njPopMatrix(1);
}
