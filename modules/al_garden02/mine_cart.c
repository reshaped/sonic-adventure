/* the mine cart: O MR_TOR, which takes the player out of the Mystic Ruins
 * garden */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/mystic_ruins.h"
#include "al_garden02/mine_cart.h"
#include "al_main/camera.h"
#include "al_main/card.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "player/player.h"
#include "save/card.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST mine_cart_texlist;
extern NJS_ACTION mine_cart_actions[];
extern NJS_OBJECT mine_cart_wheel_object;
extern NJS_OBJECT mine_cart_floor_objects[];

/* the frame of the cart's action, kept in the work's scl. the cart waits at
 * the action's last frame, and plays it back to 0 before it rolls.
 * work->timer counts the steps the cart has rolled */
#define FRAME(work) ((work)->scl.x)

/* the cart leads to this entrance of MYSTIC_RUINS_STATION */
#define RUINS_ENTRANCE 4

/* the steps the cart rolls before the fade starts, and before it lets
 * player 0 go */
#define ROLL_FADE 30
#define ROLL_END 300

#define ROLL_SPEED 0.5F /* the cart's speed as it starts to roll */
#define ROLL_SPEEDUP 0.034F /* the speed it gains each step */
#define SEAT_HEIGHT 5.0F /* player 0 rides this far above the cart */

/* the world tests the floor while player 0 or 1 is within its model's radius
 * plus NEAR_RANGE of the cart */
#define NEAR_RANGE 20.0F

#define FLOOR_FLAGS WORLD_MEDIUM

/* the cart's work->mode */
enum mine_cart_mode {
    CART_INIT,
    CART_WAIT,
    CART_CLOSE,
    CART_ROLL,
    CART_DONE,
    CART_END /* let player 0 go and free the floor */
};

struct mine_cart_vars {
    /* the floor in the collision world, which mine_cart_destroy frees */
    NJS_OBJECT *floor;
};

#define VARS(work) ((struct mine_cart_vars *)&(work)->vars)

static void mine_cart_draw(struct task *task);
static void floor_add(struct task *task);
static void floor_move(struct task *task);
static void mine_cart_destroy(struct task *task);
static void mine_cart_roll(struct task *task);

/* a ball in the cart: player 0's touch starts the ride */
static struct collision_shape mine_cart_shape[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 8.0F, 0.0F},
     4.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

void mine_cart_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player;

    switch (work->mode) {
    case CART_INIT:
        work->mode = CART_WAIT;
        collision_init(task, mine_cart_shape, COLLISION_OBJECT, 1, 1);
        floor_add(task);
        task->draw = mine_cart_draw;
        task->destructor = mine_cart_destroy;
        break;
    case CART_WAIT:
        FRAME(work) = mine_cart_actions[0].motion->nbFrame - 1;
        if (work->collision->flags & COLLISION_HIT) {
            work->collision->flags &= ~COLLISION_HIT;
            player = player_get_task(0);
            if (work->collision->hit->owner == player) {
                work->mode = CART_CLOSE;
                player_set_mode(0, PLAYER_MODE_STILL);
                garden_camera_still = TRUE;
                alife_save();
            }
        }
        floor_move(task);
        mine_cart_draw(task);
        collision_register(work);
        break;
    case CART_CLOSE:
        if (!card_saving) {
            FRAME(work) -= 1.0F;
            if (FRAME(work) <= 0.0F) {
                work->mode = CART_ROLL;
                FRAME(work) = 0.0F;
            }
        }
        mine_cart_draw(task);
        floor_move(task);
        break;
    case CART_ROLL:
        mine_cart_draw(task);
        mine_cart_roll(task);
        break;
    case CART_END:
        player_set_mode(0, PLAYER_MODE_RESET);
        mine_cart_destroy(task);
        work->mode = CART_DONE;
        break;
    case CART_DONE:
        break;
    }
}

/* draw the cart at its frame, and its two wheels, which turn a degree for
 * each step it has rolled */
static void mine_cart_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 front = {4.0F, 2.0F, 0.0F};
    NJS_POINT3 back = {-6.0F, 2.0F, 0.0F};
    Angle ang;

    njSetTexture(&mine_cart_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(mine_cart_actions, FRAME(work));
    njPushMatrix(NULL);
    njTranslate(NULL, front.x, front.y, front.z);
    TURN_VIA(njRotateZ, (Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE), ang);
    draw_tree(&mine_cart_wheel_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, back.x, back.y, back.z);
    TURN_VIA(njRotateZ, (Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE), ang);
    draw_tree(&mine_cart_wheel_object);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void floor_add(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *floor;

    VARS(work)->floor = floor = world_object_alloc();
    floor->evalflags = NJD_EVAL_BREAK;
    floor->scl[0] = 1.0F;
    floor->scl[1] = 1.0F;
    floor->scl[2] = 1.0F;
    floor->ang[0] = work->ang.x;
    floor->ang[1] = work->ang.y;
    floor->ang[2] = work->ang.z;
    floor->pos[0] = work->pos.x;
    floor->pos[1] = work->pos.y;
    floor->pos[2] = work->pos.z;
    floor->model = mine_cart_floor_objects[0].model;
    floor->child = NULL;
    floor->sibling = NULL;
    world_add(FLOOR_FLAGS, task, floor);
    /* @hack redundant: a return where the function ends, which makes the frame
     * with ADD #-4,R15 and stores task in it (0x0cb807a4, 0x0cb807aa);
     * without it, a push does both, the code after the push starts 2 bytes
     * early, and 48 of 50 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* move the floor to the cart's place and angles. the world tests it only
 * while player 0 or 1 is near */
static void floor_move(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *floor = VARS(work)->floor;

    floor->pos[0] = work->pos.x;
    floor->pos[1] = work->pos.y;
    floor->pos[2] = work->pos.z;
    floor->ang[0] = work->ang.x;
    floor->ang[1] = work->ang.y;
    floor->ang[2] = work->ang.z;
    if (player_in_range(&work->pos, floor->model->r + NEAR_RANGE)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void mine_cart_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->floor);
    world_object_free(VARS(work)->floor);
}

/* roll the cart the way it faces, ROLL_SPEEDUP faster each step, with player 0
 * seated in it facing backward. the fade to the Mystic Ruins starts after
 * ROLL_FADE steps */
static void mine_cart_roll(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 speed = {0.0F, 3.0F, 0.0F};
    Angle ang;

    speed.x = ROLL_SPEED + (TASK_TIMER(work) * ROLL_SPEEDUP);
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &speed, &speed);
    njPopMatrix(1);
    work->pos.x += speed.x;
    /* @hack redundant: pos.y += 0.0F, which keeps the image's FLDI0 FR3 and
     * FADD FR3,FR2 (0x0cb8098c, 0x0cb80994); without it, 53 of 110 words
     * differ */
    work->pos.y += 0.0F;
    work->pos.z += speed.z;
    player_work[0]->pos.x = work->pos.x;
    player_work[0]->pos.y = work->pos.y + SEAT_HEIGHT;
    player_work[0]->pos.z = work->pos.z;
    player_work[0]->ang.y = work->ang.y + 0x8000;
    if (TASK_TIMER(work) == ROLL_FADE) {
        stage_set_entrance(RUINS_ENTRANCE);
        stage_fade_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
    }
    if (TASK_TIMER(work)++ > ROLL_END) {
        work->mode = CART_END;
    }
}
