/* O SHOP2DOOR double door of the second shop, which opens only while an egg is
 * on the stand */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/egg_stand.h"
#include "adv00/shop2_door.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION shop2_door_action[];

/* the action's frames: open back, shut, and open ahead */
#define DOOR_OPEN_BACK 0.0F
#define DOOR_SHUT 1.0F
#define DOOR_OPEN_AHEAD 2.0F

/* the distance within which a player opens the door */
#define DOOR_REACH 20.0F
/* the action frames the door turns through at each step as it opens, and as
 * it shuts */
#define DOOR_OPEN_SPEED 0.125F
#define DOOR_SHUT_SPEED 0.03125F
/* the leaves' turn over the whole action, and at DOOR_OPEN_BACK, half of it */
#define DOOR_TURN NJM_DEG_ANG(160)
#define DOOR_BACK_TURN NJM_DEG_ANG(80)
/* each leaf hangs on a hinge DOOR_HINGE to its side of the door's center.
 * its shape's center lies LEAF_HALF from the hinge */
#define DOOR_HINGE 17.75F
#define LEAF_HALF 8.875F

/* the steps the door waits open. it shakes, less at each step, until DOOR_HOLD
 * steps are left. a player near with fewer steps left sets them back to
 * DOOR_HOLD */
#define DOOR_WAIT 75
#define DOOR_HOLD 60
/* a waiting door's frame is way + DOOR_REST, off by DOOR_SHAKE for each step
 * left past DOOR_HOLD. the offset changes sides at each step */
#define DOOR_REST 0.975F
#define DOOR_SHAKE (0.1F / 60.0F)

/* the bit that is set in an angle from a half turn to a whole one */
#define HALF_TURN 0x8000

/* work->tick counts down the steps the door has left to wait open */
struct shop2_door_vars {
    /* the doorway's wall in the world, which world_object_alloc lends */
    NJS_OBJECT *wall;
    Float frame;
    Float way; /* 1 to open toward DOOR_OPEN_AHEAD, -1 toward DOOR_OPEN_BACK */
};

#define VARS(work) ((struct shop2_door_vars *)&(work)->vars)

enum shop2_door_shape {
    LEAF_AHEAD,
    LEAF_BACK,
    DOOR_SHUT_SHAPE, /* the doorway, which both collide functions turn off */
    JAMB_AHEAD,
    JAMB_BACK,
    DOOR_SHAPES
};

static void shop2_door_close(struct task *task);
static void shop2_door_wait(struct task *task);
static void shop2_door_open(struct task *task);
static void shop2_door_idle(struct task *task);

/* clang-format off */
static struct collision_shape shop2_door_shapes[DOOR_SHAPES] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 13.75F, 7.5F},
     1.0F, 13.75F, 7.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 13.75F, -7.5F},
     1.0F, 13.75F, 7.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 13.75F, 0.0F},
     1.0F, 30.0F, 20.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 13.75F, 20.0F},
     5.0F, 30.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 13.75F, -20.0F},
     5.0F, 30.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* the wall the door puts in the world across its doorway: a quad 40 wide
 * and 30 high */
static NJS_MATERIAL shop2_door_wall_materials[1] = {
#include "adv00/shop2_door/shop2_door_wall_materials.inc"
};

static Sint16 shop2_door_wall_meshes[4] = {
#include "adv00/shop2_door/shop2_door_wall_meshes.inc"
};

static NJS_MESHSET shop2_door_wall_meshsets[1] = {
#include "adv00/shop2_door/shop2_door_wall_meshsets.inc"
};

static NJS_POINT3 shop2_door_wall_points[4] = {
#include "adv00/shop2_door/shop2_door_wall_points.inc"
};

static NJS_VECTOR shop2_door_wall_normals[4] = {
#include "adv00/shop2_door/shop2_door_wall_normals.inc"
};

static NJS_MODEL shop2_door_wall_model = {
#include "adv00/shop2_door/shop2_door_wall_model.inc"
};

static NJS_OBJECT shop2_door_wall[1] = {
#include "adv00/shop2_door/shop2_door_wall.inc"
};

/* which of players 0 and 1, counted from 1, is within DOOR_REACH of the
 * door: 0 for neither. no egg on the stand gives 0 too */
static int shop2_door_near(struct task_work *work)
{
    if (!event_flags[FLAG_EGG_ON_STAND]) {
        return 0;
    }
    return player_in_range(&work->pos, DOOR_REACH);
}

