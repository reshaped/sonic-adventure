/* O ELEVATOR elevator to Twinkle Park, locked until the story opens it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/twinkle_elevator.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT twinkle_elevator_object;

#define ELEVATOR_DROP 47.0F /* the travel from top to bottom */
#define ELEVATOR_STEPS 96 /* the steps the travel takes */
#define ELEVATOR_WAIT 60 /* the steps it waits at either end, in work->tick */
/* the angle by which njCos eases the travel each step, a half turn over
 * ELEVATOR_STEPS. the division truncates to 341, so the steps come 32 short
 * of a half turn, and the elevator stops just above the bottom */
#define ELEVATOR_TURN (0x8000 / ELEVATOR_STEPS)
/* player 0 in a cylinder of this radius below the descending elevator turns
 * it back up */
#define BELOW_RADIUS 10.0F
/* a player within the floor's model's radius plus this has the world test the
 * floor */
#define WORLD_REACH 20.0F

#define ELEVATOR_WORLD_FLAGS (WORLD_SMALL | WORLD_MOVES | WORLD_SOLID)

#define SE_ELEVATOR 537

/* the elevator's vars. work->timer holds its progress down, from 0 at the
 * top to ELEVATOR_STEPS at the bottom */
struct twinkle_elevator_vars {
    /* its object in the collision world, or in the set editor the model */
    NJS_OBJECT *object;
    NJS_OBJECT *model;
    Float bottom; /* its y at the bottom, ELEVATOR_DROP below its placing */
};

#define VARS(work) ((struct twinkle_elevator_vars *)&(work)->vars)

static void twinkle_elevator_answer_request(struct task *task);
static void twinkle_elevator_draw(struct task *task);
static void twinkle_elevator_mark_near(struct task *task);
static void twinkle_elevator_wait(struct task *task);
static void twinkle_elevator_move(struct task *task);
static void twinkle_elevator_rise(struct task *task);
static void twinkle_elevator_descend(struct task *task);
static void twinkle_elevator_destroy(struct task *task);
static void twinkle_elevator_idle(struct task *task);
static void twinkle_elevator_locked(struct task *task);

/* clang-format off */
static struct collision_shape twinkle_elevator_shape[] = {
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 0.5F, 0.0F},
     26.0F, 0.5F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE once the story of player 0's character has opened the elevator */
static Bool twinkle_elevator_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_TPARK_ELEVATOR];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_TPARK_ELEVATOR];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_TPARK_ELEVATOR];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_SS_TPARK_ELEVATOR];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_SS_TPARK_ELEVATOR];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_SS_TPARK_ELEVATOR] &&
               event_flags[FLAG_BIG_CLEAR_TWINKLE_PARK];
    }
    return FALSE;
}

static void twinkle_elevator_answer_request(struct task *task)
{
    struct task_work *work = task->work;

    switch (event_flags[FLAG_ELEVATOR]) {
    case ELEVATOR_STAY:
        break;
    case ELEVATOR_TO_TOP:
        work->tick = ELEVATOR_WAIT;
        work->timer = 0;
        task->step = twinkle_elevator_wait;
        twinkle_elevator_move(task);
        break;
    case ELEVATOR_TO_BOTTOM:
        work->tick = ELEVATOR_WAIT;
        work->timer = ELEVATOR_STEPS;
        task->step = twinkle_elevator_wait;
        twinkle_elevator_move(task);
        break;
    case ELEVATOR_GO:
        if (work->timer == 0) {
            task->step = twinkle_elevator_descend;
        } else if (TASK_TIMER(work) == ELEVATOR_STEPS) {
            task->step = twinkle_elevator_rise;
        }
        break;
    }
}

static void twinkle_elevator_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(&twinkle_elevator_object);
    njPopMatrix(1);
}

/* let the world test the floor while a player is within its model's radius
 * plus WORLD_REACH */
static void twinkle_elevator_mark_near(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos,
                        VARS(work)->object->model->r + WORLD_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* wait out work->tick's steps at an end, ignoring player 0 but not
 * FLAG_ELEVATOR */
static void twinkle_elevator_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        twinkle_elevator_draw(task);
        if (--work->tick == 0) {
            task->step = twinkle_elevator_idle;
        }
        twinkle_elevator_mark_near(task);
        twinkle_elevator_answer_request(task);
    }
}

/* put the elevator, and its floor with it, at its height for work->timer:
 * the top at 0, easing down by njCos of ELEVATOR_TURN a step */
static void twinkle_elevator_move(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    work->pos.y =
        (njCos(TASK_TIMER(work) * ELEVATOR_TURN) * (ELEVATOR_DROP / 2.0F)) +
        VARS(work)->bottom + (ELEVATOR_DROP / 2.0F);
    if (!placement_editing(task)) {
        object = VARS(work)->object;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
    }
}

static void twinkle_elevator_rise(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        twinkle_elevator_draw(task);
        if (TASK_TIMER(work) > 0) {
            work->timer--;
            twinkle_elevator_move(task);
        } else {
            work->tick = ELEVATOR_WAIT;
            task->step = twinkle_elevator_wait;
        }
        twinkle_elevator_mark_near(task);
        twinkle_elevator_answer_request(task);
        sound_play_for(SE_ELEVATOR, (int)task, 1, 0, 2);
    }
}

