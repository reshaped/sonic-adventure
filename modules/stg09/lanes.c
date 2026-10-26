/* the tables' Tuuka, a row of three lanes with lamps, and RightLane, one
 * lane */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "object/common.h"
#include "stage/stage.h"
#include "stg09/casino.h"
#include "stg09/lanes.h"
#include "stg09/pinball.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL lane_lamp_model;
extern NJS_MODEL lane_lamp_lit_model;

/* which object a task is, in work->mode */
#define LANES_TUUKA 0
#define LANES_RIGHT_LANE 1

/* in Tuuka's work->flags, set while each lane's lamp is lit */
#define LANE_0_LIT 0x100
#define LANE_1_LIT 0x200
#define LANE_2_LIT 0x400
#define LANES_ALL_LIT (LANE_0_LIT | LANE_1_LIT | LANE_2_LIT)
/* in a lane's or RightLane's work->flags while something touches it, so
 * that a touch counts once */
#define LANE_TOUCHING 0x8000

/* the steps Tuuka's lamps flash once all three are lit, counted down in its
 * TASK_TIMER(work). the lamps are lit while the count has FLASH_BIT set */
#define LANES_FLASH 80
#define FLASH_BIT 8

/* the offset of lane 2 from lane 1, the middle lane. lane 0 lies the same
 * distance the other way */
#define LANE_X 20.5F
#define LANE_Z 1.5F

#define LANE(work) ((work)->tick) /* a lane's number, 0 to 2 */
/* TOUCHES(work) counts RightLane's touches up to RIGHT_LANE_HITS, and starts
 * again from the touch that reaches it */
#define TOUCHES(work) ((work)->tick)
#define RIGHT_LANE_HITS 5

/* a lane's touch gives LANE_RINGS. once all three lanes are lit, Tuuka gives
 * ALL_LIT_RINGS, or ALL_LIT_BONUS while the first table's fourth countdown
 * runs. RightLane's touch gives RIGHT_LANE_RINGS, or RIGHT_LANE_HIT_RINGS for
 * the touch that reaches RIGHT_LANE_HITS */
#define LANE_RINGS 1
#define ALL_LIT_RINGS 15
#define ALL_LIT_BONUS 30
#define RIGHT_LANE_RINGS 3
#define RIGHT_LANE_HIT_RINGS 13
/* pinball_countdown2_place is where the first table's third countdown stands.
 * while it stands at 1, a touch of RightLane moves it on and gives PLACE_RINGS.
 * while the second table's first countdown runs, each touch of RightLane adds 1
 * to pinball_right_lane_touches. it then gives COUNT_RINGS times that count */
#define PLACE_RINGS 20
#define COUNT_RINGS 5

static struct collision_shape lane_shapes[3] = {
    {SHAPE_RECTANGLE,
     SHAPE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {-LANE_X, 0.0F, -LANE_Z},
     8.0F,
     1.0F,
     8.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     SHAPE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     8.0F,
     1.0F,
     8.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     SHAPE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {LANE_X, 0.0F, LANE_Z},
     8.0F,
     1.0F,
     8.0F,
     {0},
     0,
     NULL},
};
static struct collision_shape right_lane_shape = {
    SHAPE_RECTANGLE,
    SHAPE_ATTACK,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    8.0F,
    8.0F,
    0.1F,
    {0},
    0,
    NULL,
};

/* Tuuka's three lanes are its child tasks, which go with it */
static void lanes_destroy(struct task *task)
{
    if (task->work->mode == LANES_TUUKA) {
        task_destroy_children(task);
    }
}

static void lanes_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njPushMatrix(NULL);
    njTranslate(NULL, -LANE_X, 0.0F, -LANE_Z);
    if (!(work->flags & LANE_0_LIT)) {
        draw_model(&lane_lamp_model);
    } else {
        draw_model(&lane_lamp_lit_model);
    }
    njPopMatrix(1);
    if (!(work->flags & LANE_1_LIT)) {
        draw_model(&lane_lamp_model);
    } else {
        draw_model(&lane_lamp_lit_model);
    }
    njTranslate(NULL, LANE_X, 0.0F, LANE_Z);
    if (!(work->flags & LANE_2_LIT)) {
        draw_model(&lane_lamp_model);
    } else {
        draw_model(&lane_lamp_lit_model);
    }
    njPopMatrix(1);
}

