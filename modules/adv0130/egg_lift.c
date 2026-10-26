/* O EGGLIFT, a lift that takes player 0 up from inside the Egg Carrier to its
 * deck */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv0130/egg_lift.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT egg_lift_solid_object;
extern NJS_ACTION egg_lift_action;

/* player 0 boards the lift by standing on it more than BOARD_STEPS steps.
 * the lift rises LIFT_SPEED action frames a step, to LIFT_TOP, just short of
 * its action's last frame, 100 */
#define BOARD_STEPS 30
#define LIFT_SPEED 0.3F
#define LIFT_TOP 99.99F

#define CLEARANCE 20.0F

/* the collision world tests the lift's surface while player 0 or 1 is within
 * TESTED_NEAR of it */
#define TESTED_NEAR 40.0F

#define SE_LIFT 578

/* both players' pads, as the image passes them to pad_disable_player and
 * pad_enable_player. pad.h's PAD_BOTH_PLAYERS means the same, but is 0xff */
#define BOTH_PADS 2

#define SOLID_FLAGS (WORLD_MEDIUM | WORLD_MOVES | WORLD_SOLID)

struct egg_lift_vars {
    NJS_OBJECT *solid; /* in the collision world */
    /* the object whose model, pos and y angle the lift and solid take */
    NJS_OBJECT *from;
    Float frame; /* the action's frame: 0 at the bottom, LIFT_TOP at the top */
};

#define VARS(work) ((struct egg_lift_vars *)&(work)->vars)

/* the step never sets LIFT_DOWN, so neither it nor LIFT_CLEAR runs */
enum egg_lift_mode {
    LIFT_GONE = -1, /* at the top, while the stage fades to the deck */
    LIFT_INIT = 0,
    LIFT_WAIT = 1,
    LIFT_CLEAR = 2, /* back down, until player 0 is CLEARANCE away */
    LIFT_UP = 4,
    LIFT_DOWN = 5
};

static struct dust_params egg_lift_dust = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    0.01F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.5F, 1.0F, 1.0F, 1.0F},
};

static void egg_lift_solid_init(struct task *task, struct task_work *work);
static void egg_lift_draw(struct task *task);
static void egg_lift_destroy(struct task *task);
static void egg_lift_move(struct task_work *work, struct task_force *force);
static void egg_lift_puff(struct task_work *work);
static Bool egg_lift_take_aboard(struct task *task, struct task_work *work);
static Bool egg_lift_reach_middle(struct task_work *work);
static Bool egg_lift_open(void);
static Bool egg_carrier_transformed(void);

/* the step does nothing, and the lift neither draws nor collides, until
 * egg_lift_open is TRUE */
void egg_carrier_egg_lift_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;

    if (!placement_try_leave(task) && egg_lift_open()) {
        switch (work->mode) {
        /* @hack redundant: the empty default, which has to stand ahead of
         * LIFT_INIT, keeps LIFT_INIT's first load of VARS(work)->from in R2,
         * not R3 (MOV.L @(12,R14),R2 at 0x0c925c3a); without it, 2 of 234
         * words differ */
        default:
            break;
        case LIFT_GONE:
            break;
        case LIFT_INIT:
            egg_lift_solid_init(task, work);
            work->pos.x = VARS(work)->from->pos[0];
            work->pos.y = VARS(work)->from->pos[1];
            work->pos.z = VARS(work)->from->pos[2];
            work->ang.y = VARS(work)->from->ang[1];
            task->destructor = egg_lift_destroy;
            task->draw = egg_lift_draw;
            work->timer = 0;
            VARS(work)->frame = 0.0F;
            work->mode = LIFT_WAIT;
            egg_lift_move(work, force);
            break;
        case LIFT_WAIT:
            if (egg_lift_take_aboard(task, work) &&
                egg_lift_reach_middle(work)) {
                work->mode = LIFT_UP;
                egg_lift_puff(work);
            }
            break;
        case LIFT_CLEAR:
            if (njDistanceP2P(&player_work[0]->pos, &work->pos) < CLEARANCE) {
                break;
            }
            work->mode = LIFT_WAIT;
            break;
        case LIFT_UP:
            sound_play_for(SE_LIFT, (int)work, 1, 0, 2);
            player_event_stop(0);
            egg_lift_move(work, force);
            VARS(work)->frame += LIFT_SPEED;
            if (VARS(work)->frame < LIFT_TOP) {
                break;
            }
            work->mode = LIFT_GONE;
            VARS(work)->frame = LIFT_TOP;
            force->speed.x = 0.0F;
            force->speed.y = 0.0F;
            force->speed.z = 0.0F;
            force->turn.y = 0;
            pad_enable_player(BOTH_PADS);
            if (egg_carrier_transformed()) {
                stage_set_entrance(0);
                stage_fade_to(STAGE_EGG_CARRIER, 2);
            } else {
                stage_set_entrance(3);
                stage_fade_to(STAGE_EGG_CARRIER, 0);
            }
            break;
        case LIFT_DOWN:
            egg_lift_move(work, force);
            /* @hack spelling: the speed negated and added, -0.3 in the pool
             * at 0x0c925e04 and an FADD at 0x0c925d90; subtracted, 1 of 234
             * words differ */
            VARS(work)->frame += -LIFT_SPEED;
            if (VARS(work)->frame > 0.0F) {
                break;
            }
            egg_lift_puff(work);
            pad_enable_player(BOTH_PADS);
            VARS(work)->frame = 0.0F;
            force->speed.x = 0.0F;
            force->speed.y = 0.0F;
            force->speed.z = 0.0F;
            force->turn.y = 0;
            work->mode = LIFT_CLEAR;
            break;
        }
        work->flags &= ~WORLD_TESTED;
        if (player_in_range(&work->pos, TESTED_NEAR) != 0) {
            work->flags |= WORLD_TESTED;
        }
        egg_lift_draw(task);
    }
}

