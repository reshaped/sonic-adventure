/* O SIGNBOARD, a solid signboard */
#include <ninja.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg05/signboard.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT mountain_signboard_object;

enum signboard_mode { SIGNBOARD_INIT, SIGNBOARD_SHOWN };

#define DRAW_RADIUS 100.0F

static struct collision_shape mountain_signboard_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.5F},
    5.8F,
    5.8F,
    0.6F,
    {0},
    0,
    NULL,
};

static void mountain_signboard_draw(struct task *task);
static void mountain_signboard_destroy(struct task *task);

void mountain_signboard_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case SIGNBOARD_INIT:
            work->mode = SIGNBOARD_SHOWN;
            task->draw = mountain_signboard_draw;
            task->destructor = mountain_signboard_destroy;
            collision_init(task, &mountain_signboard_shape, COLLISION_OBJECT, 1,
                           1);
            break;
        case SIGNBOARD_SHOWN:
            mountain_signboard_draw(task);
            collision_register(work);
            break;
        }
    }
}

static void mountain_signboard_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&mountain_signboard_object);
        njPopMatrix(1);
    }
}

static void mountain_signboard_destroy(struct task *task)
{
}
