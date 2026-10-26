/* O KANBANA and O KANBANB solid shop signs, for drinks and for detergent,
 * hanging over the street */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg04/shop_sign.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL shop_sign_a_model;
extern NJS_OBJECT shop_sign_a_object;
extern NJS_MODEL shop_sign_b_model;
extern NJS_OBJECT shop_sign_b_object;

/* the radius of a sign's collision record. its sphere reaches 10 from its
 * position */
#define SHOP_SIGN_REACH 10.0F

/* the sign's modes, in its work->mode, which skip 2. no code sets
 * SHOP_SIGN_GONE */
enum shop_sign_mode {
    SHOP_SIGN_INIT = 0,
    SHOP_SIGN_HANG = 1,
    SHOP_SIGN_GONE = 3
};

static void shop_sign_a_draw(struct task *task);
static void shop_sign_a_destroy(struct task *task);
static void shop_sign_b_draw(struct task *task);
static void shop_sign_b_destroy(struct task *task);

static struct collision_shape shop_sign_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, -5.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

void shop_sign_a_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SHOP_SIGN_INIT:
        MODEL(work)->object = &shop_sign_a_object;
        if (!placement_editing(task)) {
            collision_init(task, &shop_sign_shape, COLLISION_OBJECT, 1, 1);
            work->collision->radius = SHOP_SIGN_REACH;
        }
        task->destructor = shop_sign_a_destroy;
        task->draw = shop_sign_a_draw;
        work->mode = SHOP_SIGN_HANG;
        break;
    case SHOP_SIGN_HANG:
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        shop_sign_a_draw(task);
        break;
    case SHOP_SIGN_GONE:
        break;
    }
}

static void shop_sign_a_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    draw_model(&shop_sign_a_model);
    njPopMatrix(1);
}

static void shop_sign_a_destroy(struct task *task)
{
}

void shop_sign_b_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SHOP_SIGN_INIT:
        MODEL(work)->object = &shop_sign_b_object;
        if (!placement_editing(task)) {
            collision_init(task, &shop_sign_shape, COLLISION_OBJECT, 1, 1);
            work->collision->radius = SHOP_SIGN_REACH;
        }
        task->destructor = shop_sign_b_destroy;
        task->draw = shop_sign_b_draw;
        work->mode = SHOP_SIGN_HANG;
        break;
    case SHOP_SIGN_HANG:
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        shop_sign_b_draw(task);
        break;
    case SHOP_SIGN_GONE:
        break;
    }
}

static void shop_sign_b_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    draw_model(&shop_sign_b_model);
    njPopMatrix(1);
}

static void shop_sign_b_destroy(struct task *task)
{
}
