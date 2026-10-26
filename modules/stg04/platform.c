/* O STP4S platform that sinks under a rider and rises back, O STP4T one
 * that stands, and O FLYST one that flies to and fro */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "stg04/platform.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT platform_sink_object;
extern NJS_OBJECT platform_object;
extern NJS_ACTION platform_fly_action;
/* the flying platform's model in the collision world, which its action
 * doesn't move */
extern NJS_OBJECT platform_fly_object;

/* the flags of each platform's copy in the collision world */
#define PLATFORM_FLAGS (WORLD_LARGE | WORLD_TURNED | WORLD_MOVES)

/* the sinking platform's modes, in its work->mode */
enum sink_mode { SINK_INIT, SINK_RUN, SINK_STILL /* a SINK SPEED of 0 */ };

#define SINK_SPEED_KEPT 0.95F
#define SINK_RISE 0.1F

/* world_collect_near_players takes in a sinking platform's copy only while a
 * player is within SINK_NEAR. SINK_TARGET is the radius of the target it
 * offers a falling player, and its collision record's reach */
#define SINK_NEAR 48.28F
#define SINK_TARGET 28.28F

/* the flying platform's modes, in its work->mode */
enum fly_mode {
    FLY_INIT,
    FLY_MOVE,
    FLY_STILL, /* a MOVEMENT RANGE or a speed of 0 */
    FLY_STOPPED /* nothing sets it */
};

/* which way the flying platform flies, in work->submode. FORTH and BACK run
 * along the axis TYPE picks, turned by the set editor's ROTATION. past its
 * range from where it was put, it turns about. nothing sets FLY_WAY_6 */
enum fly_way {
    FLY_X_FORTH,
    FLY_X_BACK,
    FLY_Z_FORTH,
    FLY_Z_BACK,
    FLY_UP,
    FLY_DOWN,
    FLY_WAY_6
};

/* a TYPE above TYPE_Y flies the platform up and down. one above 0 flies it
 * along z, and any other along x */
#define TYPE_Y 10.0F

/* as SINK_NEAR and SINK_TARGET, for the flying platform. it has no collision
 * record */
#define FLY_NEAR 97.78F
#define FLY_TARGET 77.78F

struct fly_vars {
    /* the copy in the collision world. it sits where struct world_model_vars
     * has its world field, so WORLD_MODEL reaches it too */
    NJS_OBJECT *world;
    Float frame; /* the action's frame */
    NJS_ACTION *action;
};

#define FLY(work) ((struct fly_vars *)&(work)->vars)

/* the flying platform's task->extra */
struct fly_travel {
    Float range; /* MOVEMENT RANGE without its sign */
    Float speed; /* MOVEMENT RATE + 1: how far it flies each step */
};

/* the sinking platform's box, 40 across and 5 high. its touch finds a player
 * on the platform */
/* clang-format off */
static struct collision_shape platform_sink_shape = {
    SHAPE_RECTANGLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
    {0.0F, 7.5F, 0.0F}, 20.0F, 2.5F, 20.0F, {0}, 0, NULL};
/* clang-format on */

/* put a copy of model in the collision world under PLATFORM_FLAGS. the copy
 * stands at the platform, turned only about y. WORLD_MODEL(work)->world
 * keeps it */
static void platform_add_world(struct task *task, NJS_OBJECT *model)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    WORLD_MODEL(work)->world = object = world_object_alloc();
    object->evalflags = model->evalflags;
    object->scl[0] = object->scl[1] = object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = model->model;
    object->child = model->child;
    object->sibling = model->sibling;
    world_add(PLATFORM_FLAGS, task, object);
}

/* move the platform's copy in the world to the platform. while a player is
 * within near, set WORLD_TESTED, so world_collect_near_players takes it in */
static void platform_follow(struct task *task, Float near)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = WORLD_MODEL(work)->world;

    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    if (player_in_range(&work->pos, near)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void platform_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->world) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
    task_kill(task);
}

static void platform_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(WORLD_MODEL(work)->world);
    njPopMatrix(1);
}

/* the sinking platform's step. scl.x starts as the set editor's SINK SPEED
 * made positive, and scl.z keeps that speed. each step a player stands on
 * it, the platform sinks by that player's gravity times scl.x. scl.x then
 * falls to SINK_SPEED_KEPT of itself. left alone, the platform rises by
 * SINK_RISE a step to scl.y, the height it was placed at. once there, scl.x
 * takes back scl.z's speed */