static void shop2_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(shop2_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* turn each leaf's shape with the action's frame, from DOOR_BACK_TURN at
 * DOOR_OPEN_BACK to -DOOR_BACK_TURN at DOOR_OPEN_AHEAD.
 * world_collect_near_players then passes over the wall, and the door
 * registers for this step's collision checks */
static void shop2_door_collide_leaves(struct task_work *work)
{
    struct collision_shape *shape = work->collision->shapes;
    struct collision_shape *other = shape + 1;
    Angle ang;
    Float x;
    Float z;

    shape[LEAF_AHEAD].flags &= ~SHAPE_OFF;
    shape[LEAF_BACK].flags &= ~SHAPE_OFF;
    shape[DOOR_SHUT_SHAPE].flags |= SHAPE_OFF;
    shape[JAMB_AHEAD].flags &= ~SHAPE_OFF;
    shape[JAMB_BACK].flags &= ~SHAPE_OFF;
    ang = -floorf(VARS(work)->frame * DOOR_TURN /
                  (shop2_door_action->motion->nbFrame - 1)) +
          DOOR_BACK_TURN;
    x = njSin(ang) * LEAF_HALF;
    z = (njCos(ang) * LEAF_HALF) - DOOR_HINGE;
    shape->center.z = -z;
    other->center.z = z;
    shape->center.x = other->center.x = -x;
    shape->ang.y = ang;
    other->ang.y = -ang;
    work->flags &= ~WORLD_TESTED;
    collision_register(work);
}

/* turn the leaves' shapes off, and let world_collect_near_players take the
 * wall in their place. then register for this step's collision checks */
static void shop2_door_collide_wall(struct task_work *work)
{
    struct collision_shape *shape = work->collision->shapes;

    shape[LEAF_AHEAD].flags |= SHAPE_OFF;
    shape[LEAF_BACK].flags |= SHAPE_OFF;
    shape[DOOR_SHUT_SHAPE].flags |= SHAPE_OFF;
    shape[JAMB_AHEAD].flags &= ~SHAPE_OFF;
    shape[JAMB_BACK].flags &= ~SHAPE_OFF;
    work->flags |= WORLD_TESTED;
    collision_register(work);
}

/* swing shut at DOOR_SHUT_SPEED. a player near on the way opens the door
 * again, the same way */
static void shop2_door_close(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        shop2_door_draw(task);
        if (!shop2_door_near(work)) {
            VARS(work)->frame -= VARS(work)->way * DOOR_SHUT_SPEED;
            if ((VARS(work)->way > 0.0F && VARS(work)->frame <= DOOR_SHUT) ||
                (VARS(work)->way < 0.0F && VARS(work)->frame >= DOOR_SHUT)) {
                VARS(work)->frame = DOOR_SHUT;
                task->step = shop2_door_idle;
            }
        } else {
            task->step = shop2_door_open;
        }
        shop2_door_collide_leaves(work);
    }
}

/* wait open until work->tick runs out, then close */
static void shop2_door_wait(struct task *task)
{
    struct task_work *work = task->work;
    int shake;

    if (!placement_try_leave(task)) {
        shop2_door_draw(task);
        if (work->tick < DOOR_HOLD && shop2_door_near(work)) {
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
            task->step = shop2_door_close;
        }
        shop2_door_collide_leaves(work);
    }
}

static void shop2_door_open(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        shop2_door_draw(task);
        VARS(work)->frame += VARS(work)->way * DOOR_OPEN_SPEED;
        /* @hack spelling: the goto, which gives both ends one copy of the
         * wait's start, at 0x0c903c6e, and the back end a BRA to it at
         * 0x0c903c28; with a copy for each end, 45 of 46 words differ */
        if (VARS(work)->frame <= DOOR_OPEN_BACK) {
            VARS(work)->frame = DOOR_OPEN_BACK;
            goto open;
        } else if (VARS(work)->frame >= DOOR_OPEN_AHEAD) {
            VARS(work)->frame = DOOR_OPEN_AHEAD;
        open:
            work->tick = DOOR_WAIT;
            task->step = shop2_door_wait;
        }
        shop2_door_collide_leaves(work);
    }
}

/* wait shut until a player comes near. the door then opens away from
 * player 0, even when player 1 came near */
static void shop2_door_idle(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 player;
    Angle ang;

    if (placement_editing(task)) {
        shop2_door_draw(task);
    } else if (!placement_try_leave(task)) {
        shop2_door_draw(task);
        if (shop2_door_near(work)) {
            player_get_history(0, 0, &player, NULL);
            ang =
                (Angle)(atan2f(player.x - work->pos.x, player.z - work->pos.z) *
                        ANGLE_PER_RADIAN) -
                work->ang.y;
            VARS(work)->way = (ang & HALF_TURN) ? 1.0F : -1.0F;
            task->step = shop2_door_open;
        }
        shop2_door_collide_wall(work);
    }
}

static void shop2_door_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->wall) {
        world_remove(task, VARS(work)->wall);
        world_object_free(VARS(work)->wall);
    }
}

void shop2_door_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *wall;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        wall = world_object_alloc();
        VARS(work)->wall = wall;
        wall->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
        wall->scl[0] = 1.0F;
        wall->scl[1] = 1.0F;
        wall->scl[2] = 1.0F;
        wall->ang[0] = 0;
        wall->ang[1] = work->ang.y;
        wall->ang[2] = 0;
        wall->pos[0] = work->pos.x;
        wall->pos[1] = work->pos.y;
        wall->pos[2] = work->pos.z;
        wall->model = shop2_door_wall->model;
        wall->child = NULL;
        wall->sibling = NULL;
        world_add(WORLD_SMALL | WORLD_MOVES | WORLD_SOLID, task, wall);
        work->flags |= WORLD_TESTED;
        collision_init(task, shop2_door_shapes, COLLISION_OBJECT, 1,
                       DOOR_SHAPES);
    }
    task->step = shop2_door_idle;
    task->draw = shop2_door_draw;
    task->destructor = shop2_door_destroy;
    VARS(work)->frame = DOOR_SHUT;
}
