/* O Nbox1 to O Nbox4 solid newspaper boxes */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg04/news_box.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT news_box_1_object;
extern NJS_OBJECT news_box_2_object;
extern NJS_OBJECT news_box_3_object;
extern NJS_OBJECT news_box_4_object;

/* the radius of a box's collision record. its shape's corners lie 11.49
 * from its position */
#define NEWS_BOX_REACH 11.5F

enum news_box_kind { NEWS_BOX_1, NEWS_BOX_2, NEWS_BOX_3, NEWS_BOX_4 };

struct news_box_vars {
    enum news_box_kind kind;
};

#define VARS(work) ((struct news_box_vars *)&(work)->vars)

static void news_box_destroy(struct task *task);
static void news_box_stand(struct task *task);
static void news_box_start(struct task *task);

static struct collision_shape news_box_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    4.0F,
    10.0F,
    4.0F,
    {0},
    0,
    NULL,
};

static void news_box_destroy(struct task *task)
{
}

/* the box's step and draw */
static void news_box_stand(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    njPushMatrix(NULL);
    texture_set_stage();
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    switch (VARS(work)->kind) {
    case NEWS_BOX_1:
        object = &news_box_1_object;
        break;
    case NEWS_BOX_2:
        object = &news_box_2_object;
        break;
    case NEWS_BOX_3:
        object = &news_box_3_object;
        break;
    case NEWS_BOX_4:
        object = &news_box_4_object;
        break;
    }
    draw_tree(object);
    njPopMatrix(1);
}

static void news_box_start(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &news_box_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = NEWS_BOX_REACH;
    task->destructor = news_box_destroy;
    task->step = news_box_stand;
    task->draw = news_box_stand;
}

void news_box_1_step(struct task *task)
{
    VARS(task->work)->kind = NEWS_BOX_1;
    news_box_start(task);
}

void news_box_2_step(struct task *task)
{
    VARS(task->work)->kind = NEWS_BOX_2;
    news_box_start(task);
}

void news_box_3_step(struct task *task)
{
    VARS(task->work)->kind = NEWS_BOX_3;
    news_box_start(task);
}

void news_box_4_step(struct task *task)
{
    VARS(task->work)->kind = NEWS_BOX_4;
    news_box_start(task);
}
