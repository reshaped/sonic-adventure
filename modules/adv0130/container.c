/* O CONTAINER, a container drawn at its position, with no collision */
#include <ninja.h>

#include "adv0130/container.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT container_object;

enum container_mode { CONTAINER_INIT, CONTAINER_RUN };

/* nothing reads these two cylinders, as the container calls no
 * collision_init. the image holds them in this source's data, at
 * 0x0ca18b40 */
static struct collision_shape container_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     0,
     0,
     {0.0F, 5.0F, -1.0F},
     16.0F,
     5.0F,
     5.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 4.5F, -1.0F},
     15.0F,
     4.5F,
     4.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void container_destroy(struct task *task);
static void container_draw(struct task *task);

void egg_carrier_container_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CONTAINER_INIT:
        task->destructor = container_destroy;
        task->draw = container_draw;
        work->mode = CONTAINER_RUN;
        break;
    case CONTAINER_RUN:
        break;
    }
    container_draw(task);
}

static void container_destroy(struct task *task)
{
}

static void container_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    texture_set_stage();
    draw_tree(&container_object);
    njPopMatrix(1);
}