/* move the elevator down a step, or at the bottom start its wait. once
 * player 0 is in its way, it rises from the next step on. its way is the
 * cylinder of radius BELOW_RADIUS from the bottom up to the elevator */
static void twinkle_elevator_descend(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 below;
    Float half;

    if (!placement_try_leave(task)) {
        twinkle_elevator_draw(task);
        half = (work->pos.y - VARS(work)->bottom) * 0.5F;
        below.x = work->pos.x;
        below.y = VARS(work)->bottom + half;
        below.z = work->pos.z;
        if (player_in_cylinder(&below, BELOW_RADIUS, half)) {
            task->step = twinkle_elevator_rise;
        }
        if (TASK_TIMER(work) < ELEVATOR_STEPS) {
            /* @hack spelling: timer = timer + 1, which keeps task in R13 and
             * work in R14 (MOV R4,R13 at 0x0c901b4a); written as ++, the two
             * trade and 18 of 76 words differ */
            work->timer = work->timer + 1;
            twinkle_elevator_move(task);
        } else {
            work->tick = ELEVATOR_WAIT;
            task->step = twinkle_elevator_wait;
        }
        twinkle_elevator_mark_near(task);
        twinkle_elevator_answer_request(task);
        sound_play_for(SE_ELEVATOR, (int)task, 1, 0, 2);
    }
}

static void twinkle_elevator_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}

/* at the top, descend once player 0 stands on the elevator or waits at the
 * bottom. at the bottom, rise once player 0 stands on it or leaves */
static void twinkle_elevator_idle(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;

    if (placement_editing(task)) {
        twinkle_elevator_draw(task);
    } else if (!placement_try_leave(task)) {
        twinkle_elevator_draw(task);
        pos.x = work->pos.x;
        pos.z = work->pos.z;
        if (work->timer != 0) {
            pos.y = VARS(work)->bottom + 20.0F;
            if (player_stands_on(0, task) ||
                !player_in_cylinder(&pos, 50.0F, 28.0F)) {
                task->step = twinkle_elevator_rise;
            }
        } else {
            pos.y = VARS(work)->bottom;
            if (player_stands_on(0, task) ||
                player_in_cylinder(&pos, 50.0F, 1.0F)) {
                task->step = twinkle_elevator_descend;
            }
        }
        twinkle_elevator_mark_near(task);
        twinkle_elevator_answer_request(task);
    }
}

static void twinkle_elevator_locked(struct task *task)
{
    if (!placement_try_leave(task)) {
        /* @hack redundant: an empty if on placement_editing's result, which
         * leaves the image's TST R0,R0 after the call (0x0c901d00); as a
         * call alone, 6 of 36 words differ */
        if (placement_editing(task)) {
        }
        if (twinkle_elevator_open()) {
            task->step = twinkle_elevator_idle;
            twinkle_elevator_object.model->mats[0].attrflags &=
                ~NJD_FLAG_DOUBLE_SIDE;
            twinkle_elevator_object.model->mats[1].attrflags &=
                ~NJD_FLAG_DOUBLE_SIDE;
        }
        twinkle_elevator_draw(task);
    }
}

void twinkle_elevator_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *model;
    NJS_OBJECT *object;

    if (!placement_try_leave(task)) {
        model = &twinkle_elevator_object;
        VARS(work)->model = model;
        if (!placement_editing(task)) {
            VARS(work)->object = object = world_object_alloc();
            object->evalflags = NJD_EVAL_BREAK;
            object->scl[0] = 1.0F;
            object->scl[1] = 1.0F;
            object->scl[2] = 1.0F;
            object->ang[0] = work->ang.x;
            object->ang[1] = work->ang.y;
            object->ang[2] = work->ang.z;
            object->pos[0] = work->pos.x;
            object->pos[1] = work->pos.y;
            object->pos[2] = work->pos.z;
            object->model = VARS(work)->model->model;
            object->child = VARS(work)->model->child;
            object->sibling = VARS(work)->model->sibling;
            world_add(ELEVATOR_WORLD_FLAGS, task, object);
            task->destructor = twinkle_elevator_destroy;
            collision_init(task, twinkle_elevator_shape, COLLISION_OBJECT, 1,
                           1);
        } else {
            VARS(work)->object = VARS(work)->model;
        }
        if (twinkle_elevator_open()) {
            task->step = twinkle_elevator_idle;
            event_flags[FLAG_ELEVATOR] = ELEVATOR_STAY;
            model->model->mats[0].attrflags &= ~NJD_FLAG_DOUBLE_SIDE;
            model->model->mats[1].attrflags &= ~NJD_FLAG_DOUBLE_SIDE;
        } else {
            task->step = twinkle_elevator_locked;
            model->model->mats[0].attrflags |= NJD_FLAG_DOUBLE_SIDE;
            model->model->mats[1].attrflags |= NJD_FLAG_DOUBLE_SIDE;
        }
        task->draw = twinkle_elevator_draw;
        /* @hack spelling: task->work read again, MOV.L @(32,R13),R4 at
         * 0x0c901e68; through work, 15 of 130 words differ */
        work = task->work;
        VARS(work)->bottom = work->pos.y - ELEVATOR_DROP;
        work->timer = 0;
    }
}
