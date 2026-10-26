/* BanjuDoor: a door that sinks while CASINO_BANJU_DOOR_OPEN is set, and
 * rises while it is clear */
#include <ninja.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "object/common.h"
#include "stg09/banju_door.h"
#include "stg09/casino.h"
#include "stg09/pinball.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL banju_door_model;

/* the heights the door rises to and sinks to, whatever its placement's y */
#define TOP (-240.0F)
#define BOTTOM (-330.0F)
#define SPEED 5.0F /* its move each step */

static struct collision_shape banju_door_shape = {
    SHAPE_RECTANGLE,
    ATTACK(3),
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 40.0F, 10.0F},
    30.0F,
    42.0F,
    1.0F,
    {0},
    0,
    NULL,
};

static void banju_door_destroy(struct task *task)
{
}

static void banju_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&banju_door_model);
    njPopMatrix(1);
}

static void banju_door_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!(casino_flags & CASINO_BANJU_DOOR_OPEN)) {
        if (work->pos.y != TOP) {
            work->pos.y += SPEED;
            if (work->pos.y > TOP) {
                work->pos.y = TOP;
            }
        }
    } else {
        if (work->pos.y != BOTTOM) {
            work->pos.y -= SPEED;
            if (work->pos.y < BOTTOM) {
                work->pos.y = BOTTOM;
            }
        }
    }
    banju_door_draw(task);
    collision_register(work);
    object_clear_touch(work, NULL);
}

void banju_door_step(struct task *task)
{
    casino_flags &= ~CASINO_BANJU_DOOR_OPEN;
    collision_init(task, &banju_door_shape, COLLISION_OBJECT, 1, 1);
    task->step = banju_door_run;
    task->draw = banju_door_draw;
    task->destructor = banju_door_destroy;
}
