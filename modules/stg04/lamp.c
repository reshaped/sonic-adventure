/* O LAMP street lamp with two glows that face the camera, and O LAMP01 and
 * O LAMP02 lamps on arms */
#include <ninja.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg04/lamp.h"
#include "system/cache.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL highway_lamp_model;
extern NJS_OBJECT highway_lamp_object;
extern NJS_MODEL highway_lamp_glow_1_model;
extern NJS_OBJECT highway_lamp_glow_1_object;
extern NJS_MODEL highway_lamp_glow_2_model;
extern NJS_OBJECT highway_lamp_glow_2_object;
extern NJS_OBJECT highway_lamp_1_object;
extern NJS_OBJECT highway_lamp_2_object;

/* the lamp's modes, in its work->mode. no code sets HIGHWAY_LAMP_GONE */
enum highway_lamp_mode {
    HIGHWAY_LAMP_INIT,
    HIGHWAY_LAMP_STAND,
    HIGHWAY_LAMP_GONE
};

static void highway_lamp_clear_hit(struct task *task);
static void highway_lamp_draw(struct task *task);
static void highway_lamp_destroy(struct task *task);

static struct collision_shape highway_lamp_shape = {
    SHAPE_CYLINDER, 0, 0, {0.0F, 20.0F, 0.0F}, 3.0F, 20.0F, 0.0F, {0}, 0, NULL,
};

void highway_lamp_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case HIGHWAY_LAMP_INIT:
        MODEL(work)->object = &highway_lamp_object;
        if (!placement_editing(task)) {
            collision_init(task, &highway_lamp_shape, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = highway_lamp_destroy;
        task->draw = highway_lamp_draw;
        work->mode = HIGHWAY_LAMP_STAND;
        break;
    case HIGHWAY_LAMP_STAND:
        if (!placement_editing(task)) {
            highway_lamp_clear_hit(task);
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        highway_lamp_draw(task);
        break;
    case HIGHWAY_LAMP_GONE:
        break;
    }
}

static void highway_lamp_clear_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->flags & COLLISION_HIT) {
        work->collision->flags &= ~COLLISION_HIT;
    }
}

/* on a frame's first step, draw the post, then a glow at each glow node.
 * each glow turns to face the camera and draws without fog. at each node, the
 * glow is the other node's model, but the two models are the same square */
static void highway_lamp_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *glow_2;
    NJS_OBJECT *glow_1;

    if (frame_step) {
        return;
    }
    glow_2 = &highway_lamp_glow_2_object;
    glow_1 = &highway_lamp_glow_1_object;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    draw_model(&highway_lamp_model);
    njPushMatrix(NULL);
    njTranslate(NULL, glow_2->pos[0], glow_2->pos[1], glow_2->pos[2]);
    TURN(njRotateY, -work->ang.y);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateX, camera_work->ang.x);
    njFogDisable();
    draw_model(&highway_lamp_glow_1_model);
    matrix_load_parent();
    njTranslate(NULL, glow_1->pos[0], glow_1->pos[1], glow_1->pos[2]);
    TURN(njRotateY, -work->ang.y);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateX, camera_work->ang.x);
    draw_model(&highway_lamp_glow_2_model);
    njFogEnable();
    njPopMatrix(2);
}

static void highway_lamp_destroy(struct task *task)
{
}

void highway_lamp_1_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &highway_lamp_1_object;
    if (!placement_editing(task)) {
        collision_init(task, &highway_lamp_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void highway_lamp_2_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &highway_lamp_2_object;
    if (!placement_editing(task)) {
        collision_init(task, &highway_lamp_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}
