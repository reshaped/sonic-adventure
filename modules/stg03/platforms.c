/* the stage's platforms. O FLOAT tilts toward a player standing on it. O
 * FLYER sinks under a player and rises back. O FOOTHOLD stays still */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "stg03/platforms.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT twinkle_float_object;
extern NJS_OBJECT twinkle_flyer_object;
extern NJS_OBJECT twinkle_foothold_object;

/* O FOOTHOLD's range for placement_try_leave_by, squared */
#define FOOTHOLD_RANGE (1010.0F * 1010.0F)

#define FLOAT_WORLD_FLAGS (WORLD_MEDIUM | WORLD_TURNED | WORLD_MOVES)
#define FLYER_WORLD_FLAGS (WORLD_MEDIUM | WORLD_MOVES)
#define FOOTHOLD_WORLD_FLAGS WORLD_MEDIUM

/* how far the float's and the flyer's collision records reach from their
 * positions */
#define FLOAT_REACH 20.0F
#define FLYER_REACH 30.0F
/* the world tests each platform while a player is within its FLOAT_NEAR,
 * FLYER_NEAR or FOOTHOLD_NEAR of it */
#define FLOAT_NEAR 40.0F
#define FLYER_NEAR 40.0F
#define FOOTHOLD_NEAR 54.77F
/* the radius of the target each one offers at its position, which a falling
 * player can steer onto */
#define FLOAT_TARGET 20.0F
#define FLYER_TARGET 30.0F

/* the flyer sinks FLYER_SINK a step while a player stands on it, until it is
 * DEPTH(work) below TOP(work). while no player touches it, it rises FLYER_RISE
 * a step back to TOP(work). DEPTH(work) is the set entry's scale x plus
 * FLYER_DEPTH. TOP(work) is the height the flyer was placed at */
#define FLYER_DEPTH 3.0F
#define FLYER_SINK 0.2F
#define FLYER_RISE 0.1F
#define DEPTH(work) ((work)->scl.x)
#define TOP(work) ((work)->scl.y)

/* the float tilts about z by where a player standing on it is along x. it
 * tilts about x by where they are along z. within LEVEL_WITHIN of the middle,
 * it levels by TILT_SLOW a step. within SLOW_WITHIN, it tilts TILT_SLOW a step
 * toward the player, and within FAST_WITHIN TILT_FAST. while no player
 * touches it, it levels TILT_SETTLE a step. its tilt stays within TILT_MAX one
 * way and TILT_MIN the other */
#define LEVEL_WITHIN 5.0F
#define SLOW_WITHIN 10.0F
#define FAST_WITHIN 20.0F
#define TILT_FAST 32
#define TILT_SLOW 16
#define TILT_SETTLE NJM_DEG_ANG(1)
#define TILT_MAX NJM_DEG_ANG(10)
#define TILT_MIN NJM_DEG_ANG(350)
#define HALF_TURN NJM_DEG_ANG(180)

enum platform_mode { PLATFORM_INIT, PLATFORM_RUN };

/* clang-format off */
static struct collision_shape twinkle_float_shape[1] = {
    {SHAPE_SPHERE | SHAPE_NO_PUSH | SHAPE_IGNORE_ENEMIES |
     SHAPE_IGNORE_OBJECTS | SHAPE_IGNORE_OTHERS, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, FLOAT_REACH, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
};
static struct collision_shape twinkle_flyer_shape[1] = {
    {SHAPE_SPHERE | SHAPE_NO_PUSH | SHAPE_IGNORE_ENEMIES |
     SHAPE_IGNORE_OBJECTS | SHAPE_IGNORE_OTHERS, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, FLYER_REACH, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
};
/* clang-format on */

static void twinkle_platform_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && WORLD_MODEL(work)->world) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
    task_kill(task);
}

static void twinkle_platform_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    texture_set_stage();
    draw_tree(WORLD_MODEL(work)->model);
    njPopMatrix(1);
}

