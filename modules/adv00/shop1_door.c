/* O SHOP1DOOR double door of the first shop, which swings open away from a
 * player */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/shop1_door.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION shop1_door_action[];

/* the action's frames: open back, shut, and open ahead */
#define DOOR_OPEN_BACK 0.0F
#define DOOR_SHUT 1.0F
#define DOOR_OPEN_AHEAD 2.0F

/* the action frames the door turns through at each step as it opens, and as
 * it shuts */
#define DOOR_OPEN_SPEED 0.125F
#define DOOR_SHUT_SPEED 0.03125F
/* the leaves' turn over the whole action, and at DOOR_OPEN_BACK, half of it */
#define DOOR_TURN NJM_DEG_ANG(160)
#define DOOR_BACK_TURN NJM_DEG_ANG(80)
/* each leaf hangs on a hinge DOOR_HINGE to its side of the door's center.
 * its shape's center lies LEAF_HALF from the hinge */
#define DOOR_HINGE 15.0F
#define LEAF_HALF 7.5F

/* the steps the door waits open. it shakes, less at each step, until DOOR_HOLD
 * steps are left. a touch with fewer steps left sets them back to DOOR_HOLD */
#define DOOR_WAIT 75
#define DOOR_HOLD 60
/* a waiting door's frame is way + DOOR_REST, off by DOOR_SHAKE for each step
 * left past DOOR_HOLD. the offset changes sides at each step */
#define DOOR_REST 0.975F
#define DOOR_SHAKE (0.1F / 60.0F)

/* the bit that is set in an angle from a half turn to a whole one */
#define HALF_TURN 0x8000

/* work->tick counts down the steps the door has left to wait open */
struct shop1_door_vars {
    Float frame;
    Uint8 unk_04[4];
    Float way; /* 1 to open toward DOOR_OPEN_AHEAD, -1 toward DOOR_OPEN_BACK */
};

#define VARS(work) ((struct shop1_door_vars *)&(work)->vars)

static void shop1_door_close(struct task *task);
static void shop1_door_wait(struct task *task);
static void shop1_door_open(struct task *task);
static void shop1_door_idle(struct task *task);

/* the two leaves, shut, each from the door's center to its hinge */
/* clang-format off */
static struct collision_shape shop1_door_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 13.75F, 7.5F},
     1.0F, 13.75F, 7.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 13.75F, -7.5F},
     1.0F, 13.75F, 7.5F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE when a player touches the door */
static Bool shop1_door_touched(struct task_work *work)
{
    return (work->flags & COLLISION_TOUCHING) &&
           work->collision->hit->kind == COLLISION_PLAYER;
}

static void shop1_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(shop1_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* turn each leaf's shape with the action's frame, from DOOR_BACK_TURN at
 * DOOR_OPEN_BACK to -DOOR_BACK_TURN at DOOR_OPEN_AHEAD. then register for
 * this step's collision checks */
static void shop1_door_collide(struct task_work *work)
{
    struct collision_shape *shape = work->collision->shapes;
    struct collision_shape *other;
    Angle ang;
    Float x;
    Float z;

    ang = -floorf(VARS(work)->frame * DOOR_TURN /
                  (shop1_door_action->motion->nbFrame - 1)) +
          DOOR_BACK_TURN;
    x = njSin(ang) * LEAF_HALF;
    z = (njCos(ang) * LEAF_HALF) - DOOR_HINGE;
    other = shape + 1;
    shape->center.z = -z;
    other->center.z = z;
    shape->center.x = other->center.x = -x;
    shape->ang.y = ang;
    other->ang.y = -ang;
    collision_register(work);
}

/* swing shut at DOOR_SHUT_SPEED. a touch on the way opens the door again,
 * the same way */
static void shop1_door_close(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        shop1_door_draw(task);
        if (!shop1_door_touched(work)) {
            VARS(work)->frame -= VARS(work)->way * DOOR_SHUT_SPEED;
            if ((VARS(work)->way > 0.0F && VARS(work)->frame <= DOOR_SHUT) ||
                (VARS(work)->way < 0.0F && VARS(work)->frame >= DOOR_SHUT)) {
                VARS(work)->frame = DOOR_SHUT;
                task->step = shop1_door_idle;
            }
        } else {
            task->step = shop1_door_open;
        }
        shop1_door_collide(work);
    }
}

/* wait open until work->tick runs out, then close */
static void shop1_door_wait(struct task *task)
{
    struct task_work *work = task->work;
    int shake;

    if (!placement_try_leave(task)) {
        shop1_door_draw(task);
        if (work->tick < DOOR_HOLD && shop1_door_touched(work)) {
            work->tick = DOOR_HOLD;
        }
        if (work->tick) {
            work->tick--;
            shake = work->tick - DOOR_HOLD;
            if (shake < 0) {
                shake = 0;
            }
            if (shake & 1) {
                shake = -shake;
            }
            VARS(work)->frame =
                VARS(work)->way + (shake * DOOR_SHAKE) + DOOR_REST;
        } else {
            task->step = shop1_door_close;
        }
        shop1_door_collide(work);
    }
}

static void shop1_door_open(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        shop1_door_draw(task);
        VARS(work)->frame += VARS(work)->way * DOOR_OPEN_SPEED;
        /* @hack spelling: the goto, which gives both ends one copy of the
         * wait's start, at 0x0c9034e8, and the back end a BRA to it at
         * 0x0c9034d6; with a copy for each end, 23 of 46 words differ */
        if (VARS(work)->frame <= DOOR_OPEN_BACK) {
            VARS(work)->frame = DOOR_OPEN_BACK;
            goto open;
        } else if (VARS(work)->frame >= DOOR_OPEN_AHEAD) {
            VARS(work)->frame = DOOR_OPEN_AHEAD;
        open:
            work->tick = DOOR_WAIT;
            task->step = shop1_door_wait;
        }
        shop1_door_collide(work);
    }
}

/* wait shut until a player touches the door, then open it away from that
 * player */
static void shop1_door_idle(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    Angle ang;

    if (placement_editing(task)) {
        shop1_door_draw(task);
    } else if (!placement_try_leave(task)) {
        shop1_door_draw(task);
        if (shop1_door_touched(work)) {
            player = work->collision->hit->owner->work;
            ang = (Angle)(atan2f(player->pos.x - work->pos.x,
                                 player->pos.z - work->pos.z) *
                          ANGLE_PER_RADIAN) -
                  work->ang.y;
            VARS(work)->way = (ang & HALF_TURN) ? 1.0F : -1.0F;
            task->step = shop1_door_open;
        }
        shop1_door_collide(work);
    }
}

void shop1_door_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        collision_init(task, shop1_door_shapes, COLLISION_OBJECT, 1, 2);
        task->step = shop1_door_idle;
        task->draw = shop1_door_draw;
        VARS(work)->frame = DOOR_SHUT;
    }
}
