/* O KEYICE the Ice Stone, which Sonic, Tails and Big carry from Station Square
 * to the Mystic Ruins */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/ice_stone.h"
#include "adv00/station_square.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "event/script.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 63
#include "adv00/ice_stone/text.inc"

extern NJS_ACTION ice_stone_action;

/* player 0's held kind while carrying the stone */
#define HELD_ICE_STONE 1

#define STONE_FRAME 0.5F /* motion frames a step while the stone lies loose */
/* a dropped stone goes once it has lain more than this many steps */
#define STONE_WAIT 1800

struct ice_stone_vars {
    NJS_ACTION *action;
    Float frame;
};

#define VARS(work) ((struct ice_stone_vars *)&(work)->vars)

enum ice_stone_mode {
    STONE_PLACED, /* lying where it was placed */
    STONE_DROPPED, /* lying where it was dropped */
    STONE_HELD,
    STONE_ALREADY_HELD /* player 0 held the stone when the task started */
};

/* clang-format off */
static struct collision_shape ice_stone_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.0F, 0.0F}, 5.5F, 6.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL, {0.0F, 2.0F, 0.0F},
     2.0F, 6.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static char *ice_stone_found[] = {
    TEXT_1,
    NULL,
};

/* the stone lies for Sonic and Tails in the casino act, and for Big in the
 * station act. TRUE there while the character's SS_ICE_STONE flag is set and
 * its MR_ICE_STONE flag isn't */
/* @hack spelling: each of the three tests as &, not &&, with which SHC reads
 * both flags and ands them without a branch (AND R3,R0 at 0x0c90d770).
 * every store to the six flags is 1 or 0, so & and && agree. with &&, the
 * function grows, and ice_stone.c's P runs 64 bytes past its row, count
 * unmeasured */
static Bool ice_stone_available(struct task *task)
{
    Uint8 act = stage_get_stage_act();
    int character = player_get_character(0);

    switch (act) {
    case STATION_SQUARE_CASINO:
        switch (character) {
        case CHARACTER_SONIC:
            return event_flags[FLAG_SONIC_SS_ICE_STONE] &
                   !event_flags[FLAG_SONIC_MR_ICE_STONE];
        case CHARACTER_TAILS:
            return event_flags[FLAG_TAILS_SS_ICE_STONE] &
                   !event_flags[FLAG_TAILS_MR_ICE_STONE];
        case CHARACTER_KNUCKLES:
        case CHARACTER_AMY:
        case CHARACTER_E102:
        case CHARACTER_BIG:
            break;
        }
        break;
    case STATION_SQUARE_STATION:
        switch (character) {
        case CHARACTER_SONIC:
        case CHARACTER_TAILS:
        case CHARACTER_KNUCKLES:
        case CHARACTER_AMY:
        case CHARACTER_E102:
            break;
        case CHARACTER_BIG:
            return event_flags[FLAG_BIG_SS_ICE_STONE] &
                   !event_flags[FLAG_BIG_MR_ICE_STONE];
        }
        break;
    }
    return FALSE;
}

/* draw the stone, and pose it for the next draw outside the set editor. a
 * held stone turns with player 0 and rests at its motion's first frame. a
 * loose one plays its motion over and over */
static void ice_stone_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(VARS(work)->action, VARS(work)->frame);
    njPopMatrix(1);
    if (!placement_editing(task)) {
        if (work->flags & CARRY_HELD) {
            VARS(work)->frame = 0.0F;
            work->ang.y = -player_work[0]->ang.y;
        } else {
            /* @hack spelling: the loose stone's frame advanced in the test,
             * which SHC compares from FR2, where the sum is (FCMP/GT at
             * 0x0c90d82a). as a statement of its own, SHC reads the frame back
             * to compare it, and 13 of 77 words differ */
            if ((VARS(work)->frame += STONE_FRAME) >=
                VARS(work)->action->motion->nbFrame) {
                VARS(work)->frame = 0.0F;
            }
        }
    }
}

static void ice_stone_destroy(struct task *task)
{
    task->placement->flags &= ~PLACEMENT_HELD;
}

static void ice_stone_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: motion set and never read. SHC drops the load, but
     * the local leaves task R14 and work R13 (MOV R4,R14 at 0x0c90d8a2);
     * without it, the two trade and 27 of 104 words differ */
    struct task_motion *motion = task->motion;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        ice_stone_draw(task);
    } else {
        ice_stone_draw(task);
        switch (work->mode) {
        case STONE_PLACED:
        case STONE_DROPPED:
            carry_step_small(task);
            if (work->mode == STONE_DROPPED) {
                ++work->timer;
                if (TASK_TIMER(work) > STONE_WAIT) {
                    task_kill(task);
                }
            }
            if (work->flags & CARRY_HELD) {
                player_set_held_kind(0, HELD_ICE_STONE);
                work->mode = STONE_HELD;
                message_show_timed(ice_stone_found);
            }
            break;
        case STONE_HELD:
            work->ang.y = -player_work[0]->ang.y;
            carry_step_small(task);
            if (!(work->flags & CARRY_HELD)) {
                player_set_held_kind(0, 0);
                work->timer = 0;
                work->mode = STONE_DROPPED;
                task->placement->flags &= ~PLACEMENT_HELD;
            } else {
                placement_hold(task);
            }
            break;
        case STONE_ALREADY_HELD:
            break;
        }
    }
}

void ice_stone_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->ang.x = 0;
    work->ang.z = 0;
    work->timer = 0;
    if (placement_editing(task) || ice_stone_available(task)) {
        if (player_get_held_kind(0) == HELD_ICE_STONE) {
            work->mode = STONE_ALREADY_HELD;
        } else {
            work->mode = STONE_PLACED;
        }
        VARS(work)->action = &ice_stone_action;
        if (!placement_editing(task)) {
            collision_init(task, ice_stone_shapes, COLLISION_OBJECT, 1, 2);
        }
        carry_set_kind(work, motion, CARRY_KIND_SMALL);
        work->flags |= CARRY_WALLS;
        task->step = ice_stone_step;
        task->destructor = ice_stone_destroy;
        task->draw = ice_stone_draw;
    }
}