static void lane_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *lanes = task->parent->work;

    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & LANE_TOUCHING)) {
            switch (LANE(work)) {
            case 0:
                lanes->flags |= LANE_0_LIT;
                if ((casino_flags & CASINO_COUNTDOWN_ACT2_2) &&
                    pinball_countdown2_place >= 3) {
                    pinball_countdown2_place++;
                    casino_flags |= CASINO_HOUDAI_L_FIRES;
                    casino_flags |= CASINO_HOUDAI_R_FIRES;
                    /* XOR, which gives 80 more than pinball_countdown2_place -
                     * 2 while that is under 16 */
                    pinball_give_rings((pinball_countdown2_place - 2) ^ 80);
                }
                break;
            case 1:
                lanes->flags |= LANE_1_LIT;
                break;
            case 2:
                lanes->flags |= LANE_2_LIT;
                break;
            }
            pinball_give_rings(LANE_RINGS);
            work->flags |= LANE_TOUCHING;
        }
    } else {
        work->flags &= ~LANE_TOUCHING;
    }
    collision_register(work);
    object_clear_touch(work, NULL);
}

static void lanes_run(struct task *task)
{
    struct task_work *work = task->work;

    task_step_children(task);
    if (TASK_TIMER(work)) {
        --TASK_TIMER(work);
        if (!(TASK_TIMER(work) & FLASH_BIT)) {
            work->flags &= ~LANES_ALL_LIT;
        } else {
            work->flags |= LANES_ALL_LIT;
        }
    } else if ((work->flags & LANES_ALL_LIT) == LANES_ALL_LIT) {
        TASK_TIMER(work) = LANES_FLASH;
        if (!(casino_flags & CASINO_COUNTDOWN_ACT2_3)) {
            pinball_give_rings(ALL_LIT_RINGS);
        } else {
            casino_flags |= CASINO_HOUDAI_L_FIRES;
            casino_flags |= CASINO_HOUDAI_R_FIRES;
            pinball_give_rings(ALL_LIT_BONUS);
        }
        if ((Uint8)stage_get_stage_act() == 3) {
            casino_flags |= CASINO_STOPPER_R_UP;
        }
    } else if (per_info[0]->press & PDD_DGT_TL) {
        Sint16 lit = work->flags;

        work->flags &= ~LANES_ALL_LIT;
        if (lit & LANE_0_LIT) {
            work->flags |= LANE_2_LIT;
        }
        if (lit & LANE_1_LIT) {
            work->flags |= LANE_0_LIT;
        }
        if (lit & LANE_2_LIT) {
            work->flags |= LANE_1_LIT;
        }
    } else if ((per_info[0]->press & PDD_DGT_TR) ||
               (per_info[0]->press & PDD_DGT_TA)) {
        Sint16 lit = work->flags;

        work->flags &= ~LANES_ALL_LIT;
        if (lit & LANE_0_LIT) {
            work->flags |= LANE_1_LIT;
        }
        if (lit & LANE_1_LIT) {
            work->flags |= LANE_2_LIT;
        }
        if (lit & LANE_2_LIT) {
            work->flags |= LANE_0_LIT;
        }
    }
    lanes_draw(task);
}

static void right_lane_run(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & LANE_TOUCHING)) {
            if ((casino_flags & CASINO_COUNTDOWN_ACT2_2) &&
                pinball_countdown2_place == 1) {
                pinball_countdown2_place++;
                pinball_give_rings(PLACE_RINGS);
            }
            if (casino_flags & CASINO_COUNTDOWN_ACT3_0) {
                pinball_right_lane_touches++;
                pinball_give_rings(pinball_right_lane_touches * COUNT_RINGS);
            }
            if (++TOUCHES(work) % RIGHT_LANE_HITS == 0) {
                pinball_give_rings(RIGHT_LANE_HIT_RINGS);
                TOUCHES(work) = 0;
            } else {
                pinball_give_rings(RIGHT_LANE_RINGS);
            }
            work->flags |= LANE_TOUCHING;
        }
    } else {
        work->flags &= ~LANE_TOUCHING;
    }
    collision_register(work);
    object_clear_touch(work, NULL);
}

static void lane_init(struct task *task, int lane)
{
    lane_shapes[lane].flags &= ~SHAPE_OFF;
    collision_init(task, &lane_shapes[lane], COLLISION_OBJECT, 1, 1);
    LANE(task->work) = lane;
}

void lanes_step(struct task *task)
{
    struct task_work *work = task->work;

    lane_init(task_create_child(TASK_WORK, lane_run, task), 0);
    lane_init(task_create_child(TASK_WORK, lane_run, task), 1);
    lane_init(task_create_child(TASK_WORK, lane_run, task), 2);
    work->flags &= ~LANES_ALL_LIT;
    work->mode = LANES_TUUKA;
    task->step = lanes_run;
    task->draw = lanes_draw;
    task->destructor = lanes_destroy;
}

void right_lane_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = LANES_RIGHT_LANE;
    TOUCHES(work) = 0;
    collision_init(task, &right_lane_shape, COLLISION_OBJECT, 1, 1);
    task->step = right_lane_run;
    task->destructor = lanes_destroy;
}
