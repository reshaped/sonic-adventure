/* ShoogaiL and ShoogaiR, obstacles on the first table, which sink when
 * touched and rise again 1500 steps later */
#include <ninja.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "object/common.h"
#include "stg09/casino.h"
#include "stg09/obstacle.h"
#include "stg09/pinball.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL obstacle_model;

/* in work->flags while a touch lasts, so that each counts once */
#define OBSTACLE_TOUCHING 0x8000

/* the obstacle's height up and down, and how far it rises or sinks each
 * step */
#define OBSTACLE_UP 8.0F
#define OBSTACLE_DOWN (-9.0F)
#define OBSTACLE_RISE 2.0F
#define OBSTACLE_SINK 1.0F
/* the steps an obstacle's bit stays set after a touch, counted down in
 * TASK_TIMER */
#define OBSTACLE_DOWN_STEPS 1500

/* each touch gives OBSTACLE_RINGS. pinball_countdown2_place is where the first
 * table's third countdown stands while it runs. at 0, a touch of ShoogaiL moves
 * it on and gives LEFT_PLACE_RINGS more. at 2, a touch of ShoogaiR moves it on
 * and gives RIGHT_PLACE_RINGS more */
#define OBSTACLE_RINGS 1
#define LEFT_PLACE_RINGS 10
#define RIGHT_PLACE_RINGS 40

/* ShoogaiL's shape, ShoogaiR's, and the shape of either's child, which rises
 * and sinks with its parent */
static struct collision_shape obstacle_shapes[3] = {
    {SHAPE_CYLINDER,
     SHAPE_ATTACK,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {18.0F, 5.0F, -7.5F},
     5.0F,
     10.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     SHAPE_ATTACK,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-20.0F, 5.0F, -7.0F},
     5.0F,
     10.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     SHAPE_ATTACK,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -3.0F, -1.0F},
     12.0F,
     8.0F,
     3.0F,
     {0},
     0,
     NULL},
};

static void obstacle_destroy(struct task *task)
{
}

static void obstacle_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&obstacle_model);
    njPopMatrix(1);
}

static void obstacle_child_run(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.y = task->parent->work->pos.y;
    /* @hack spelling: the record reached through task->work for each call,
     * which SHC loads again into R4 (MOV.L @(32,R14),R4 at 0x0c906c5c and
     * 0x0c906c64). through work, 17 of obstacle_child_run's 18 words
     * differ */
    collision_register(task->work);
    object_clear_touch(task->work, NULL);
}

static void obstacle_left_run(struct task *task)
{
    struct task_work *work = task->work;

    task_step_children(task);
    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & OBSTACLE_TOUCHING)) {
            if ((casino_flags & CASINO_COUNTDOWN_ACT2_2) &&
                pinball_countdown2_place == 0) {
                pinball_countdown2_place++;
                pinball_give_rings(LEFT_PLACE_RINGS);
            }
            if (!(casino_flags & CASINO_SHOOGAI_L_DOWN)) {
                casino_flags |= CASINO_SHOOGAI_L_DOWN;
                work->timer = OBSTACLE_DOWN_STEPS;
            }
            pinball_give_rings(OBSTACLE_RINGS);
            work->flags |= OBSTACLE_TOUCHING;
        }
    } else {
        work->flags &= ~OBSTACLE_TOUCHING;
    }
    if (!(casino_flags & CASINO_SHOOGAI_L_DOWN)) {
        if (work->pos.y != OBSTACLE_UP) {
            work->pos.y += OBSTACLE_RISE;
            if (work->pos.y > OBSTACLE_UP) {
                work->pos.y = OBSTACLE_UP;
            }
        }
    } else {
        if (work->pos.y != OBSTACLE_DOWN) {
            work->pos.y -= OBSTACLE_SINK;
            if (work->pos.y < OBSTACLE_DOWN) {
                work->pos.y = OBSTACLE_DOWN;
            }
        }
        if (--TASK_TIMER(work) == 0) {
            casino_flags &= ~CASINO_SHOOGAI_L_DOWN;
        }
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    obstacle_draw(task);
}

static void obstacle_right_run(struct task *task)
{
    struct task_work *work = task->work;

    task_step_children(task);
    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & OBSTACLE_TOUCHING)) {
            if ((casino_flags & CASINO_COUNTDOWN_ACT2_2) &&
                pinball_countdown2_place == 2) {
                pinball_countdown2_place++;
                pinball_give_rings(RIGHT_PLACE_RINGS);
            }
            if (!(casino_flags & CASINO_SHOOGAI_R_DOWN)) {
                casino_flags |= CASINO_SHOOGAI_R_DOWN;
                work->timer = OBSTACLE_DOWN_STEPS;
            }
            pinball_give_rings(OBSTACLE_RINGS);
            work->flags |= OBSTACLE_TOUCHING;
        }
    } else {
        work->flags &= ~OBSTACLE_TOUCHING;
    }
    if (!(casino_flags & CASINO_SHOOGAI_R_DOWN)) {
        if (work->pos.y != OBSTACLE_UP) {
            work->pos.y += OBSTACLE_RISE;
            if (work->pos.y > OBSTACLE_UP) {
                work->pos.y = OBSTACLE_UP;
            }
        }
    } else {
        if (work->pos.y != OBSTACLE_DOWN) {
            work->pos.y -= OBSTACLE_SINK;
            if (work->pos.y < OBSTACLE_DOWN) {
                work->pos.y = OBSTACLE_DOWN;
            }
        }
        if (--TASK_TIMER(work) == 0) {
            casino_flags &= ~CASINO_SHOOGAI_R_DOWN;
        }
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    obstacle_draw(task);
}

static void obstacle_child_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;

    work->pos.x = parent->pos.x;
    work->pos.z = parent->pos.z;
    work->ang.y = parent->ang.y;
    collision_init(task, &obstacle_shapes[2], COLLISION_OBJECT, 1, 1);
}

/* side is 0 for ShoogaiL, 1 for ShoogaiR */
static void obstacle_init(struct task *task, int side)
{
    collision_init(task, &obstacle_shapes[side], COLLISION_OBJECT, 1, 1);
    obstacle_child_init(task_create_child(TASK_WORK, obstacle_child_run, task));
    if (side == 0) {
        task->step = obstacle_left_run;
    } else {
        task->step = obstacle_right_run;
    }
    task->draw = obstacle_draw;
    task->destructor = obstacle_destroy;
}

void obstacle_left_step(struct task *task)
{
    obstacle_init(task, 0);
}

void obstacle_right_step(struct task *task)
{
    obstacle_init(task, 1);
}
