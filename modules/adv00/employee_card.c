/* O CARD employee card that Sonic carries to the station's reader to open the
 * way to the highway */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv00/employee_card.h"
#include "adv00/station_square.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "effect/sparkle.h"
#include "event/script.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 45
#define TEXT_2_LENGTH 8
#include "adv00/employee_card/text.inc"

/* @hack spelling: event_flags[FLAG_SONIC_SS_ENTRANCE_HIGHWAY] under a name of
 * its own, at the flag's address in the symbol table. stored through it, the
 * flag's address takes R3 and the stored 1 takes R2 (MOV.B R2,@R3 at
 * 0x0c9010bc). stored through the array, the two swap and 7 of
 * employee_card_step's 338 words differ */
extern Uint8 sonic_ss_entrance_highway;

extern NJS_ACTION employee_card_action;

/* player 0's held kind while carrying the card */
#define HELD_CARD 3

/* music played when the reader accepts the card */
#define MUSIC_READER MUSIC_ITEM1

#define CARD_SPIN 0.5F /* motion frames a step while the card lies */
/* a dropped card goes once it has lain more than this many steps */
#define CARD_WAIT 1800

/* the reader takes the card within READER_REACH of one of its READER_SPOTS
 * spots, in x and z. the spots stand in a row at READER_X */
#define READER_SPOTS 3
#define READER_X 369.0F
#define READER_REACH 35.0F

struct card_vars {
    NJS_ACTION *action;
    Float frame; /* the card's frame in its spinning motion */
    Float fade; /* from 0 down to -1 as the card fades */
};

#define VARS(work) ((struct card_vars *)&(work)->vars)

enum card_mode {
    CARD_PLACED, /* lying where it was placed */
    CARD_DROPPED, /* lying where it was dropped */
    CARD_HELD,
    CARD_ALREADY_HELD, /* player 0 held the card when the task started */
    CARD_READ, /* rising, stretching and fading with sparkles; never set */
    CARD_HIDDEN, /* hidden until employee_card_available returns TRUE */
    CARD_GONE
};

static Bool employee_card_at_reader(NJS_POINT3 *pos);
/* NOLINTNEXTLINE(clang-diagnostic-strict-prototypes) */
static Bool employee_card_available();
static void employee_card_draw(struct task *task);
static void employee_card_fade_draw(struct task *task);
static void employee_card_destroy(struct task *task);
static void employee_card_step(struct task *task);

/* clang-format off */
static struct collision_shape employee_card_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.0F, 0.0F}, 5.5F, 3.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL, {0.0F, 3.0F, 0.0F},
     2.0F, 3.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* Sonic's line when he picks up the card, and the reader's message */
static char *employee_card_found[] = {
    TEXT_1,
    NULL,
};
static char *employee_card_checked[] = {
    TEXT_2,
    NULL,
};

/* the z of each of the reader's spots */
static Float employee_card_reader_z[READER_SPOTS] = {1320.0F, 1360.0F, 1400.0F};

/* TRUE while pos is within READER_REACH of one of the reader's spots, in x and
 * z */
static Bool employee_card_at_reader(NJS_POINT3 *pos)
{
    Float x = pos->x - READER_X;
    Float z = pos->z;
    Float distance;
    int i;

    x *= x;
    for (i = 0; i < READER_SPOTS; i++) {
        distance = z - employee_card_reader_z[i];
        distance = x + (distance * distance);
        if (distance < READER_REACH * READER_REACH) {
            return TRUE;
        }
    }
    return FALSE;
}

/* TRUE for Sonic while FLAG_SONIC_SS_CARD is set and
 * FLAG_SONIC_SS_ENTRANCE_HIGHWAY isn't */
/* NOLINTNEXTLINE(clang-diagnostic-strict-prototypes) */
static Bool employee_card_available()
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        /* @hack spelling: & for &&, with which SHC reads both flags and ands
         * them without a branch (AND R3,R0 at 0x0c900d76). every store to the
         * two flags is 1 or 0, so & and && agree. with &&, employee_card_draw
         * moves, and 20 of the function's 34 words differ */
        return event_flags[FLAG_SONIC_SS_CARD] &
               !event_flags[FLAG_SONIC_SS_ENTRANCE_HIGHWAY];
    case CHARACTER_TAILS:
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
    default:
        return FALSE;
    }
}

static void employee_card_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(VARS(work)->action, VARS(work)->frame);
    njPopMatrix(1);
}

static void employee_card_fade_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(VARS(work)->fade, 0.0F, 0.0F, 0.0F);
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    draw_action(VARS(work)->action, VARS(work)->frame);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
}