void twinkle_float_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    NJS_OBJECT *model;

    if (placement_try_leave(task)) {
        return;
    }
    model = &twinkle_float_object;
    if (edit_mode) {
        WORLD_MODEL(work)->model = model;
        twinkle_platform_draw(task);
    } else {
        switch (work->mode) {
        case PLATFORM_INIT:
            work->mode = PLATFORM_RUN;
            work->ang.x = work->ang.y = work->ang.z = 0;
            WORLD_MODEL(work)->model = model;
            task->destructor = twinkle_platform_destroy;
            task->draw = twinkle_platform_draw;
            collision_init(task, twinkle_float_shape, COLLISION_OBJECT, 1, 1);
            task->work->collision->radius = FLOAT_REACH;
            WORLD_MODEL(work)->world = world = world_object_alloc();
            world->evalflags = WORLD_MODEL(work)->model->evalflags &
                               ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = WORLD_MODEL(work)->model->model;
            world->child = WORLD_MODEL(work)->model->child;
            world->sibling = WORLD_MODEL(work)->model->sibling;
            world_add(FLOAT_WORLD_FLAGS, task, world);
            break;
        case PLATFORM_RUN:
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_PLAYER) {
                Sint8 player =
                    PLAYER_VARS(work->collision->hit->owner->work)->player;

                /* @hack redundant: a never-run store of the mode, which keeps 1
                 * in R4 at 0x0c904c74 and the player's slot of player_work in
                 * R5 at 0x0c904daa, and, with the store below, TILT_MIN in R5
                 * at 0x0c904eae and HALF_TURN in R6 at 0x0c904eb2, where
                 * otherwise each pair trades; without it, 31 of 356 words
                 * differ */
                if (0) {
                    work->mode = PLATFORM_RUN;
                }
                if (player_work[player]->flags & PLAYER_ON_LAND) {
                    Float x = player_work[player]->pos.x - work->pos.x;
                    Float z = player_work[player]->pos.z - work->pos.z;

                    /* @hack spelling: the gotos to tilted_z, which load
                     * LEVEL_WITHIN and SLOW_WITHIN into FR3 (0x0c904df6,
                     * 0x0c904e0a); as an else-if chain, they load into FR2
                     * and FR1 and 4 of 356 words differ */
                    if (x > -FAST_WITHIN && x < -SLOW_WITHIN) {
                        work->ang.z += TILT_FAST;
                        goto tilted_z;
                    }
                    /* @bug logic: a player FAST_WITHIN or more off toward -x
                     * still tilts the float TILT_SLOW, where one as far off
                     * toward +x doesn't tilt it. this test has no -FAST_WITHIN
                     * bound, so it takes the players the TILT_FAST test above
                     * leaves past it (0x0c904de6 to 0x0c904df2) */
                    if (x < -LEVEL_WITHIN) {
                        work->ang.z += TILT_SLOW;
                        goto tilted_z;
                    }
                    if (x < LEVEL_WITHIN) {
                        work->ang.z = angle_approach(work->ang.z, 0, TILT_SLOW);
                        goto tilted_z;
                    }
                    if (x < SLOW_WITHIN) {
                        work->ang.z -= TILT_SLOW;
                        goto tilted_z;
                    }
                    if (x < FAST_WITHIN) {
                        work->ang.z -= TILT_FAST;
                    }
                tilted_z:
                    /* the same along z, tilting about x */
                    /* @hack spelling: the gotos to tilted_x, which load
                     * LEVEL_WITHIN and SLOW_WITHIN into FR3 (0x0c904e48,
                     * 0x0c904e5c) and keep the -LEVEL_WITHIN test's
                     * subtraction in the BRA's slot (0x0c904e44); as an
                     * else-if chain, the bounds load into FR2 and FR1, the
                     * slot takes a NOP and 22 of 356 words differ */
                    if (z > -FAST_WITHIN && z < -SLOW_WITHIN) {
                        work->ang.x -= TILT_FAST;
                        goto tilted_x;
                    }
                    /* @bug logic: a player FAST_WITHIN or more off toward -z
                     * still tilts the float TILT_SLOW, where one as far off
                     * toward +z doesn't tilt it. this test has no -FAST_WITHIN
                     * bound, so it takes the players the TILT_FAST test above
                     * leaves past it (0x0c904e38 to 0x0c904e44) */
                    if (z < -LEVEL_WITHIN) {
                        work->ang.x -= TILT_SLOW;
                        goto tilted_x;
                    }
                    if (z < LEVEL_WITHIN) {
                        work->ang.x = angle_approach(work->ang.x, 0, TILT_SLOW);
                        goto tilted_x;
                    }
                    if (z < SLOW_WITHIN) {
                        work->ang.x += TILT_SLOW;
                        goto tilted_x;
                    }
                    if (z < FAST_WITHIN) {
                        work->ang.x += TILT_FAST;
                    }
                tilted_x:
                    work->ang.x &= 0xffff;
                    work->ang.z &= 0xffff;
                    if (work->ang.x > TILT_MAX && work->ang.x < HALF_TURN) {
                        work->ang.x = TILT_MAX;
                    } else if (work->ang.x > HALF_TURN &&
                               work->ang.x < TILT_MIN) {
                        work->ang.x = TILT_MIN;
                    }
                    /* @hack redundant: a never-run store of TILT_MAX, which,
                     * with the store above, keeps TILT_MIN in R5 at 0x0c904eae
                     * and HALF_TURN in R6 at 0x0c904eb2, which otherwise trade,
                     * with their pool words; without it, 8 of 356 words
                     * differ */
                    if (0) {
                        work->ang.z = TILT_MAX;
                    }
                    if (work->ang.z > TILT_MAX && work->ang.z < HALF_TURN) {
                        work->ang.z = TILT_MAX;
                    } else if (work->ang.z > HALF_TURN &&
                               work->ang.z < TILT_MIN) {
                        work->ang.z = TILT_MIN;
                    }
                }
            } else {
                work->ang.x = angle_approach(work->ang.x, 0, TILT_SETTLE);
                work->ang.z = angle_approach(work->ang.z, 0, TILT_SETTLE);
            }
            world = WORLD_MODEL(work)->world;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            if (player_in_range(&work->pos, FLOAT_NEAR)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            collision_register(work);
            object_clear_touch(work, NULL);
            twinkle_platform_draw(task);
            player_add_target(work->pos.x, work->pos.y, work->pos.z,
                              FLOAT_TARGET);
            break;
        }
    }
}