void platform_sink_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    Sint8 player;
    struct player *rider;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&platform_sink_object);
        njPopMatrix(1);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> NOT USE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "> NOT USE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> SINK SPEED");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> NOT USE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "> NOT USE");
        }
    } else {
        switch (work->mode) {
        case SINK_INIT:
            if (work->scl.x == 0.0F) {
                work->mode = SINK_STILL;
            } else {
                work->mode = SINK_RUN;
                if (work->scl.x < 0.0F) {
                    work->scl.x *= -1.0F;
                }
                work->scl.y = work->pos.y;
                work->scl.z = work->scl.x;
                /* @hack redundant: 1s selected by a constant condition, which
                 * SHC loads into R3 where it passes them (0x0c909f60). as 1,
                 * SHC keeps a 1 in a register through the whole step, and 249
                 * of 302 words differ */
                /* NOLINTBEGIN(misc-redundant-expression) */
                collision_init(task, &platform_sink_shape, COLLISION_OBJECT,
                               1.0F ? 1 : 1, 1.0F ? 1 : 1);
                /* NOLINTEND(misc-redundant-expression) */
                work->collision->radius = SINK_TARGET;
            }
            work->ang.x = work->ang.z = 0;
            WORLD_MODEL(work)->model = &platform_sink_object;
            platform_add_world(task, &platform_sink_object);
            task->draw = platform_draw;
            task->destructor = platform_destroy;
            break;
        case SINK_RUN:
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_PLAYER) {
                player = PLAYER_VARS(work->collision->hit->owner->work)->player;
                rider = player_get(player);
                if (player_work[player]->flags & PLAYER_ON_LAND) {
                    force[0].speed.y = -(rider->consts.gravity * work->scl.x);
                    work->scl.x *= SINK_SPEED_KEPT;
                    work->pos.y += force[0].speed.y;
                }
            } else {
                force[0].speed.y = 0.0F;
                if (work->pos.y < work->scl.y) {
                    work->pos.y += SINK_RISE;
                } else {
                    work->scl.x = work->scl.z;
                }
            }
            platform_follow(task, SINK_NEAR);
            player_add_target(work->pos.x, work->pos.y, work->pos.z,
                              SINK_TARGET);
            force[1] = force[0];
            platform_draw(task);
            collision_register(work);
            break;
        case SINK_STILL:
            platform_follow(task, SINK_NEAR);
            player_add_target(work->pos.x, work->pos.y, work->pos.z,
                              SINK_TARGET);
            force[1] = force[0];
            platform_draw(task);
            break;
        }
    }
}

void platform_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> NOT USE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "> NOT USE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> NOT USE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> NOT USE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "> NOT USE");
    }
    WORLD_MODEL(work)->model = &platform_object;
    model_start_world(task);
}

static void platform_fly_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(FLY(work)->action, FLY(work)->frame);
    njPopMatrix(1);
}

/* draw the platform, then advance its action a frame. past the action's last
 * frame, it starts again at frame 0 */
static void platform_fly_animate(struct task *task)
{
    struct task_work *work = task->work;

    platform_fly_draw(task);
    if ((FLY(work)->frame += 1.0F) >=
        (Float)FLY(work)->action->motion->nbFrame) {
        FLY(work)->frame = 0.0F;
    }
}

/* the flying platform's step. the set editor's TYPE, scl.x, picks the axis
 * the platform flies along, as TYPE_Y says. the sign of MOVEMENT RANGE,
 * scl.y, picks the way it starts. MOVEMENT RATE, scl.z, is the platform's
 * speed less 1. once the platform starts, scl holds the place it was put
 * at */
