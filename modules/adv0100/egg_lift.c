/* O EGGLIFT lift that takes player 0 down from the deck into the Egg Carrier */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/egg_lift.h"
#include "adv0100/gate.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the lift's object as an array, which has SHC pool its address
 * and read its model 4 bytes past it (MOV.L @(4,R4),R4 at 0x0c92a6e0); as a
 * plain object, SHC pools the model's address, and 6 of 33 words differ */
extern NJS_OBJECT outside_egg_lift_object[];
extern NJS_OBJECT outside_egg_lift_solid_object;

/* player 0 boards the waiting lift after standing on it more than this many
 * steps */
#define BOARD_STEPS 30
/* how far the travel moves each step, as a fraction of the way between the
 * lift's ends */
#define LIFT_SPEED 0.01F
#define LIFT_TOP 0.99F /* the travel at the top */

#define BOB_TURN 0x400 /* the bob's phase gain each step */
#define BOB_SPEED 0.05F /* the change in the bob's amplitude each step */

/* the surface's flags in the collision world */
#define SOLID_FLAGS (WORLD_MEDIUM | WORLD_MOVES | WORLD_SOLID)
/* player 0's distance within which the collision world tests the surface */
#define TEST_RANGE 40.0F

/* the squared distance across within which player 0 counts as at the lift's
 * middle */
#define MIDDLE_REACH_SQUARED 10.0F

/* both players' pads, as the image passes them: pad_disable_player takes any
 * number but 0 and 1 for both, and PAD_BOTH_PLAYERS is 0xff */
#define BOTH_PADS 2

/* where the lift takes player 0: the Egg Carrier inside's act, and the
 * entrance in it */
#define INSIDE_ACT 1
#define INSIDE_ENTRANCE 8

/* in work->flags: player 0 must step off the lift before it goes down */
#define LIFT_STEP_OFF 0x400

struct egg_lift_vars {
    NJS_OBJECT *solid; /* the surface's object in the collision world */
    NJS_OBJECT *object; /* the model solid copies */
    Float travel; /* from 0 at the bottom to LIFT_TOP */
};

#define VARS(work) ((struct egg_lift_vars *)&(work)->vars)
#define PHASE(work) ((work)->ang.x) /* the bob's phase */
#define BOB(work) ((work)->scl.x) /* the bob's amplitude, from 0 to 1 */

enum egg_lift_mode {
    LIFT_GONE = -1, /* gone down: the act changes, and the step kills the
                     * placement */
    LIFT_HIGH = 0, /* at the top, bobbing */
    LIFT_LOW = 1, /* at the bottom */
    LIFT_SINKING = 2,
    LIFT_RISING = 3,
    LIFT_WAIT = 4, /* at the top, waiting for player 0 */
    /* commands the gates open, disables both pads and starts rising */
    LIFT_CALLED = 5,
    LIFT_RISING_READY = 6, /* rising, to wait at the top */
    LIFT_TAKING = 7 /* taking player 0 down */
};

static void outside_egg_lift_destroy(struct task *task);
static void outside_egg_lift_draw(struct task *task);
static void outside_egg_lift_step(struct task *task);
static void outside_egg_lift_obey(struct task_work *work,
                                  struct task_force *force);
static void outside_egg_lift_solid_init(struct task *task,
                                        struct task_work *work);
static Bool outside_egg_lift_take_aboard(struct task *task,
                                         struct task_work *work);
static Bool outside_egg_lift_reach_middle(struct task_work *work);
static void outside_egg_lift_move(struct task_work *work,
                                  struct task_force *force);
static Bool outside_egg_lift_shut_gates_or_opened(void);

static struct collision_shape outside_egg_lift_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_UNTURNED,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     10.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the surface's offsets from the lift's position at travel 0 and at travel 1,
 * 40 below and 10 above, which njLinear blends by the travel */
static Float outside_egg_lift_ends[6] = {0.0F, -40.0F, 0.0F, 0.0F, 10.0F, 0.0F};

void egg_carrier_outside_egg_lift_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;

    if (!placement_editing(task)) {
        collision_init(task, outside_egg_lift_shapes, COLLISION_OBJECT, 1, 1);
    }
    outside_egg_lift_solid_init(task, work);
    VARS(work)->solid->pos[0] = work->pos.x;
    VARS(work)->solid->pos[2] = work->pos.z;
    VARS(work)->solid->ang[1] = work->ang.y;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    task->destructor = outside_egg_lift_destroy;
    task->draw = outside_egg_lift_draw;
    task->step = outside_egg_lift_step;
}

static void outside_egg_lift_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

/* drawn where the surface is, not at work->pos */
static void outside_egg_lift_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)VARS(work)->solid->pos);
    TURN_VIA(njRotateY, VARS(work)->solid->ang[1], ang);
    draw_model(outside_egg_lift_object->model);
    njPopMatrix(1);
}

