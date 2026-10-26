/* ROCKET, a rocket in the tornado's way. a touch by a player, a shot or an
 * enemy sets it off in a bomb */
#include <ninja.h>

#include "collision/collision.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "shooting/rocket.h"
#include "system/task.h"
#include "system/util.h"

enum shooting_rocket_mode {
    ROCKET_INIT = 0,
    ROCKET_WAIT = 1,
    ROCKET_BLOW = 3,
    ROCKET_GONE = 4
};

extern NJS_TEXLIST shooting0_texlist;
extern NJS_OBJECT shooting_rocket_object;

static void shooting_rocket_draw(struct task *task);
static void shooting_rocket_destroy(struct task *task);
static void shooting_rocket_take_touch(struct task *task);

/* a cylinder2 7 in radius and 40 long. its own quarter turn about x lays it
 * on its side */
static struct collision_shape shooting_rocket_shapes[1] = {
    {SHAPE_CYLINDER2 | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     7.0F,
     20.0F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
};

void shooting_rocket_step(struct task *task)
{
    struct task_work *work = task->work;

    shooting_rocket_take_touch(task);
    switch (work->mode) {
    case ROCKET_INIT:
        work->mode = ROCKET_WAIT;
        work->vars.f[0] = 0.0F; /* nothing reads it */
        task->destructor = shooting_rocket_destroy;
        task->draw = shooting_rocket_draw;
        collision_init(task, shooting_rocket_shapes, COLLISION_OBJECT, 1, 1);
        break;
    case ROCKET_WAIT:
        collision_register(work);
        break;
    case ROCKET_BLOW:
        bomb_create(&work->pos, 2.0F);
        work->mode = ROCKET_GONE;
        break;
    case ROCKET_GONE:
        task_destroy(task);
        break;
    }
    shooting_rocket_draw(task);
}

static void shooting_rocket_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&shooting0_texlist);
    draw_tree(&shooting_rocket_object);
    njPopMatrix(1);
}

static void shooting_rocket_destroy(struct task *task)
{
}

static void shooting_rocket_take_touch(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_TOUCHING) {
        work->mode = ROCKET_BLOW;
    }
}
