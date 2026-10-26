/* O LIGHT, a siren light whose mirror turns, and O LIGHTB, a yellow lamp that
 * stands still */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg05/light.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_MODEL mountain_light_model;
extern NJS_OBJECT mountain_light_mirror_object;
extern NJS_OBJECT mountain_light_mirror_back_object;
extern NJS_OBJECT mountain_light_box_object;
extern NJS_OBJECT mountain_light_b_object;

/* the mirror's angle about y, in degrees */
#define MIRROR_ANGLE(work) TASK_TIMER(work)

/* no step sets LIGHT_IDLE, in which the step does nothing */
enum mountain_light_mode { LIGHT_INIT, LIGHT_RUN, LIGHT_IDLE };

/* the mirror's turn each step, in degrees */
#define MIRROR_TURN 12

static struct collision_shape mountain_light_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    5.0F,
    7.0F,
    0.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape mountain_light_b_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 10.0F, 0.0F},
    5.0F,
    7.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void mountain_light_draw(struct task *task);
static void mountain_light_destroy(struct task *task);

/* @hack spelling: the turn as a helper SHC expands; written in place, the
 * step reaches the work through another register, is 4 bytes shorter, and 27
 * of its 65 words differ */
static void mountain_light_turn(struct task *task)
{
    struct task_work *work = task->work;

    MIRROR_ANGLE(work) += MIRROR_TURN;
}

void mountain_light_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case LIGHT_INIT:
        if (!placement_editing(task)) {
            collision_init(task, &mountain_light_shape, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = mountain_light_destroy;
        task->draw = mountain_light_draw;
        work->mode = LIGHT_RUN;
        break;
    case LIGHT_RUN:
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        mountain_light_turn(task);
        mountain_light_draw(task);
        break;
    case LIGHT_IDLE:
        break;
    }
}

static void mountain_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 50.0F)) {
        return;
    }
    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&mountain_light_model);
    njPushMatrix(NULL);
    object = &mountain_light_mirror_object;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateZ, object->ang[2], ang);
    TURN_VIA(njRotateX, object->ang[0], ang);
    TURN_VIA(njRotateY, object->ang[1], ang);
    TURN_VIA(njRotateY, MIRROR_ANGLE(work) * ANGLE_PER_DEGREE, ang);
    draw_model(object->model);
    object = &mountain_light_mirror_back_object;
    njRotateY(NULL, 0x8000);
    draw_model(object->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    object = &mountain_light_box_object;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateZ, object->ang[2], ang);
    TURN_VIA(njRotateX, object->ang[0], ang);
    TURN_VIA(njRotateY, object->ang[1], ang);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void mountain_light_destroy(struct task *task)
{
}

/* O LIGHTB, a lamp model that stands still, solid unless the set editor
 * holds it */
void mountain_light_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &mountain_light_b_object;
    if (!placement_editing(task)) {
        collision_init(task, &mountain_light_b_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}