static void outside_egg_lift_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;

    if (placement_try_leave(task)) {
        return;
    }
    if (!outside_egg_lift_shut_gates_or_opened()) {
        return;
    }
    outside_egg_lift_obey(work, force);
    switch (work->mode) {
    case LIFT_WAIT:
        PHASE(work) += BOB_TURN;
        if (outside_egg_lift_take_aboard(task, work) &&
            outside_egg_lift_reach_middle(work)) {
            work->mode = LIFT_TAKING;
            /* @hack redundant: a break of the if's own, as LIFT_TAKING has,
             * which lets this case's store of its mode share LIFT_TAKING's (BRA
             * at 0x0c92a7e2 to 0x0c92a884); without it, 9 of 237 words differ
             */
            break;
        }
        break;
    case LIFT_CALLED:
        /* @hack redundant: a goto to the next statement, which keeps
         * LIFT_RISING_READY in R3 and the phase's sum in R2 (0x0c92a820 to
         * 0x0c92a82e); without it, the two trade and 7 of 237 words differ */
        goto called;
    called:
        event_flags[FLAG_GATE] = GATE_OPEN;
        PHASE(work) += BOB_TURN;
        work->mode = LIFT_RISING_READY;
        pad_disable_player(BOTH_PADS);
        break;
    case LIFT_RISING_READY:
        VARS(work)->travel += LIFT_SPEED;
        if (VARS(work)->travel >= LIFT_TOP) {
            VARS(work)->travel = LIFT_TOP;
            work->mode = LIFT_WAIT;
            work->flags |= LIFT_STEP_OFF;
            pad_enable_player(BOTH_PADS);
        }
        break;
    case LIFT_TAKING:
        /* @hack spelling: -LIFT_SPEED added, which has SHC hold -0.01 in FR6,
         * beside LIFT_SPEED in FR5 and LIFT_TOP in FR4, through the switch
         * (0x0c92a76e to 0x0c92a77a); with -= here, 105 of 237 words
         * differ */
        VARS(work)->travel += -LIFT_SPEED;
        if (VARS(work)->travel <= 0.0F) {
            event_flags[FLAG_GATE] = GATE_NO_COMMAND;
            VARS(work)->travel = 0.0F;
            pad_enable_player(BOTH_PADS);
            stage_set_entrance(INSIDE_ENTRANCE);
            stage_fade_to(STAGE_EGG_CARRIER_INSIDE, INSIDE_ACT);
            work->mode = LIFT_GONE;
            /* @hack redundant: the if's own break that lets LIFT_WAIT's store
             * of its mode share this case's (0x0c92a884); without it, 5 of 237
             * words differ */
            break;
        }
        break;
    case LIFT_HIGH:
        /* @hack redundant: a goto to the next statement, which sums the phase
         * in R1 (0x0c92a888 to 0x0c92a88e); without it, in R3, and 3 of 237
         * words differ */
        goto high;
    high:
        PHASE(work) += BOB_TURN;
        break;
    case LIFT_RISING:
        VARS(work)->travel += LIFT_SPEED;
        if (VARS(work)->travel >= LIFT_TOP) {
            VARS(work)->travel = LIFT_TOP;
            work->mode = LIFT_HIGH;
        }
        break;
    case LIFT_SINKING:
        /* @hack spelling: -LIFT_SPEED added, as in LIFT_TAKING, from FR6
         * (0x0c92a76e); with -= here, 134 of 237 words differ */
        VARS(work)->travel += -LIFT_SPEED;
        if (VARS(work)->travel <= 0.0F) {
            VARS(work)->travel = 0.0F;
            work->mode = LIFT_LOW;
        }
        break;
    case LIFT_GONE:
        placement_kill(task);
        break;
    case LIFT_LOW:
        break;
    }
    /* the bob grows to its full amplitude while the lift is at the top, and
     * dies away while it is lower */
    if (VARS(work)->travel > LIFT_TOP - LIFT_SPEED) {
        BOB(work) += BOB_SPEED;
        if (BOB(work) > 1.0F) {
            BOB(work) = 1.0F;
        }
    } else {
        /* @hack spelling: -BOB_SPEED added, a constant of its own, which SHC
         * loads through R1 and FPUL (0x0c92a920) as the growth loads 0.05
         * (0x0c92a8cc); with -=, SHC loads 0.05 once into FR4 for both, the
         * function runs 8 bytes short and 73 of 237 words differ */
        BOB(work) += -BOB_SPEED;
        if (BOB(work) < 0.0F) {
            BOB(work) = 0.0F;
        }
    }
    work->flags &= ~WORLD_TESTED;
    if (njDistanceP2P(&player_work[0]->pos, &work->pos) < TEST_RANGE) {
        work->flags |= WORLD_TESTED;
    }
    outside_egg_lift_move(work, force);
    outside_egg_lift_draw(task);
    if (VARS(work)->travel > LIFT_TOP - LIFT_SPEED) {
        collision_register(work);
    }
}

/* take the command in FLAG_LIFT, if any, and clear the flag. the command sets
 * the mode and the travel, and the surface moves there without carrying its
 * riders */
static void outside_egg_lift_obey(struct task_work *work,
                                  struct task_force *force)
{
    Uint8 flag;
    int command;

