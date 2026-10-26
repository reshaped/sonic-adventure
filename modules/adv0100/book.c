/* O BOOK1, O BOOK2 and O BOOK3 books, each a box a player can stand on */
#include <ninja.h>

#include "adv0100/book.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT book1_object;
extern NJS_OBJECT book2_object;
extern NJS_OBJECT book3_object;

#define SHAPE(work) ((work)->submode)

enum book_mode { BOOK_INIT, BOOK_RUN };

static void book_step(struct task *task);
static void book_draw(struct task *task);
static void book_destroy(struct task *task);
static void book_draw_work(struct task_work *work);

static struct collision_shape book_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL,
     {0.0F, 1.0F, 0.0F},
     2.5F,
     1.0F,
     2.5F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL,
     {0.0F, 1.0F, 0.0F},
     2.5F,
     1.0F,
     2.5F,
     {0, 0, 0},
     0,
     NULL},
};

void egg_carrier_book1_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &book1_object;
    SHAPE(work) = 1;
    task->step = book_step;
}

void egg_carrier_book2_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &book2_object;
    SHAPE(work) = 0;
    task->step = book_step;
}

void egg_carrier_book3_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &book3_object;
    SHAPE(work) = 0;
    task->step = book_step;
}

static void book_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BOOK_INIT:
        if (!placement_editing(task)) {
            collision_init(task, &book_shapes[SHAPE(work)], COLLISION_OBJECT, 1,
                           1);
        }
        task->destructor = book_destroy;
        task->draw = book_draw;
        work->mode = BOOK_RUN;
        break;
    case BOOK_RUN:
        break;
    }
    book_draw_work(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void book_draw(struct task *task)
{
    struct task_work *work = task->work;

    book_draw_work(work);
}

static void book_destroy(struct task *task)
{
}

static void book_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}
