/* O KARTPASS pass that opens the Twinkle Park elevator and Twinkle Circuit for
 * Tails, Knuckles and E-102 */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv00/kart_pass.h"
#include "adv00/station_square.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
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

#define TEXT_1_LENGTH 59
#define TEXT_2_LENGTH 34
#include "adv00/kart_pass/text.inc"

extern NJS_OBJECT kart_pass_object;

/* player 0's held kind while carrying the pass */
#define HELD_KART_PASS 4

/* music played when the pass opens a way */
#define MUSIC_OPENED MUSIC_ITEM1

#define PASS_SPIN 0x400 /* the pass's turn a step while it lies */
/* a dropped pass goes once its timer passes this many steps */
#define PASS_WAIT 1800

/* in the station act, the pass opens the Twinkle Park elevator within
 * ELEVATOR_REACH of this x and z */
#define ELEVATOR_X 356.0F
#define ELEVATOR_Z 1607.0F
#define ELEVATOR_REACH 20.0F
/* in the Twinkle Park act, the pass opens the way into the circuit within
 * CIRCUIT_REACH of this x and z */
#define CIRCUIT_X 757.0F
#define CIRCUIT_Z 1740.0F
#define CIRCUIT_REACH 15.0F

struct kart_pass_vars {
    /* the change in scl.y a step, negative while the pass shrinks */
    Float swell_speed;
    Uint8 unk_04[4];
    Float fade; /* from 0 down to -1 as the pass fades */
};

#define VARS(work) ((struct kart_pass_vars *)&(work)->vars)

enum kart_pass_mode {
    PASS_PLACED, /* lying where it was placed */
    PASS_DROPPED, /* lying where it was dropped */
    PASS_HELD,
    PASS_ALREADY_HELD, /* player 0 held the pass when the task started */
    PASS_SWELLING, /* stretching in y and back; never set */
    /* spinning, stretching, rising and fading with sparkles; never set */
    PASS_FADING,
    /* hidden until kart_pass_grant_and_check returns TRUE */
    PASS_HIDDEN,
    PASS_GONE
};

/* NOLINTNEXTLINE(clang-diagnostic-strict-prototypes) */
static Bool kart_pass_grant_and_check();
static void kart_pass_draw(struct task *task);
static void kart_pass_destroy(struct task *task);
static void kart_pass_step(struct task *task);

/* clang-format off */
static struct collision_shape kart_pass_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.0F, 0.0F}, 5.5F, 3.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL, {0.0F, 3.0F, 0.0F},
     2.0F, 3.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static char *kart_pass_found[] = {
    TEXT_1,
    NULL,
};
static char *kart_pass_welcome[] = {
    TEXT_2,
    NULL,
};

/* once Sonic, Amy and Big have all cleared Twinkle Park, set the
 * CART_PASSPORT flags of Tails, Knuckles and E-102. then return TRUE for those
 * three while their CART_PASSPORT flag is set and their ENTRANCE_CIRCUIT flag
 * isn't */
/* NOLINTNEXTLINE(clang-diagnostic-strict-prototypes) */
static Bool kart_pass_grant_and_check()
{
    if (event_flag_check(FLAG_SONIC_CLEAR_TWINKLE_PARK) &&
        event_flag_check(FLAG_AMY_CLEAR_TWINKLE_PARK) &&
        event_flag_check(FLAG_BIG_CLEAR_TWINKLE_PARK)) {
        event_flag_set(FLAG_TAILS_SS_CART_PASSPORT);
        event_flag_set(FLAG_KNUCKLES_SS_CART_PASSPORT);
        event_flag_set(FLAG_E102_SS_CART_PASSPORT);
    }
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        break;
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_CART_PASSPORT] &&
               !event_flags[FLAG_TAILS_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_CART_PASSPORT] &&
               !event_flags[FLAG_KNUCKLES_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_AMY:
        break;
    case CHARACTER_E102:
        return event_flags[FLAG_E102_SS_CART_PASSPORT] &&
               !event_flags[FLAG_E102_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_BIG:
        break;
    }
    return FALSE;
}