    /* @hack spelling: two statements as one comma expression, which loads the
     * flag's address into R6 ahead of the store of force at @R15 (0x0c92a9c6
     * to 0x0c92a9cc); as two, the flag goes through R5 after it, and 4 of 68
     * words differ */
    flag = event_flags[FLAG_LIFT], command = flag;
    if (command) {
        switch (command) {
        case LIFT_HIGH_NOW:
            work->mode = LIFT_HIGH;
            VARS(work)->travel = LIFT_TOP;
            break;
        case LIFT_LOW_NOW:
            work->mode = LIFT_LOW;
            VARS(work)->travel = 0.0F;
            break;
        case LIFT_RISE:
            work->mode = LIFT_RISING;
            VARS(work)->travel = 0.0F;
            break;
        case LIFT_SINK:
            work->mode = LIFT_SINKING;
            VARS(work)->travel = LIFT_TOP;
            break;
        case LIFT_READY_NOW:
            work->mode = LIFT_WAIT;
            VARS(work)->travel = LIFT_TOP;
            work->flags |= LIFT_STEP_OFF;
            break;
        case LIFT_RISE_READY:
            work->mode = LIFT_CALLED;
            VARS(work)->travel = 0.0F;
            work->flags |= LIFT_STEP_OFF;
            break;
        }
        outside_egg_lift_move(work, force);
        force->speed.y = 0.0F;
        event_flags[FLAG_LIFT] = LIFT_NO_COMMAND;
    }
}

static void outside_egg_lift_solid_init(struct task *task,
                                        struct task_work *work)
{
    NJS_OBJECT *solid;

    VARS(work)->object = &outside_egg_lift_solid_object;
    VARS(work)->solid = solid = world_object_alloc();
    solid->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK;
    solid->scl[0] = 1.0F;
    solid->scl[1] = 1.0F;
    solid->scl[2] = 1.0F;
    solid->ang[0] = 0;
    solid->ang[1] = VARS(work)->object->ang[1];
    solid->ang[2] = 0;
    solid->pos[0] = VARS(work)->object->pos[0];
    solid->pos[1] = VARS(work)->object->pos[1];
    solid->pos[2] = VARS(work)->object->pos[2];
    solid->model = VARS(work)->object->model;
    solid->child = NULL;
    solid->sibling = NULL;
    world_add(SOLID_FLAGS, task, solid);
}

/* count player 0's steps on the lift while LIFT_STEP_OFF is clear, and clear
 * the count and the flag while player 0 is off it. past BOARD_STEPS steps,
 * disable both pads, start running player 0 to the lift's middle and return
 * TRUE. return FALSE before that */
static Bool outside_egg_lift_take_aboard(struct task *task,
                                         struct task_work *work)
{
    /* @hack spelling: player_stands_on compared with TRUE, CMP/EQ #1 at
     * 0x0c92ab12. player_stands_on returns only TRUE or FALSE, so a plain test
     * of its result is the same; as one, 2 of 48 words differ */
    if (player_stands_on(0, task) == TRUE) {
        if (work->flags & LIFT_STEP_OFF) {
            return FALSE;
        }
        if (++TASK_TIMER(work) > BOARD_STEPS) {
            pad_disable_player(BOTH_PADS);
            player_event_run(0, work->pos.x, work->pos.z);
            return TRUE;
        }
    } else {
        work->flags &= ~LIFT_STEP_OFF;
        work->timer = 0;
    }
    return FALSE;
}

/* run player 0 to the lift's middle. TRUE once player 0's squared distance
 * from it, across x and z, is at most MIDDLE_REACH_SQUARED, else FALSE */
static Bool outside_egg_lift_reach_middle(struct task_work *work)
{
    player_event_run(0, work->pos.x, work->pos.z);
    if (((work->pos.x - player_work[0]->pos.x) *
         (work->pos.x - player_work[0]->pos.x)) +
            ((work->pos.z - player_work[0]->pos.z) *
             (work->pos.z - player_work[0]->pos.z)) >
        MIDDLE_REACH_SQUARED) {
        return FALSE;
    }
    return TRUE;
}

/* move the surface to the travel's height plus the bob. force->speed.y gets
 * the change, so the lift carries what stands on it */
static void outside_egg_lift_move(struct task_work *work,
                                  struct task_force *force)
{
    Float at[3];
    Float height;

    njLinear(outside_egg_lift_ends, at, NULL, VARS(work)->travel);
    height = work->pos.y + at[1] + (njSin(PHASE(work)) * BOB(work));
    force->speed.y = height - VARS(work)->solid->pos[1];
    VARS(work)->solid->pos[1] = height;
}

/* during a battle, command the deck's gates shut at once and return FALSE.
 * otherwise return TRUE once the character's story has opened the lift, else
 * FALSE */
static Bool outside_egg_lift_shut_gates_or_opened(void)
{
    if (battle_rival != -1) {
        event_flags[FLAG_GATE] = GATE_SHUT_NOW;
        return FALSE;
    }
    switch (get_character()) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_EC_EGG_LIFT];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_EC_EGG_LIFT];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_EC_EGG_LIFT];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_EC_EGG_LIFT];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_EC_EGG_LIFT];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_EC_EGG_LIFT];
    }
    return FALSE;
}