static void employee_card_destroy(struct task *task)
{
    task->placement->flags &= ~PLACEMENT_HELD;
}

static void employee_card_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        employee_card_draw(task);
        return;
    }
    switch (work->mode) {
    case CARD_PLACED:
    case CARD_DROPPED:
        /* @hack spelling: the frame advanced in the test, which SHC compares
         * from FR2, where the sum is (FCMP/GT at 0x0c900fa2). as a statement of
         * its own, SHC reads the frame back to compare it, and 78 of 338 words
         * differ */
        if ((VARS(work)->frame += CARD_SPIN) >=
            VARS(work)->action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        carry_step_small(task);
        employee_card_draw(task);
        if (work->flags & CARRY_HELD) {
            player_set_held_kind(0, HELD_CARD);
            work->mode = CARD_HELD;
            message_show_timed(employee_card_found);
        }
        if (work->mode == CARD_DROPPED) {
            TASK_TIMER(work)++;
            if (TASK_TIMER(work) > CARD_WAIT) {
                task_kill(task);
            }
        }
        break;
    case CARD_HELD:
        VARS(work)->frame = 0.0F;
        work->ang.y = -player_work[0]->ang.y;
        carry_step_small(task);
        employee_card_draw(task);
        if (!(work->flags & CARRY_HELD)) {
            player_set_held_kind(0, 0);
            work->mode = CARD_DROPPED;
            TASK_TIMER(work) = 0;
            task->placement->flags &= ~PLACEMENT_HELD;
        } else {
            placement_hold(task);
            if (!event_flag_check(FLAG_SONIC_SS_ENTRANCE_HIGHWAY) &&
                (Uint8)stage_get_stage_act() == STATION_SQUARE_STATION) {
                if (!employee_card_at_reader(&work->pos)) {
                    break;
                }
                sonic_ss_entrance_highway = 1;
                VARS(work)->fade = 0.0F;
                message_show_timed(employee_card_checked);
                stream_request_interlude(MUSIC_READER);
                TASK_TIMER(work) = 0;
            }
        }
        break;
    case CARD_ALREADY_HELD:
        break;
    case CARD_READ:
        /* @hack spelling: the frame advanced in the test, as above (FCMP/GT at
         * 0x0c90111e). as a statement of its own, 122 of 338 words differ */
        if ((VARS(work)->frame += 1.0F) >=
            VARS(work)->action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        work->scl.y += 0.02F;
        work->scl.x = 2.0F - work->scl.x;
        VARS(work)->fade -= 0.005F;
        if (VARS(work)->fade <= -1.0F) {
            VARS(work)->fade = -1.0F;
            work->mode = CARD_GONE;
        }
        work->pos.y += 0.15F;
        employee_card_fade_draw(task);
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > 6) {
            NJS_VECTOR speed = {0.0F, 0.0F, 0.0F};
            Float y;

            speed.x = ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
            speed.y = ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
            speed.z = ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
            y = work->pos.y;
            work->pos.y += 8.0F;
            sparkle_create(&work->pos, &speed, 0.3F, 0x38e);
            work->pos.y = y;
            TASK_TIMER(work) = 0;
            /* @hack redundant: a break at the end of the sparkle's block, which
             * leaves R3 to the pool loads of the cases below (0x0c901260,
             * 0x0c901272). without it, they take R2 and 4 of 338 words
             * differ */
            break;
        }
        break;
    case CARD_GONE:
        placement_kill(task);
        break;
    case CARD_HIDDEN:
        if (employee_card_available()) {
            work->mode = CARD_PLACED;
            task->draw = employee_card_draw;
        }
        break;
    }
}

void employee_card_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    VARS(work)->fade = 0.0F;
    if (player_get_held_kind(0) == HELD_CARD) {
        work->mode = CARD_ALREADY_HELD;
    } else {
        work->mode = CARD_PLACED;
    }
    VARS(work)->action = &employee_card_action;
    carry_set_kind(work, motion, CARRY_KIND_SMALL);
    work->flags |= CARRY_WALLS;
    task->step = employee_card_step;
    task->destructor = employee_card_destroy;
    task->draw = employee_card_draw;
    if (!placement_editing(task)) {
        collision_init(task, employee_card_shapes, COLLISION_OBJECT, 1, 2);
        /* @bug mismatched-call: employee_card_available takes no argument, but
         * init passes it the task, MOV R13,R4 at 0x0c9012fc in the slot of the
         * BSR at 0x0c9012fa */
        /* NOLINTNEXTLINE */
        if (!employee_card_available(task)) {
            work->mode = CARD_HIDDEN;
            task->draw = NULL;
        }
    }
}