static void egg_lift_solid_init(struct task *task, struct task_work *work)
{
    NJS_OBJECT *solid;

    VARS(work)->from = &egg_lift_solid_object;
    VARS(work)->solid = solid = world_object_alloc();
    solid->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK;
    solid->scl[0] = 1.0F;
    solid->scl[1] = 1.0F;
    solid->scl[2] = 1.0F;
    solid->ang[0] = 0;
    solid->ang[1] = VARS(work)->from->ang[1];
    solid->ang[2] = 0;
    solid->pos[0] = VARS(work)->from->pos[0];
    solid->pos[1] = VARS(work)->from->pos[1];
    solid->pos[2] = VARS(work)->from->pos[2];
    solid->model = VARS(work)->from->model;
    solid->child = NULL;
    solid->sibling = NULL;
    world_add(SOLID_FLAGS, task, solid);
}

static void egg_lift_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (egg_lift_open()) {
        texture_set_stage();
        njPushMatrix(NULL);
        draw_action(&egg_lift_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void egg_lift_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

/* move the lift and its surface to where the action puts them at
 * VARS(work)->frame. force takes the change, so the lift carries what stands
 * on it */
static void egg_lift_move(struct task_work *work, struct task_force *force)
{
    NJS_MOTION_DATA data;
    Float pos[3];
    Angle ang[3];
    Float scl[3];

    data.pos = pos;
    data.ang = ang;
    data.scl = scl;
    njPushMatrix(NULL);
    njGetAction(&egg_lift_action, &data, VARS(work)->frame);
    njPopMatrix(1);
    force->speed.x = pos[0] - work->pos.x;
    force->speed.y = pos[1] - work->pos.y;
    force->speed.z = pos[2] - work->pos.z;
    force->turn.y = -ang[1] + work->ang.y;
    VARS(work)->solid->pos[0] = work->pos.x = pos[0];
    VARS(work)->solid->pos[1] = work->pos.y = pos[1];
    VARS(work)->solid->pos[2] = work->pos.z = pos[2];
    VARS(work)->solid->ang[1] = work->ang.y = ang[1];
}

static void egg_lift_puff(struct task_work *work)
{
    Angle ang;

    for (ang = 0; ang < 0x10000; ang += 0x800) {
        egg_lift_dust.pos.x = work->pos.x + (njCos(ang) * 20.0F);
        egg_lift_dust.pos.y = work->pos.y + (rand() * RAND_SCALE);
        egg_lift_dust.pos.z = work->pos.z + (njSin(ang) * 20.0F);
        egg_lift_dust.speed.x = njCos(ang) * 0.3F;
        egg_lift_dust.speed.y = 0.0F;
        egg_lift_dust.speed.z = njSin(ang) * 0.3F;
        egg_lift_dust.scale = 0.8F;
        dust_create_glow(&egg_lift_dust);
    }
}

/* count the steps player 0 stands on the lift in TASK_TIMER(work), from 0
 * again each time it steps off. once the count passes BOARD_STEPS, stop both
 * players' pads, run player 0 toward the lift's middle, and return TRUE.
 * FALSE before then */
static Bool egg_lift_take_aboard(struct task *task, struct task_work *work)
{
    if (player_stands_on(0, task) == TRUE) {
        if (++TASK_TIMER(work) > BOARD_STEPS) {
            pad_disable_player(BOTH_PADS);
            player_event_run(0, work->pos.x, work->pos.z);
            return TRUE;
        }
    } else {
        work->timer = 0;
    }
    return FALSE;
}

/* run player 0 toward the lift's middle. TRUE once it is within 1 of the
 * middle in x and z, else FALSE */
static Bool egg_lift_reach_middle(struct task_work *work)
{
    struct task_work *player;

    player_event_run(0, work->pos.x, work->pos.z);
    player = player_work[0];
    if (((work->pos.x - player->pos.x) * (work->pos.x - player->pos.x)) +
            ((work->pos.z - player->pos.z) * (work->pos.z - player->pos.z)) >
        1.0F) {
        return FALSE;
    }
    return TRUE;
}

/* the character's FLAG_*_EC_EGG_LIFT, which its story sets to open the lift.
 * FALSE for Eggman and Tikal */
static Bool egg_lift_open(void)
{
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

/* the character's FLAG_*_EC_TRANSFORM, set while the Egg Carrier is
 * transformed. FALSE for Eggman and Tikal */
static Bool egg_carrier_transformed(void)
{
    switch (get_character()) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_EC_TRANSFORM];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_EC_TRANSFORM];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_EC_TRANSFORM];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_EC_TRANSFORM];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_EC_TRANSFORM];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_EC_TRANSFORM];
    }
    return FALSE;
}
