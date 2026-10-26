/* O CONTAINER shipping container, placed separately for Big and for the other
 * characters */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/container.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT container_object;

/* a placement's scale x: the characters its container is for */
#define FOR_BIG 1.0F
#define FOR_OTHERS 0.0F

/* Big's box is smaller than the others' */
/* clang-format off */
static struct collision_shape container_big_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 15.0F, 0.0F},
    15.0F, 15.0F, 15.0F, {0}, 0, NULL,
};
static struct collision_shape container_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 20.0F, 0.0F},
    20.0F, 20.0F, 15.0F, {0}, 0, NULL,
};
/* clang-format on */

static void container_step(struct task *task);
static void container_draw(struct task *task);
static void container_destroy(struct task *task);

static Bool container_for_big(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
    case CHARACTER_TAILS:
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
        break;
    case CHARACTER_BIG:
        return TRUE;
    }
    return FALSE;
}

void station_square_container_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = 0;
    motion->speed.x = 0.0F;
    motion->speed.y = 0.0F;
    motion->speed.z = 0.0F;
    if (placement_editing(task)) {
        container_draw(task);
    } else {
        /* @hack spelling: the step set in each branch, which keeps a
         * collision_init call in each (JSR at 0x0c90ede0 and 0x0c90ee0a). set
         * once after the test, SHC merges the calls and 41 of 75 words
         * differ */
        if (container_for_big()) {
            if (work->scl.x != FOR_BIG) {
                placement_kill(task);
                return;
            }
            collision_init(task, &container_big_shape, COLLISION_OBJECT, 1, 1);
            task->step = container_step;
        } else {
            if (work->scl.x != FOR_OTHERS) {
                placement_kill(task);
                return;
            }
            collision_init(task, &container_shape, COLLISION_OBJECT, 1, 1);
            task->step = container_step;
        }
    }
    task->draw = container_draw;
    task->destructor = container_destroy;
}

static void container_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        collision_register(work);
        container_draw(task);
    }
}

static void container_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&container_object);
    njPopMatrix(1);
}

static void container_destroy(struct task *task)
{
}
