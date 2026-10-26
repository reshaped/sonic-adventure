/* O PIRATES two pirate ships that loop around on their arms */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg03/pirates.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the ride's stand. its child is the arm, which turns, and the arm's child is
 * the ship at its end. the arm's sibling turns with the arm */
extern NJS_OBJECT pirates_ship_object;

/* the ride's range for placement_try_leave_by, squared */
#define PIRATES_RANGE (1510.0F * 1510.0F)

/* the sound of a turning ship. each step refreshes it, SHIP_SOUND_HEIGHT above
 * the ride */
#define SHIP_SOUND_HEIGHT 350.0F

/* the ship's distance from the arm's pivot */
#define SHIP_ARM 115.0F
/* the world tests a ship while a player is within SHIP_NEAR of the point
 * SHIP_TOP above the ride */
#define SHIP_TOP 335.0F
#define SHIP_NEAR 600.0F

/* the most a ship turns a step. the least is 1 */
#define TURN_FASTEST 256
#define QUARTER_TURN NJM_DEG_ANG(90)
#define HALF_TURN NJM_DEG_ANG(180)

/* the tick counts up to LAMPS_BLINK + 1, then starts again at 0. the lamps
 * blink every 62 steps, and are dark while the tick is past LAMPS_LIT */
#define LAMPS_BLINK 60
#define LAMPS_LIT 30
/* the texture of the lamps' materials, dark and lit */
#define LAMPS_DARK_TEXTURE 28
#define LAMPS_LIT_TEXTURE 95

#define SHIP_WORLD_FLAGS (WORLD_LARGE | WORLD_TURNED | WORLD_MOVES)

struct ship_vars {
    NJS_OBJECT *world;
    NJS_OBJECT *object;
    Sint32 speed; /* the ship's turn a step */
};

#define VARS(work) ((struct ship_vars *)&(work)->vars)
/* the ship's turn about the arm's pivot, 0 at the top */
#define SHIP_TURN(work) ((work)->ang.z)

/* modes of the ride and of each ship */
enum pirates_mode {
    PIRATES_INIT,
    PIRATES_RUN,
    PIRATES_IDLE /* this file never sets it */
};

static void pirates_ship_step(struct task *task);

/* the two ships. the second faces the other way, and starts half a turn
 * around from the first, at the bottom. the first starts at the slowest turn,
 * and the second at the fastest */
static struct task_spec pirates_ships[3] = {
    {pirates_ship_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&pirates_ship_object},
    {pirates_ship_step,
     TASK_WORK | TASK_FORCE,
     1,
     {0, HALF_TURN, HALF_TURN},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&pirates_ship_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* on a frame's first step, draw the stand, the arm and its sibling, and the
 * ship. the arm and its sibling turn about z by SHIP_TURN. the ship turns back
 * as far, so it stays level */
static void pirates_ship_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle turn = SHIP_TURN(work);
    Angle ang;
    NJS_OBJECT *object;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        object = VARS(work)->object;
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(object->model);
        njPushMatrix(NULL);
        object = object->child;
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        TURN(njRotateZ, turn);
        draw_model(object->model);
        njPushMatrix(NULL);
        object = object->child;
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        TURN_VIA(njRotateZ, -turn, ang);
        draw_model(object->model);
        njPopMatrix(1);
        njPopMatrix(1);
        njPushMatrix(NULL);
        object = VARS(work)->object->child->sibling;
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        TURN(njRotateZ, turn);
        draw_model(object->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

/* set the texture of the lamps' materials. both ships and their world objects
 * share pirates_ship_object's models. the texture holds for all four until
 * the next call */
static void pirates_ship_set_lamps(struct task *task, Uint32 texture)
{
    struct task_work *work = task->work;

    VARS(work)->world->model->mats[4].attr_texId = texture;
    VARS(work)->world->child->model->mats[0].attr_texId = texture;
    VARS(work)->world->child->child->model->mats[2].attr_texId = texture;
    VARS(work)->world->child->child->model->mats[3].attr_texId = texture;
    VARS(work)->world->child->child->model->mats[8].attr_texId = texture;
    VARS(work)->world->child->child->model->mats[12].attr_texId = texture;
    VARS(work)->world->child->sibling->model->mats[0].attr_texId = texture;
}

static void pirates_ship_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        if (VARS(work)->world != NULL) {
            world_remove(task, VARS(work)->world);
            world_object_free(VARS(work)->world);
        }
    }
    task_kill(task);
}

static void pirates_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* the ship speeds up by 1 a step on its way down. it slows by 1 a step on its
 * way up. task->force's speed is how far the ship's end moves over the next
 * step. it moves the players standing on the ship. on a step whose tick is
 * past LAMPS_LIT, the lamps are dark for this ship's draw alone */
static void pirates_ship_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *ship = &pirates_ship_object;
    struct task_force *force;
    NJS_OBJECT *world;
    Angle turn;
    Angle next_turn;
    Angle heading;
    Float reach;
    Float next_reach;