void twinkle_flyer_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;
    NJS_OBJECT *model;

    if (placement_try_leave(task)) {
        return;
    }
    model = &twinkle_flyer_object;
    if (edit_mode) {
        WORLD_MODEL(work)->model = model;
        twinkle_platform_draw(task);
    } else {
        switch (work->mode) {
        case PLATFORM_INIT:
            work->mode = PLATFORM_RUN;
            work->ang.x = work->ang.z = 0;
            DEPTH(work) += FLYER_DEPTH;
            TOP(work) = work->pos.y;
            WORLD_MODEL(work)->model = model;
            task->destructor = twinkle_platform_destroy;
            task->draw = twinkle_platform_draw;
            collision_init(task, twinkle_flyer_shape, COLLISION_OBJECT, 1, 1);
            task->work->collision->radius = FLYER_REACH;
            WORLD_MODEL(work)->world = world = world_object_alloc();
            world->evalflags = WORLD_MODEL(work)->model->evalflags &
                               ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
            world->ang[0] = 0;
            world->ang[1] = 0;
            world->ang[2] = 0;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = WORLD_MODEL(work)->model->model;
            world->child = WORLD_MODEL(work)->model->child;
            world->sibling = WORLD_MODEL(work)->model->sibling;
            world_add(FLYER_WORLD_FLAGS, task, world);
            break;
        case PLATFORM_RUN:
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_PLAYER) {
                Sint8 player =
                    PLAYER_VARS(work->collision->hit->owner->work)->player;

                if ((player_work[player]->flags & PLAYER_ON_LAND) &&
                    TOP(work) - work->pos.y < DEPTH(work)) {
                    force->speed.y = -FLYER_SINK;
                    work->pos.y += force->speed.y;
                }
            } else {
                if (work->pos.y < TOP(work)) {
                    work->pos.y += FLYER_RISE;
                }
                force->speed.y = 0.0F;
            }
            world = WORLD_MODEL(work)->world;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            if (player_in_range(&work->pos, FLYER_NEAR)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            collision_register(work);
            object_clear_touch(work, NULL);
            twinkle_platform_draw(task);
            player_add_target(work->pos.x, work->pos.y, work->pos.z,
                              FLYER_TARGET);
            break;
        }
    }
}

void twinkle_foothold_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    NJS_OBJECT *model;

    if (placement_try_leave_by(task, FOOTHOLD_RANGE)) {
        return;
    }
    model = &twinkle_foothold_object;
    if (edit_mode) {
        WORLD_MODEL(work)->model = model;
        twinkle_platform_draw(task);
    } else {
        switch (work->mode) {
        case PLATFORM_INIT:
            work->mode = PLATFORM_RUN;
            WORLD_MODEL(work)->model = model;
            task->destructor = twinkle_platform_destroy;
            task->draw = twinkle_platform_draw;
            WORLD_MODEL(work)->world = world = world_object_alloc();
            world->evalflags = WORLD_MODEL(work)->model->evalflags &
                               ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = WORLD_MODEL(work)->model->model;
            world->child = WORLD_MODEL(work)->model->child;
            world->sibling = WORLD_MODEL(work)->model->sibling;
            world_add(FOOTHOLD_WORLD_FLAGS, task, world);
            break;
        case PLATFORM_RUN:
            world = WORLD_MODEL(work)->world;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            if (player_in_range(&work->pos, FOOTHOLD_NEAR)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            object_clear_touch(work, NULL);
            twinkle_platform_draw(task);
            break;
        }
    }
}