static Bool kart_pass_elevator_open(int character)
{
    switch (character) {
    case CHARACTER_SONIC:
        break;
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_TPARK_ELEVATOR];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_TPARK_ELEVATOR];
    case CHARACTER_AMY:
        break;
    case CHARACTER_E102:
        return event_flags[FLAG_E102_SS_TPARK_ELEVATOR];
    case CHARACTER_BIG:
        break;
    }
    return FALSE;
}

static Bool kart_pass_circuit_open(int character)
{
    switch (character) {
    case CHARACTER_SONIC:
        break;
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_AMY:
        break;
    case CHARACTER_E102:
        return event_flags[FLAG_E102_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_BIG:
        break;
    }
    return FALSE;
}

static void kart_pass_open_elevator(int character)
{
    switch (character) {
    case CHARACTER_SONIC:
        break;
    case CHARACTER_TAILS:
        event_flags[FLAG_TAILS_SS_TPARK_ELEVATOR] = TRUE;
        break;
    case CHARACTER_KNUCKLES:
        event_flags[FLAG_KNUCKLES_SS_TPARK_ELEVATOR] = TRUE;
        break;
    case CHARACTER_AMY:
        break;
    case CHARACTER_E102:
        event_flags[FLAG_E102_SS_TPARK_ELEVATOR] = TRUE;
        break;
    case CHARACTER_BIG:
        break;
    }
}

static void kart_pass_open_circuit(int character)
{
    switch (character) {
    case CHARACTER_SONIC:
        break;
    case CHARACTER_TAILS:
        event_flags[FLAG_TAILS_SS_ENTRANCE_CIRCUIT] = TRUE;
        break;
    case CHARACTER_KNUCKLES:
        event_flags[FLAG_KNUCKLES_SS_ENTRANCE_CIRCUIT] = TRUE;
        break;
    case CHARACTER_AMY:
        break;
    case CHARACTER_E102:
        event_flags[FLAG_E102_SS_ENTRANCE_CIRCUIT] = TRUE;
        break;
    case CHARACTER_BIG:
        break;
    }
}

static void kart_pass_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(VARS(work)->fade, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, 1.0F, work->scl.y, 1.0F);
    draw_tree(&kart_pass_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
}

static void kart_pass_destroy(struct task *task)
{
    task->placement->flags &= ~PLACEMENT_HELD;
}

static void kart_pass_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 act;
    int character;
    int act_copy;
    Float x;
    Float z;
    Float distance;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        kart_pass_draw(task);
    } else {
        act = (Uint8)stage_get_stage_act();
        character = player_get_character(0);
        switch (work->mode) {
        case PASS_PLACED:
        case PASS_DROPPED:
            carry_step_small(task);
            work->ang.y += PASS_SPIN;
            kart_pass_draw(task);
            if (work->flags & CARRY_HELD) {
                player_set_held_kind(0, HELD_KART_PASS);
                work->mode = PASS_HELD;
                message_show_timed(kart_pass_found);
            }
            if (work->mode == PASS_DROPPED) {
                ++work->timer;
                if (TASK_TIMER(work) > PASS_WAIT) {
                    task_kill(task);
                }
            }
            kart_pass_draw(task);
            break;
        case PASS_HELD:
            work->ang.y = -player_work[0]->ang.y;
            carry_step_small(task);
            kart_pass_draw(task);
            if (!(work->flags & CARRY_HELD)) {
                player_set_held_kind(0, 0);
                work->mode = PASS_DROPPED;
                task->placement->flags &= ~PLACEMENT_HELD;
            } else {
                placement_hold(task);
                /* @hack spelling: a copy of act, for which SHC keeps act on
                 * the stack (MOV.W R0,@(4,R15) at 0x0c90de7c) and widens the
                 * copy once, in R10 (EXTU.W R10,R10 at 0x0c90dfca). tested
                 * itself, act stays in R10, each test widens it again, and
                 * 180 of 405 words differ */
                act_copy = act;
                if (!kart_pass_circuit_open(character) &&
                    act_copy == STATION_SQUARE_TWINKLE) {
                    x = work->pos.x - CIRCUIT_X;
                    z = work->pos.z - CIRCUIT_Z;
                    distance = (x * x) + (z * z);
                    if (distance < CIRCUIT_REACH * CIRCUIT_REACH) {
                        kart_pass_open_circuit(character);
                        message_show_timed(kart_pass_welcome);
                        stream_request_interlude(MUSIC_OPENED);
                        work->timer = 0;
                    }
                }
                if (!kart_pass_elevator_open(character) &&
                    act_copy == STATION_SQUARE_STATION) {
                    x = work->pos.x - ELEVATOR_X;
                    z = work->pos.z - ELEVATOR_Z;
                    distance = (x * x) + (z * z);
                    if (distance < ELEVATOR_REACH * ELEVATOR_REACH) {
                        kart_pass_open_elevator(character);
                        stream_request_interlude(MUSIC_OPENED);
                        /* only PASS_SWELLING reads tick and swell_speed, and
                         * nothing sets that mode, so these stores go unread */
                        work->tick = 0;
                        VARS(work)->swell_speed = 0.1F;
                    }
                }
            }
            kart_pass_draw(task);
            break;
        case PASS_ALREADY_HELD:
            break;
        case PASS_SWELLING:
            work->scl.y += VARS(work)->swell_speed;
            if (VARS(work)->swell_speed > 0.0F) {
                if (work->scl.y > 2.0F) {
                    VARS(work)->swell_speed *= -1.0F;
                    ++work->tick;
                }
            } else if (work->scl.y <= 1.0F) {
                VARS(work)->swell_speed *= -1.0F;
                ++work->tick;
            }
            if (work->tick > 10) {
                work->tick = 0;
                work->scl.y = 1.0F;
                work->mode = PASS_HELD;
            }
            kart_pass_draw(task);
            break;
        case PASS_FADING:
            work->ang.y += 2 * PASS_SPIN;
            work->scl.y += 0.02F;
            work->scl.x = 2.0F - work->scl.x;
            VARS(work)->fade -= 0.005F;
            if (VARS(work)->fade <= -1.0F) {
                VARS(work)->fade = -1.0F;
                work->mode = PASS_GONE;
            }
            work->pos.y += 0.15F;
            kart_pass_draw(task);
            ++work->timer;
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
                work->timer = 0;
                /* @hack redundant: a break at the end of the sparkle's block,
                 * which leaves R3 to the pool loads of the cases below
                 * (0x0c90e202, 0x0c90e268). with the outer break alone, they
                 * take R2 and 4 of 405 words differ */
                break;
            }
            break;
        case PASS_GONE:
            placement_kill(task);
            break;
        case PASS_HIDDEN:
            if (kart_pass_grant_and_check()) {
                work->mode = PASS_PLACED;
                task->draw = kart_pass_draw;
            }
            break;
        }
    }
}

void kart_pass_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    VARS(work)->fade = 0.0F;
    work->tick = 0;
    work->timer = 0;
    if (player_get_held_kind(0) == HELD_KART_PASS) {
        work->mode = PASS_ALREADY_HELD;
    } else {
        work->mode = PASS_PLACED;
    }
    carry_set_kind(work, motion, CARRY_KIND_SMALL);
    work->flags |= CARRY_WALLS;
    task->step = kart_pass_step;
    task->destructor = kart_pass_destroy;
    task->draw = kart_pass_draw;
    if (!placement_editing(task)) {
        collision_init(task, kart_pass_shapes, COLLISION_OBJECT, 1, 2);
        /* @bug mismatched-call: kart_pass_grant_and_check takes no argument,
         * but init passes it the task, MOV R13,R4 at 0x0c90e2fa in the slot of
         * the BSR at 0x0c90e2f8 */
        /* NOLINTNEXTLINE */
        if (!kart_pass_grant_and_check(task)) {
            work->mode = PASS_HIDDEN;
            task->draw = NULL;
        }
    }
}