    if (edit_mode) {
        if (VARS(work)->object == NULL) {
            VARS(work)->object = ship;
        }
        pirates_ship_draw(task);
    } else {
        switch (work->mode) {
        case PIRATES_INIT:
            work->mode = PIRATES_RUN;
            switch (work->flags) {
            case 0:
                VARS(work)->speed = 1;
                break;
            case 1:
                VARS(work)->speed = TURN_FASTEST;
            }
            task->destructor = pirates_ship_destroy;
            task->draw = pirates_ship_draw;
            VARS(work)->object = ship;
            world = world_object_alloc();
            VARS(work)->world = world;
            world->evalflags = VARS(work)->object->evalflags &
                               ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
            world->ang[0] = 0;
            world->ang[1] = work->ang.y;
            world->ang[2] = 0;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = VARS(work)->object->model;
            world->child = VARS(work)->object->child;
            world->sibling = VARS(work)->object->sibling;
            world_add(SHIP_WORLD_FLAGS, task, world);
            break;
        case PIRATES_RUN:
            force = task->force;
            SHIP_TURN(work) &= 0xffff;
            if (SHIP_TURN(work) < HALF_TURN) {
                VARS(work)->speed--;
                if (VARS(work)->speed < 1) {
                    VARS(work)->speed = 1;
                }
            } else {
                if (++VARS(work)->speed > TURN_FASTEST) {
                    VARS(work)->speed = TURN_FASTEST;
                }
            }
            SHIP_TURN(work) -= VARS(work)->speed;
            turn = SHIP_TURN(work);
            next_turn = turn - VARS(work)->speed;
            reach = njSin(turn) * SHIP_ARM;
            next_reach = njSin(next_turn) * SHIP_ARM;
            {
                NJS_POINT3 now;
                NJS_POINT3 next;

                heading = work->ang.y;
                now.x = -(reach * njCos(heading));
                now.y = njCos(turn) * SHIP_ARM;
                now.z = reach * njSin(heading);
                /* @hack redundant: never-run stores to next.x and the tick,
                 * which give the zero R11 and &now R9 (0x0c9061e0, 0x0c906318),
                 * which otherwise trade; without it, 14 of 348 words differ */
                if (0) {
                    next.x = heading;
                    work->tick = 0;
                }
                next.x = -(next_reach * njCos(heading));
                next.y = njCos(next_turn) * SHIP_ARM;
                next.z = next_reach * njSin(heading);
                force->speed.x = next.x - now.x;
                force->speed.y = next.y - now.y;
                force->speed.z = next.z - now.z;
            }
            /* the world object follows the ship only while player 0 is on the
             * ship's side of the ride. that is, player 0's direction to the
             * ride is more than QUARTER_TURN off the ship's heading. the ships
             * face opposite ways, so at most one world object follows its ship
             * at a time. whether the world tests the ship changes only then
             * too */
            if (angle_distance(
                    work->ang.y,
                    (Angle)(atan2f(work->pos.x - player_work[0]->pos.x,
                                   work->pos.z - player_work[0]->pos.z) *
                            ANGLE_PER_RADIAN)) > QUARTER_TURN) {
                NJS_POINT3 top;

                world = VARS(work)->world;
                world->ang[0] = 0;
                world->ang[1] = work->ang.y;
                world->ang[2] = 0;
                world->pos[0] = work->pos.x;
                world->pos[1] = work->pos.y;
                world->pos[2] = work->pos.z;
                world->child->ang[2] = turn;
                world->child->sibling->ang[2] = turn;
                world->child->child->ang[2] = -turn;
                top = work->pos;
                top.y += SHIP_TOP;
                if (player_in_range(&top, SHIP_NEAR)) {
                    work->flags |= WORLD_TESTED;
                } else {
                    work->flags &= ~WORLD_TESTED;
                }
            }
            if (work->tick++ > LAMPS_BLINK) {
                work->tick = 0;
            }
            if (work->tick > LAMPS_LIT) {
                pirates_ship_set_lamps(task, LAMPS_DARK_TEXTURE);
                pirates_ship_draw(task);
                pirates_ship_set_lamps(task, LAMPS_LIT_TEXTURE);
            } else {
                pirates_ship_draw(task);
            }
            /* @bug logic: SE_MACHINE_RUN plays at z 0, not at the ship. its z
             * is work->scl.z, read at 0x0c9064ca, where its x and y come from
             * work->pos. a ship's work starts zeroed, and this file never sets
             * its scale, so the sound plays at the ride's x and
             * SHIP_SOUND_HEIGHT above it, but at z 0 */
            sound_play_at_for(SE_MACHINE_RUN, (int)work, 1, 0, work->pos.x,
                              work->pos.y + SHIP_SOUND_HEIGHT, work->scl.z, 2);
            break;
        }
    }
}

/* create the two ships, then step them. while the set editor holds the ride,
 * draw its model twice, a half turn apart, instead */
void pirates_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, PIRATES_RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&pirates_ship_object);
        njRotateY(NULL, HALF_TURN);
        draw_tree(&pirates_ship_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case PIRATES_INIT:
            work->mode = PIRATES_RUN;
            task->destructor = pirates_destroy;
            task_create_children(pirates_ships, task);
            break;
        case PIRATES_RUN:
            task_step_children(task);
            break;
        case PIRATES_IDLE:
            break;
        }
    }
}
