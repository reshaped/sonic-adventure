/* O SNAKE, the snake on the pyramid, a model that only draws */
#include <ninja.h>

#include "adv03/past.h"
#include "adv03/pyramid_snake.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

enum pyramid_snake_mode { SNAKE_INIT, SNAKE_SHOWN };

static void pyramid_snake_draw(struct task *task);

void pyramid_snake_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SNAKE_INIT:
        task->draw = pyramid_snake_draw;
        work->mode = SNAKE_SHOWN;
        break;
    case SNAKE_SHOWN:
        break;
    }
    pyramid_snake_draw(task);
}

static void pyramid_snake_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(&pyramid_snake_object);
    njPopMatrix(1);
}