void platform_fly_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    struct fly_travel *travel = task->extra;
    Float speed;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        FLY(work)->action = &platform_fly_action;
        platform_fly_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> NOT USE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "> NOT USE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> TYPE:");
            if (work->scl.x > TYPE_Y) {
                njPrintC(NJM_LOCATION(25, EDITOR_ROW_SCL_X), "MOVE Y: U - D");
            } else if (work->scl.x > 0.0F) {
                njPrintC(NJM_LOCATION(25, EDITOR_ROW_SCL_X), "MOVE Z: F - B");
            } else {
                njPrintC(NJM_LOCATION(25, EDITOR_ROW_SCL_X), "MOVE X: L - R");
            }
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> MOVEMENT RANGE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "> MOVEMENT RATE + 1.0F");
        }
    } else {
        switch (work->mode) {
        case FLY_INIT:
            work->mode = FLY_MOVE;
            work->ang.x = work->ang.z = 0;
            travel->range = njAbs(work->scl.y);
            travel->speed = work->scl.z + 1.0F;
            force[0].speed.x = force[0].speed.y = force[0].speed.z = 0.0F;
            if (work->scl.y == 0.0F) {
                work->mode = FLY_STILL;
            } else if (work->scl.x > TYPE_Y) {
                if (work->scl.y > 0.0F) {
                    work->submode = FLY_UP;
                    force[0].speed.y = travel->speed;
                } else {
                    work->submode = FLY_DOWN;
                    /* @hack redundant: a store of FLY_DOWN that never runs. SHC
                     * weighs it, and gives 5 R4 and 4 R5, as the image does
                     * (0x0c90a30c, 0x0c90a30a); without it, the two trade, and
                     * 6 of 504 words differ */
                    if (0) {
                        work->submode = FLY_DOWN;
                    }
                    force[0].speed.y = -travel->speed;
                }
            } else {
                /* @hack spelling: njSin's address copied to sine, which keeps
                 * it in R9 across both axes (0x0c90a410). called by name, 32
                 * of 504 words differ */
                Float (*sine)(Angle) = njSin;

                if (work->scl.x > 0.0F) {
                    speed = travel->speed;
                    ang = -work->ang.y + NJM_DEG_ANG(90);
                    if (work->scl.y > 0.0F) {
                        work->submode = FLY_Z_FORTH;
                        force[0].speed.x = speed * njCos(ang);
                        force[0].speed.z = speed * sine(ang);
                    } else {
                        work->submode = FLY_Z_BACK;
                        force[0].speed.x = -(speed * njCos(ang));
                        force[0].speed.z = -(speed * sine(ang));
                    }
                } else {
                    speed = travel->speed;
                    ang = -work->ang.y;
                    if (work->scl.y > 0.0F) {
                        /* @hack redundant: FLY_X_BACK selected by a constant
                         * condition. as FLY_X_BACK, its 1 takes R8, where the
                         * image keeps 0 (0x0c90a306), and 9 of 504 words
                         * differ */
                        /* clang-format off */
                        /* NOLINTNEXTLINE(*-redundant-expression,*-literal-conversion) */
                        work->submode = 2.0F ? FLY_X_BACK : FLY_X_BACK;
                        /* clang-format on */
                        force[0].speed.x = -(speed * njCos(ang));
                        force[0].speed.z = -(speed * sine(ang));
                    } else {
                        work->submode = FLY_X_FORTH;
                        force[0].speed.x = speed * njCos(ang);
                        force[0].speed.z = speed * sine(ang);
                    }
                }
            }
            if (travel->speed == 0.0F) {
                work->mode = FLY_STILL;
            }
            work->scl = work->pos;
            FLY(work)->action = &platform_fly_action;
            platform_add_world(task, &platform_fly_object);
            task->destructor = platform_destroy;
            task->draw = platform_fly_draw;
            break;
        case FLY_MOVE:
            work->pos.x += force[0].speed.x;
            work->pos.y += force[0].speed.y;
            work->pos.z += force[0].speed.z;
            switch (work->submode) {
            case FLY_X_FORTH:
                if (njDistanceP2P(&work->pos, &work->scl) > travel->range) {
                    work->submode = FLY_X_BACK;
                    force[0].speed.x *= -1.0F;
                    force[0].speed.z *= -1.0F;
                }
                break;
            case FLY_X_BACK:
                if (njDistanceP2P(&work->pos, &work->scl) > travel->range) {
                    work->submode = FLY_X_FORTH;
                    force[0].speed.x *= -1.0F;
                    force[0].speed.z *= -1.0F;
                }
                break;
            case FLY_Z_FORTH:
                if (njDistanceP2P(&work->pos, &work->scl) > travel->range) {
                    work->submode = FLY_Z_BACK;
                    force[0].speed.x *= -1.0F;
                    force[0].speed.z *= -1.0F;
                }
                break;
            case FLY_Z_BACK:
                if (njDistanceP2P(&work->pos, &work->scl) > travel->range) {
                    work->submode = FLY_Z_FORTH;
                    force[0].speed.x *= -1.0F;
                    force[0].speed.z *= -1.0F;
                }
                break;
            case FLY_UP:
                if (work->pos.y > work->scl.y + travel->range) {
                    work->submode = FLY_DOWN;
                    force[0].speed.y *= -1.0F;
                }
                break;
            case FLY_DOWN:
                if (work->pos.y < work->scl.y - travel->range) {
                    work->submode = FLY_UP;
                    force[0].speed.y *= -1.0F;
                }
                break;
            /* @hack redundant: case FLY_WAY_6, which does nothing, keeps the
             * image's CMP/EQ #6 at 0x0c90a56e; without it, 220 of 504 words
             * differ */
            case FLY_WAY_6:
                break;
            }
            platform_follow(task, FLY_NEAR);
            /* @hack redundant: a call of njDistanceP2P that never runs. SHC
             * weighs the distance tests' operands by it, and gives 0 R8 and
             * njSin's address R9, as the image does (0x0c90a306, 0x0c90a410);
             * without it, the two trade, and 22 of 504 words differ */
            if (0) {
                njDistanceP2P(&work->pos, &work->scl);
            }
            player_add_target(work->pos.x, work->pos.y, work->pos.z,
                              FLY_TARGET);
            force[1] = force[0];
            platform_fly_animate(task);
            break;
        case FLY_STILL:
            platform_follow(task, FLY_NEAR);
            player_add_target(work->pos.x, work->pos.y, work->pos.z,
                              FLY_TARGET);
            force[1] = force[0];
            platform_fly_animate(task);
            break;
        /* @hack redundant: case FLY_STOPPED, which does nothing, keeps the
         * image's CMP/EQ #3 at 0x0c90a324; without it, 426 of 504 words differ
         */
        case FLY_STOPPED:
            break;
        }
    }
}
