/* the player core: the record, moves and collision every character shares */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/invisible.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "effect/crash_stars.h"
#include "effect/dust.h"
#include "effect/effect.h"
#include "effect/particle.h"
#include "effect/snow.h"
#include "effect/splash.h"
#include "event/event.h"
#include "event/play.h"
#include "hud/hud.h"
#include "object/common.h"
#include "object/panel.h"
#include "path/path.h"
#include "player/effects.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "sboard/sand.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "system/vector.h"

/* 61.875 degrees: the most a surface's normal can turn from a direction
 * and still face it. a normal facing the player's up marks the surface as
 * ground, though other tests can too. a wall whose normal faces the
 * player's motion, reversed, meets that motion head on */
#define FACING_TURN 0x2c00

/* how the wall sweeps mark each contact in contact_flags, which nothing
 * reads: as ground, or as a wall the ball moves into */
#define CONTACT_GROUND 0x2
#define CONTACT_WALL 0x8

struct task_work *player_work[PLAYER_COUNT];
struct task_motion *player_motion[PLAYER_COUNT];
struct task *player_tasks[PLAYER_COUNT];
struct player *player_records[PLAYER_COUNT];
Uint8 player_history_index[PLAYER_COUNT];
NJS_POINT3 player_history_pos[PLAYER_HISTORY_STEPS][PLAYER_COUNT];
Angle player_history_ang[PLAYER_HISTORY_STEPS][PLAYER_COUNT][3];
Sint8 player_held_kinds[CHARACTER_COUNT];
/* TRUE once player_collide_world finds WORLD_NO_SLOPE ground among the
 * world objects it gathers near the player. it stays FALSE while
 * event_in_control holds */
static Bool no_slope_near;
/* a static no source reads, whose 4 bytes keep the weld matrices below at
 * their place in the zeroed data */
static Sint32 dat_8c7a1214;
/* the matrices of a weld's two nodes in the pose, and their inverses back
 * into each node's frame */
static NJS_MATRIX player_weld_matrix_a;
static NJS_MATRIX player_weld_matrix_b;
static NJS_MATRIX player_weld_inverse_a;
static NJS_MATRIX player_weld_inverse_b;
/* each player's step and draw, as player_suspend found them */
static void (*player_saved_steps[PLAYER_COUNT])(struct task *task);
static void (*player_saved_draws[PLAYER_COUNT])(struct task *task);

/* clang-format off */
static struct character_constants character_constants[CHARACTER_COUNT] = {
    /* Sonic */
    {60, 2.0F, 16.0F, 16.0F, 3.0F, 0.6F, 1.66F, 3.0F, 0.23F,
     0.46F, 1.39F, 2.3F, 3.7F, 5.09F, 0.076F,
     0.05F, 0.031F, -0.06F, -0.18F, -0.17F,
     -0.028F, {-0.008F, -0.01F, -0.4F}, -0.1F, -0.6F, -0.2825F, 0.3F,
     4.0F, 10.0F, 0.08F, 7.0F, 5.4F},
    /* Eggman */
    {60, 3.0F, 16.0F, 16.0F, 3.0F, 1.0F, 1.66F, 3.0F, 0.23F,
     0.46F, 1.39F, 2.3F, 3.7F, 5.09F, 0.076F,
     0.06F, 0.031F, -0.06F, -0.18F, -0.17F,
     -0.028F, {-0.008F, -0.01F, -0.4F}, -0.1F, -0.6F, -0.3375F, 0.3F,
     8.5F, 18.0F, 0.08F, 7.0F, 5.3F},
    /* Tails */
    {60, 1.5F, 16.0F, 16.0F, 2.0F, 0.6F, 1.66F, 3.0F, 0.23F,
     0.46F, 1.39F, 2.8F, 3.7F, 5.09F, 0.076F,
     0.06F, 0.031F, -0.06F, -0.18F, -0.17F,
     -0.028F, {-0.008F, -0.01F, -0.4F}, -0.1F, -0.6F, -0.3375F, 0.3F,
     3.5F, 9.0F, 0.08F, 6.0F, 4.5F},
    /* Knuckles */
    {60, 2.0F, 16.0F, 16.0F, 2.5F, 0.6F, 1.66F, 3.0F, 0.23F,
     0.46F, 1.39F, 3.1F, 3.7F, 5.09F, 0.076F,
     0.05F, 0.031F, -0.06F, -0.18F, -0.17F,
     -0.028F, {-0.008F, -0.01F, -0.4F}, -0.1F, -0.6F, -0.3375F, 0.3F,
     4.0F, 11.4F, 0.08F, 9.0F, 5.7F},
    /* Tikal */
    {60, 2.0F, 16.0F, 16.0F, 3.0F, 1.0F, 1.66F, 3.0F, 0.23F,
     0.46F, 1.39F, 2.3F, 3.7F, 5.09F, 0.076F,
     0.06F, 0.031F, -0.06F, -0.18F, -0.17F,
     -0.028F, {-0.008F, -0.01F, -0.4F}, -0.1F, -0.6F, -0.3375F, 0.3F,
     4.0F, 10.0F, 0.08F, 7.0F, 5.0F},
    /* Amy */
    {60, 1.5F, 16.0F, 16.0F, 0.05F, 0.5F, 1.3F, 3.0F, 0.23F,
     0.46F, 1.39F, 2.3F, 3.7F, 5.09F, 0.013F,
     0.045F, 0.031F, -0.06F, -0.18F, -0.17F,
     -0.028F, {-0.008F, -0.01F, -0.4F}, -0.1F, -0.6F, -0.3375F, 0.3F,
     3.5F, 10.0F, 0.04F, 7.0F, 5.0F},
    /* E-102 */
    {60, 2.0F, 16.0F, 16.0F, 2.5F, 0.6F, 2.0F, 3.0F, 0.23F,
     1.0F, 2.0F, 2.5F, 3.7F, 5.09F, 0.09F,
     0.06F, 0.031F, -0.06F, -0.25F, -0.17F,
     -0.028F, {-0.008F, -0.01F, -0.4F}, -0.1F, -0.6F, -0.9F, 0.3F,
     4.0F, 21.0F, 0.1F, 20.0F, 14.0F},
    /* Big */
    {60, 2.0F, 5.0F, 8.0F, 1.0F, 0.2F, 2.0F, 3.0F, 0.23F,
     0.32F, 1.39F, 2.3F, 3.7F, 5.09F, 0.1F,
     0.08F, 0.031F, -0.08F, -0.18F, -0.17F,
     -0.028F, {-0.04F, -0.01F, -0.4F}, -0.1F, -0.6F, -0.2F, 0.3F,
     9.5F, 17.0F, 0.135F, 15.0F, 8.0F},
};
/* clang-format on */

/* a stage's lists of zones, at the address they take once its module is
 * loaded */
#define ZONES(address) ((struct death_zone **)(address))

struct death_zone **death_zones[STAGE_COUNT] = {
    NULL,
    ZONES(0x0cab1338), /* Emerald Coast */
    ZONES(0x0ca14600), /* Windy Valley */
    ZONES(0x0c9c3b68), /* Twinkle Park */
    ZONES(0x0ca34010), /* Speed Highway */
    ZONES(0x0ca39fb4), /* Red Mountain */
    ZONES(0x0c91f938), /* Sky Deck */
    ZONES(0x0c90cdf8), /* Lost World */
    ZONES(0x0ca511ec), /* Ice Cap */
    ZONES(0x0cb17564), /* Casinopolis */
    ZONES(0x0cb003e0), /* Final Egg */
    NULL,
    ZONES(0x0ca91520), /* Hot Shelter */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ZONES(0x0cb2edd8), /* Station Square */
    NULL,
    NULL,
    ZONES(0x0cae7418), /* the Egg Carrier */
    NULL,
    NULL,
    ZONES(0x0c9cdaf0), /* inside the Egg Carrier */
    ZONES(0x0cb20104), /* the Mystic Ruins */
    ZONES(0x0c9ca070), /* the Past */
    ZONES(0x0c900230), /* Twinkle Circuit */
    NULL,
    NULL,
    NULL,
    ZONES(0x0cb9356c), /* Station Square's garden */
    ZONES(0x0cb9032c), /* the Egg Carrier's */
    ZONES(0x0cb9ba24), /* the Mystic Ruins' */
    NULL,
};

/* the foot and head heights player_step_up takes for slot 5 while
 * no_slope_near is TRUE, in place of 4.0 and the ball's radius */
static Float step_up_foot = 5.2F;
static Float step_up_head = 4.0F;

/* what a board throws up: snow off Ice Cap's ground, and sand */
/* clang-format off */
static struct dust_params board_snow = {
    1.0F, 0.038F, 0.19F, 0.06F, 0.04F, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F},
    {0.8F, 0.4F, 0.4F, 0.4F}};
static struct dust_params board_sand = {
    1.0F, 0.038F, 0.19F, 0.06F, 0.04F, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F}};
/* clang-format on */

/* the seconds left to breathe, as a drowning player counts them down */
static struct hud_number air_number = {
    NUMBER_SCALED | NUMBER_CLAMPED,
    0,
    0,
    5,
    0,
    {0.0F, 0.0F, 0.0F},
    1.0F,
    0xffffff60,
};

struct drown {
    Uint8 player;
    Uint32 steps;
};

#define DROWN(task) ((struct drown *)(task)->extra)

static void player_note_history(Uint8 player);
static void player_accel_cling(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void player_accel_level(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void player_accel_ground_event(struct task_work *work,
                                      struct task_motion *motion,
                                      struct player *record);
static void player_accel_air_event(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record);
static void player_follow_path(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void player_roll_downhill(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static void player_steer_downhill(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record);
static void player_turn_toward(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record, Angle ang);
static void player_turn_on_course(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record, Angle ang);
static void player_turn_at_speed(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record, Angle ang);
static void player_limit_speed(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void player_stop_creep(struct task_work *work,
                              struct task_motion *motion);
static void player_slide_along_wall(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record);
static Uint32 player_collide_ground(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record);
static Uint32 player_collide_world(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record);
static Bool player_step_up(struct task_work *work, struct polygon_ball *ball,
                           struct world_object *world);
static Uint32 player_hit_wall(struct task_work *work,
                              struct task_motion *motion, struct player *record,
                              struct polygon_ball *ball, NJS_POINT3 *old_pos,
                              NJS_VECTOR *offset, Uint32 result, Uint32 flags);
static Uint32 pinball_hit_wall(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record, struct polygon_ball *ball,
                               NJS_POINT3 *old_pos, NJS_VECTOR *offset,
                               Uint32 result, Uint32 flags);
static void ball_drop(struct task_motion *motion, struct player *record,
                      struct polygon_ball *ball, NJS_VECTOR *position);
static void player_tilt_to_ground(struct task_work *work, struct player *record,
                                  struct polygon_ball *ball);
static void player_ride_task(struct task_work *work, struct player *record,
                             struct polygon_ball *ball, NJS_VECTOR *offset);
static void player_slip_off(struct task_work *work, struct task_motion *motion,
                            struct polygon_ball *ball);
static void player_set_ground(struct task_work *work,
                              struct task_motion *motion, struct player *record,
                              Uint32 flags);
static Bool ball_stop_at_walls(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record, struct polygon_ball *ball,
                               Uint32 flags);
static Bool pinball_stop_at_walls(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record,
                                  struct polygon_ball *ball, Uint32 flags);
static Bool ball_stop_at_ground(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record,
                                struct polygon_ball *ball);
static int ball_slide_along_walls(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record,
                                  struct polygon_ball *ball, Uint32 flags);
static int pinball_slide_along_walls(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record,
                                     struct polygon_ball *ball, Uint32 flags);
static void player_deflect(struct task_work *work, struct player *record,
                           struct polygon_ball *ball, NJS_VECTOR *vector);
static void pinball_deflect(struct task_work *work, struct player *record,
                            struct polygon_ball *ball, NJS_VECTOR *vector);
static Uint32 player_collide_pinball(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record);
static void ground_turn_back(NJS_VECTOR *normal, NJS_VECTOR *vector,
                             Angle ang_y);
static void ground_turn_to_world(NJS_VECTOR *normal, NJS_VECTOR *vector,
                                 Angle ang_y);
static void player_anim_play_next(struct player_anim *anim, Uint16 index);
static void player_anim_start(struct player_anim *anim);
static void player_anim_blend_from(struct player_anim *anim);
static void player_anim_make_blend(struct player_anim *anim);
static void player_anim_free_blend(struct player_anim *anim);
static void player_mouth_blend(NJS_OBJECT *object, struct player_mouth *mouth,
                               Sint16 *index);
static void player_mode_sound_step(struct task *task);
static void player_fall_destroy(struct task *task);
static void player_fall_step(struct task *task);
static void player_fall_cry(Uint8 player);
static void player_fall_in_zone(struct task *parent, Uint8 player);
static void player_drown_draw(struct task *task);
static void player_drown_destroy(struct task *task);
static void player_drown_step(struct task *task);
static void player_look_timer_destroy(struct task *task);
static void player_look_timer_step(struct task *task);
static void player_speed_up_step(struct task *task);
static void player_speed_up_destroy(struct task *task);
static void player_speed_up_start(struct task *task);

void player_clear_slots(void)
{
    int i;

    for (i = 0; i < PLAYER_COUNT; i++) {
        player_tasks[i] = NULL;
        player_work[i] = NULL;
        player_motion[i] = NULL;
        player_records[i] = NULL;
    }
}

Bool player_debug_pressed(int player)
{
    if (PAD_BUTTONS(pad_records[player].press, pad_debug_buttons) &&
        debug_unlocked) {
        return TRUE;
    }
    return FALSE;
}

void player_init(int player, struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = heap_calloc(1, sizeof(struct player));
    struct task *bubbles;

    record->consts = character_constants[PLAYER_VARS(work)->character];
    motion->record = record;
    event_alloc(work);
    record->contact = heap_calloc(1, sizeof(struct polygon_ball));
    player_tasks[player] = task;
    player_work[player] = work;
    player_motion[player] = motion;
    player_records[player] = record;
    record->anim.state = PLAYER_ANIM_NEW;
    record->anim.weld_mode = PLAYER_WELD_MAKE;
    record->anim.speed = &record->speed.x;
    record->anim.pace = &record->action_speed;
    record->shadow.scale = record->consts.radius * 0.2F;
    switch (PLAYER_VARS(work)->character) {
    case CHARACTER_SONIC:
        if (event_flag_check(FLAG_SONIC_LIGHT_SHOES)) {
            record->upgrades |= PLAYER_SONIC_LIGHT_SHOES;
        }
        if (event_flag_check(FLAG_SONIC_CRYSTAL_RING)) {
            record->upgrades |= PLAYER_SONIC_CRYSTAL_RING;
        }
        if (event_flag_check(FLAG_SONIC_ANCIENT_LIGHT)) {
            record->upgrades |= PLAYER_SONIC_ANCIENT_LIGHT;
        }
        /* fall through */
    case CHARACTER_TAILS:
        if (event_flag_check(FLAG_TAILS_JET_ANKLET)) {
            record->upgrades |= PLAYER_TAILS_JET_ANKLET;
        }
        if (event_flag_check(FLAG_TAILS_RHYTHM_BROOCH)) {
            record->upgrades |= PLAYER_TAILS_RHYTHM_BROOCH;
        }
        break;
    case CHARACTER_KNUCKLES:
        if (event_flag_check(FLAG_KNUCKLES_SHOVEL_CLAWS)) {
            record->upgrades |= PLAYER_KNUCKLES_SHOVEL_CLAWS;
        }
        if (event_flag_check(FLAG_KNUCKLES_FIGHTING_GLOVES)) {
            record->upgrades |= PLAYER_KNUCKLES_FIGHTING_GLOVES;
        }
        break;
    case CHARACTER_AMY:
        if (event_flag_check(FLAG_AMY_WARRIOR_FEATHER)) {
            record->upgrades |= PLAYER_AMY_WARRIOR_FEATHER;
        }
        if (event_flag_check(FLAG_AMY_LONG_HAMMER)) {
            record->upgrades |= PLAYER_AMY_LONG_HAMMER;
        }
        break;
    case CHARACTER_E102:
        if (event_flag_check(FLAG_E102_BOOSTER)) {
            record->upgrades |= PLAYER_E102_BOOSTER;
        }
        if (event_flag_check(FLAG_E102_LASER)) {
            record->upgrades |= PLAYER_E102_LASER;
        }
        /* fall through */
    case CHARACTER_BIG:
        if (event_flag_check(FLAG_BIG_LIFE_BELT)) {
            record->upgrades |= PLAYER_BIG_LIFE_BELT;
        }
        if (event_flag_check(FLAG_BIG_POWER_ROD)) {
            record->upgrades |= PLAYER_BIG_POWER_ROD;
        }
        break;
    }
    player_clear_speed(NULL, record);
    motion->gravity = 1.0F;
    if (work->id == 0) {
        work->id = PLAYER_ID;
    }
    player_targets_alloc(record);
    work->scl.x = work->scl.y = work->scl.z = 1.0F;
    bubbles = task_create(TASK_WORK, 2, bubbles_start);
    bubbles->work->scl.z = player;
}

void player_free(int player)
{
    struct task_work *work = player_work[player];
    struct task_motion *motion = player_motion[player];
    struct player *record = player_records[player];

    player_tasks[player] = NULL;
    player_work[player] = NULL;
    player_motion[player] = NULL;
    player_records[player] = NULL;
    if (record->contact != NULL) {
        heap_free(record->contact);
        record->contact = NULL;
    }
    record->anim.state = PLAYER_ANIM_FREE;
    if (record->anim.blend != NULL) {
        player_anim_step(&record->anim);
    }
    record->anim.weld_mode = PLAYER_WELD_FREE;
    if (record->anim.welds != NULL) {
        player_update_welds(work, motion, record);
    }
    player_targets_free(record);
    if (motion->record != NULL) {
        heap_free(motion->record);
        motion->record = NULL;
    }
}

/* note where the player is and how it faces, in the next entry of its
 * history */
static void player_note_history(Uint8 player)
{
    struct task_work *work;
    int index;

    work = player_work[player];
    index = ++player_history_index[player];
    player_history_pos[index][player] = work->pos;
    player_history_ang[index][player][0] = work->ang.x;
    player_history_ang[index][player][1] = work->ang.y;
    player_history_ang[index][player][2] = work->ang.z;
}

void player_finish_step(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    NJS_POINT3 eyes;
    struct task *drown;

    work->flags &= ~(PLAYER_REQUEST | COLLISION_TOUCHED);
    motion->accel.x = motion->accel.y = motion->accel.z = 0.0F;
    record->nudge.x = record->nudge.y = record->nudge.z = record->accel.x =
        record->accel.y = record->accel.z = 0.0F;
    motion->slip = 0.0F;
    if (record->launch_steps > 0) {
        record->launch_steps--;
    } else {
        record->launch_steps = 0;
    }
    player_note_history(PLAYER_VARS(work)->player);
    {
        /* @hack spelling: record copied to same, through which the image
         * clears the count (MOV R14,R4 and MOV.W R0,@(18,R4) at
         * 0x8c0701fc); through record, 48 of 103 words differ */
        struct player *same = record;

        same->target_count = 0;
    }
    if (record->shadow.last_above_flags & WORLD_LIQUID) {
        eyes.z = 0.0F;
        eyes.x = 0.0F;
        eyes.y = record->consts.eye_height;
        player_turn_to_world(work, &eyes);
        njAddVector(&eyes, &work->pos);
        if (record->shadow.above >= eyes.y) {
            record->underwater_steps++;
            if (record->underwater_steps == PLAYER_DROWN_STEPS) {
                drown = task_create(TASK_EXTRA, 0, player_drown_step);
                if (drown != NULL) {
                    DROWN(drown)->player = PLAYER_VARS(work)->player;
                }
            }
        }
    } else {
        record->underwater_steps = 0;
    }
}

int player_action_touch_id(struct task_work *work)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        Uint32 flags = work->collision->flags;

        if ((flags & COLLISION_HIT) && !(work->flags & PLAYER_HURT) &&
            (flags & COLLISION_HIT_SIDE)) {
            struct collision_work *hit = work->collision->hit;

            if (hit != NULL) {
                return hit->owner->work->id;
            }
        }
    }
    return 0;
}

/* set flags to the PLAYER_CAN_* bits for a character and a carry kind, or
 * leave it as it was for any other kind or character */
/* @hack spelling: kind copied to kind_copy, which the image stores to the
 * frame and loads back into R6 for the tests (MOV.B R0,@R15 at 0x8c07033c,
 * MOV.B @R15,R6 at 0x8c070340); without it, MOV R0,R6 takes it there, the
 * tests extend it again, and 43 of player_carry_flags' 190 words differ */
#define CARRY_FLAGS(flags, character, kind)                                    \
    {                                                                          \
        Uint8 kind_copy = (kind);                                              \
                                                                               \
        switch (character) {                                                   \
        case CHARACTER_SONIC:                                                  \
        case CHARACTER_TAILS:                                                  \
        case CHARACTER_KNUCKLES:                                               \
        case CHARACTER_AMY:                                                    \
        case CHARACTER_E102:                                                   \
            if (kind_copy == CARRY_KIND_SMALL) {                               \
                (flags) = PLAYER_CAN_CARRY;                                    \
            } else if (kind_copy == CARRY_KIND_MEDIUM) {                       \
                (flags) = PLAYER_CAN_PUSH;                                     \
            } else if (kind_copy == CARRY_KIND_LARGE) {                        \
                (flags) = 0;                                                   \
            } else if (kind_copy == CARRY_KIND_SHAKE) {                        \
                (flags) = PLAYER_CAN_SHAKE;                                    \
            }                                                                  \
            break;                                                             \
        case CHARACTER_BIG:                                                    \
            if (kind_copy == CARRY_KIND_SMALL) {                               \
                (flags) = PLAYER_CAN_CARRY;                                    \
            } else if (kind_copy == CARRY_KIND_MEDIUM) {                       \
                (flags) = PLAYER_CAN_PUSH | PLAYER_CAN_CARRY;                  \
            } else if (kind_copy == CARRY_KIND_LARGE) {                        \
                (flags) = PLAYER_CAN_PUSH | PLAYER_CAN_CARRY;                  \
            } else if (kind_copy == CARRY_KIND_SHAKE) {                        \
                (flags) = PLAYER_CAN_SHAKE;                                    \
            }                                                                  \
            break;                                                             \
        }                                                                      \
    }

int player_carry_flags(struct task_work *work)
{
    struct collision_work *hit;
    struct task_work *other;
    Uint8 kind;
    int flags;
    Sint16 other_flags;
    int result = 0;

    if (work->flags & PLAYER_HOLDING) {
        other = player_records[PLAYER_VARS(work)->player]->held->work;
        if (other == NULL) {
            return 0;
        }
        if (other->collision == NULL) {
            return 0;
        }
        kind = other->id;
        CARRY_FLAGS(result, PLAYER_VARS(work)->character, kind);
    } else {
        flags = work->collision->flags;
        if ((flags & COLLISION_HIT) && !(work->flags & PLAYER_HURT) &&
            (flags & COLLISION_HIT_SIDE)) {
            hit = work->collision->hit;
            if (hit != NULL && (hit->owner->work->flags & CARRY_HOLDABLE)) {
                other = hit->owner->work;
                kind = other->id;
                /* @hack spelling: the flags copied to other_flags, which the
                 * image extends into R6 to test them (EXTS.W R0,R6 at
                 * 0x8c070432); tested in place, 7 of 190 words differ */
                other_flags = other->flags;
                if (other_flags & CARRY_HELD) {
                    return 0;
                }
                CARRY_FLAGS(result, PLAYER_VARS(work)->character, kind);
            }
        }
    }
    return result;
}

Bool player_touches_shakable(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    struct collision_work *hit = work->collision->hit;

    if (hit == NULL || hit->owner->work->id != CARRY_KIND_SHAKE) {
        return FALSE;
    }
    return TRUE;
}

void player_shake_touched(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    NJS_VECTOR push;
    struct player_anim *anim;
    Float frames;
    Float half;
    Float strength;

    if (!player_touches_shakable(work, motion, record)) {
        return;
    }
    if (!collision_get_hit_normal(work, &push)) {
        return;
    }
    push.y = 0.0F;
    if (njScalor(&push) != 0.0F) {
        njUnitVector(&push);
    }
    anim = &record->anim;
    frames = (Float)anim->actions[anim->current].action->motion->nbFrame;
    half = frames * 0.5F;
    if (anim->frame <= half) {
        strength = anim->frame / half;
    } else {
        strength = (frames - anim->frame) / half;
    }
    push.x = -(push.x * strength);
    push.y = -(push.y * strength);
    push.z = -(push.z * strength);
    work->collision->hit->owner->motion->accel = push;
}

void player_approach_touched(struct task_work *work, struct task_motion *motion)
{
    NJS_VECTOR normal;
    NJS_VECTOR offset;
    struct collision_work *hit;
    NJS_POINT3 *center;
    struct task_work *other;
    Float reach;

    collision_get_hit_normal(work, &normal);
    motion->aim.y = angle_approach(
        motion->aim.y, (Angle)(atan2f(-normal.z, -normal.x) * ANGLE_PER_RADIAN),
        0x800);
    work->ang.y = angle_approach(work->ang.y, motion->aim.y, 0xc00);
    hit = work->collision->hit;
    if (hit == NULL) {
        return;
    }
    other = hit->owner->work;
    offset = other->pos;
    center = &work->collision->shapes->center;
    offset.x -= center->x;
    offset.y = 0.0F;
    offset.z -= center->z;
    switch (other->id) {
    case CARRY_KIND_SMALL:
        reach = 2.22F;
        break;
    case CARRY_KIND_MEDIUM:
        reach = 7.5F;
        break;
    case CARRY_KIND_LARGE:
    default:
        reach = 27.5F;
        break;
    }
    reach = reach + work->collision->shapes->radius;
    if (reach > njScalor(&offset)) {
        return;
    }
    work->pos.x -= normal.x;
    work->pos.y -= normal.y;
    work->pos.z -= normal.z;
}

Bool player_upright(struct task_work *work)
{
    NJS_VECTOR up;
    NJS_VECTOR turned;

    up.x = up.z = 0.0F;
    up.y = 1.0F;
    matrix_push_unit();
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    njCalcVector(NULL, &up, &turned);
    njPopMatrix(1);
    njCalcVector(NULL, &turned, &up);
    return up.y > 0.0F;
}

Bool player_read_stick(struct task_work *work, Angle *ang, Float *stroke)
{
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        if (work->event->action != 0) {
            return TRUE;
        }
        return FALSE;
    }
    if (pad_sticks[PLAYER_VARS(work)->player].stroke != 0.0F) {
        if (!(work->flags & PLAYER_ON_PATH) &&
            player_records[PLAYER_VARS(work)->player]->launch_steps != 0) {
            return FALSE;
        }
        if (ang != NULL) {
            *ang = pad_sticks[PLAYER_VARS(work)->player].ang;
        }
        if (stroke != NULL) {
            *stroke = pad_sticks[PLAYER_VARS(work)->player].stroke;
        }
        return TRUE;
    }
    /* curled up on Casinopolis' pinball tables, Sonic rolls on with the
     * stick let go */
    if (PLAYER_VARS(work)->character == CHARACTER_SONIC &&
        (work->flags & PLAYER_BALL) &&
        (stage_get_stage_act() == STAGE_ACT(STAGE_CASINO, 2) ||
         stage_get_stage_act() == STAGE_ACT(STAGE_CASINO, 3))) {
        if (ang != NULL) {
            *ang = work->ang.y;
        }
        if (stroke != NULL) {
            *stroke = 0.0F;
        }
        return TRUE;
    }
    return FALSE;
}

/* the turns from straight into a wall past which a direction is
 * PLAYER_WALL_AWAY, PLAYER_WALL_ACROSS or PLAYER_WALL_ASKEW */
#define TURN_AWAY 0x6000 /* 135 degrees */
#define TURN_ACROSS 0x3400 /* 73.125 degrees */
#define TURN_ASKEW_HEADING 0x1c00 /* 39.375 degrees, for the heading */
#define TURN_ASKEW 0x1400 /* 28.125 degrees, for the stick and the motion */

Bool player_stick_back_or_end_event(struct task_work *work,
                                    struct task_motion *motion)
{
    Angle ang;
    Angle turn;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        if (work->event->action == EVENT_ACTION_DONE) {
            player_clear_speed(NULL, player_records[PLAYER_VARS(work)->player]);
            work->event->action = 0;
            work->flags &= ~PLAYER_EVENT_DRIVEN;
            return TRUE;
        }
    } else if (!(work->flags & PLAYER_ON_PATH)) {
        if (player_read_stick(work, &ang, NULL)) {
            turn = angle_distance(work->ang.y, ang);
            if (turn > TURN_AWAY) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

enum player_jump player_jump_pressed(struct task_work *work,
                                     struct player *record)
{
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return PLAYER_JUMP_NONE;
    }
    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_jump_buttons)) {
        if (panel_jump_ready_or_idle(work) > 0) {
            return PLAYER_JUMP_PANEL;
        }
        return PLAYER_JUMP_PRESSED;
    }
    return PLAYER_JUMP_NONE;
}

Bool player_count_jump_hold(struct task_work *work, struct player *record)
{
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return FALSE;
    }
    if (pad_records[PLAYER_VARS(work)->player].on & pad_jump_buttons) {
        if (record->jump_steps++ < record->consts.jump_hold_steps) {
            return TRUE;
        }
    }
    return FALSE;
}

int player_face_wall(struct task_work *work, struct task_motion *motion)
{
    Angle ang = work->ang.y;
    NJS_VECTOR normal;
    Angle turn;

    collision_get_hit_normal(work, &normal);
    turn = angle_distance(
        ang, (Angle)(atan2f(-normal.z, -normal.x) * ANGLE_PER_RADIAN));
    if (turn > TURN_AWAY) {
        return PLAYER_WALL_AWAY;
    }
    if (turn > TURN_ACROSS) {
        return PLAYER_WALL_ACROSS;
    }
    if (turn > TURN_ASKEW_HEADING) {
        return PLAYER_WALL_ASKEW;
    }
    return PLAYER_WALL_HEAD_ON;
}

int player_stick_into_wall(struct task_work *work, struct task_motion *motion)
{
    Angle ang;
    NJS_VECTOR normal;
    Angle turn;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return PLAYER_WALL_NO_STICK;
    }
    if (!player_read_stick(work, &ang, NULL)) {
        return PLAYER_WALL_NO_STICK;
    }
    collision_get_hit_normal(work, &normal);
    turn = angle_distance(
        ang, (Angle)(atan2f(-normal.z, -normal.x) * ANGLE_PER_RADIAN));
    if (turn > TURN_AWAY) {
        return PLAYER_WALL_AWAY;
    }
    if (turn > TURN_ACROSS) {
        return PLAYER_WALL_ACROSS;
    }
    if (turn > TURN_ASKEW) {
        return PLAYER_WALL_ASKEW;
    }
    return PLAYER_WALL_HEAD_ON;
}

int player_move_into_wall(struct task_work *work, struct task_motion *motion)
{
    NJS_VECTOR normal;
    NJS_VECTOR back;
    Angle ang;

    collision_get_hit_normal(work, &normal);
    back.x = -motion->speed.x;
    back.y = -motion->speed.y;
    back.z = -motion->speed.z;
    if (njScalor(&back) != 0.0F) {
        njUnitVector(&back);
    }
    ang = (Angle)(acosf(njInnerProduct(&normal, &back)) * ANGLE_PER_RADIAN);
    if (ang > TURN_AWAY) {
        return PLAYER_WALL_AWAY;
    }
    if (ang > TURN_ACROSS) {
        return PLAYER_WALL_ACROSS;
    }
    if (ang > TURN_ASKEW) {
        return PLAYER_WALL_ASKEW;
    }
    return PLAYER_WALL_HEAD_ON;
}

Bool player_has_target(Uint8 player)
{
    struct near_task *near;

    if (player == 0) {
        near = near_tasks;
    } else {
        near = near_tasks_player1;
    }
    if (near->work == NULL) {
        return FALSE;
    }
    return TRUE;
}

void player_turn_to_target(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    struct near_task *near;
    struct task_work *nearest;
    Float distance;
    struct collision_shape *own;
    struct collision_shape *other;
    NJS_VECTOR to;
    Angle ang;

    if (PLAYER_VARS(work)->player == 0) {
        near = near_tasks;
    } else {
        near = near_tasks_player1;
    }
    if (near->work == NULL) {
        player_turn_to_stick(work, motion, record);
        return;
    }
    nearest = near->work;
    distance = near->distance;
    for (; near->work != NULL; near++) {
        if (distance > near->distance) {
            distance = near->distance;
            nearest = near->work;
        }
    }
    other = nearest->collision->shapes;
    own = work->collision->shapes;
    to.x = nearest->pos.x + other->center.x - own->center.x;
    to.y = nearest->pos.y + other->center.y - own->center.y;
    to.z = nearest->pos.z + other->center.z - own->center.z;
    ang = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN) & 0xffff;
    motion->aim.y = ang;
    work->ang.y = angle_approach(work->ang.y, ang, 0x1000);
}

void player_accel_ground(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    NJS_VECTOR acc;
    NJS_VECTOR push;
    NJS_VECTOR normal;
    Angle ang;
    Float stroke;
    Bool pushed;
    int off;
    Float x;
    Float y;
    Float z;
    Float grip;
    Float excess;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        player_accel_ground_event(work, motion, record);
        return;
    } else if (record->ground_flags & WORLD_CLING) {
        player_accel_cling(work, motion, record);
        return;
    } else if (record->ground_flags & WORLD_NO_SLOPE) {
        player_accel_level(work, motion, record);
        return;
    }

    acc = motion->accel;
    push.x = push.y = push.z = 0.0F;
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    pushed = player_read_stick(work, &ang, &stroke);
    njOuterProduct(&record->contact->normal, &motion->speed, &normal);

    if (record->level < 0.1F && _builtin_fabsf(normal.y) > 0.6F &&
        record->speed.x > 1.16F) {
        acc.x = acc.y = acc.z = 0.0F;
        acc.y -= record->consts.gravity * 1.0F;
    } else if (record->level < -0.4F && record->speed.x > 1.16F) {
        acc.y -= record->consts.gravity * 5.0F;
    } else if (record->level < -0.3F && record->speed.x > 1.16F) {
        acc.y -= record->consts.gravity * 0.8F;
    } else if (record->level < -0.1F && record->speed.x > 1.16F) {
        acc.y -= record->consts.gravity * 0.4F;
    } else if (record->level < 0.5F &&
               record->speed.x < record->consts.running_speed &&
               record->speed.x > -record->consts.running_speed) {
        acc.x *= 4.225F;
        acc.z *= 4.225F;
    } else if (record->level < 0.7F &&
               record->speed.x < record->consts.running_speed &&
               record->speed.x > -record->consts.running_speed) {
        acc.x *= 1.0F;
        acc.z *= 2.0F;
    } else if (record->level < 0.87F &&
               record->speed.x < record->consts.trot_speed &&
               record->speed.x > -record->consts.running_speed) {
        acc.x *= 1.0F;
        acc.z *= 1.4F;
    }

    if (!pushed && record->speed.x > record->consts.running_speed) {
        acc.x += record->consts.resist.x * record->speed.x;
    } else if (pushed && record->speed.x > record->consts.top_speed &&
               record->level > 0.96F) {
        acc.x += 1.7F * (record->consts.resist.x *
                         (record->speed.x - record->consts.top_speed));
    } else if (record->speed.x > record->consts.top_speed) {
        acc.x += record->consts.resist.x *
                 (record->speed.x - record->consts.top_speed);
    } else if (record->speed.x < 0.0F) {
        acc.x += record->consts.resist.x * record->speed.x;
    }
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;

    if (!pushed) {
        if (record->level < 0.71F) {
            player_roll_downhill(work, motion, record);
        } else if (record->speed.x > 0.0F) {
            push.x = record->consts.ground_drag;
        } else if (record->speed.x < 0.0F) {
            push.x = -record->consts.ground_drag;
        }
        motion->drive = 0.0F;
    } else {
        if (work->flags & PLAYER_ON_PATH) {
            player_follow_path(work, motion, record);
            ang = motion->aim.y;
            push.y -= 0.8F;
        } else if (record->level < 0.94F && (record->ground_flags & 0x800000)) {
            player_steer_downhill(work, motion, record);
        }
        if (record->speed.x < record->consts.top_speed) {
            if (record->speed.x < record->consts.trot_speed) {
                if (stroke > 0.5F) {
                    stroke *= record->consts.ground_accel;
                } else if (record->speed.x < record->consts.trot_speed * 0.4F) {
                    stroke *= record->consts.ground_accel;
                } else {
                    stroke = 0.0F;
                }
            } else if (record->speed.x < record->consts.running_speed) {
                if (stroke > 0.7F) {
                    stroke *= record->consts.ground_accel;
                } else if (record->speed.x < (record->consts.running_speed +
                                              record->consts.running_speed) *
                                                 0.5F) {
                    stroke *= record->consts.ground_accel;
                } else {
                    stroke = 0.0F;
                }
            } else if (record->speed.x < record->consts.sprint_speed) {
                if (stroke > 0.9F) {
                    stroke *= record->consts.ground_accel;
                } else {
                    stroke *= record->consts.ground_accel * 0.3F;
                }
            } else {
                stroke *= record->consts.ground_accel;
            }
        } else {
            if (record->speed.x >= record->consts.top_speed &&
                record->level < 0.0F) {
                stroke *= record->consts.ground_accel;
            } else {
                stroke *= record->consts.ground_accel * 0.4F;
            }
        }

        off = angle_distance(work->ang.y, ang);
        if (record->speed.x == 0.0F && off > 0x1000) {
            motion->aim.y = ang;
            stroke = 0.0F;
            player_turn_on_course(work, motion, record, motion->aim.y);
        } else if (record->speed.x >= (record->consts.trot_speed +
                                       record->consts.running_speed) *
                                          0.5F &&
                   off > 0x1000) {
            motion->aim.y = ang;
            stroke = record->consts.ground_drag;
            player_turn_toward(work, motion, record, motion->aim.y);
        } else if (record->speed.x >= record->consts.trot_speed &&
                   off < 0x1000) {
            motion->aim.y = ang;
            player_turn_at_speed(work, motion, record, motion->aim.y);
        } else if (record->speed.x >= record->consts.full_speed &&
                   (record->ground_flags & 0x800000)) {
            motion->aim.y = ang;
            player_turn_at_speed(work, motion, record, motion->aim.y);
        } else if (record->consts.trot_speed <= record->speed.x &&
                   record->speed.x <= record->consts.sprint_speed &&
                   off > 0x2000) {
            motion->aim.y = ang;
            stroke *= 0.8F;
            player_turn_toward(work, motion, record, motion->aim.y);
        } else {
            motion->aim.y = ang;
            player_turn_toward(work, motion, record, motion->aim.y);
        }
        motion->drive = stroke;
        push.x = stroke;
    }

    x = acc.x;
    y = acc.y;
    if (record->level < 0.71F && record->speed.x < record->consts.trot_speed &&
        record->speed.x > -record->consts.trot_speed && !pushed) {
        acc.x = x * 10.0F;
        acc.z *= 10.0F;
    } else if (record->speed.x == 0.0F) {
        grip = y * record->consts.still_grip * motion->gravity;
        acc.x += push.x;
        if (!pushed && grip > acc.x && acc.x > -grip) {
            acc.x = 0.0F;
        }
        if (!pushed && acc.x < 0.051F && acc.x > -0.051F) {
            acc.x = 0.0F;
        }
    } else if (push.x < 0.0F) {
        if (record->ground_flags & WORLD_GRIP_LEAST) {
            if (y < 0.0F) {
                grip = y * record->consts.moving_grip * motion->gravity;
            } else {
                grip = 0.0F;
            }
            if (grip > 0.0F && -push.x > grip) {
                x = push.x + x;
                excess = x - grip;
                if (x < 0.0F && -grip > x) {
                    motion->slip = excess;
                    x = -grip;
                } else if (x > 0.0F && x > grip) {
                    motion->slip = excess;
                    x = grip;
                }
                acc.x = x;
            } else if (!pushed &&
                       record->speed.x <= record->consts.trot_speed &&
                       x < 0.051F && x > -0.051F) {
                acc.x = 0.0F;
            } else {
                acc.x += push.x;
            }
        } else {
            if (record->speed.x > 0.0F) {
                x += push.x;
                if (push.x * x < 0.0F) {
                    x = 0.0F;
                }
            } else if (!pushed &&
                       record->speed.x <= record->consts.trot_speed &&
                       x < 0.051F && x > -0.051F) {
                x = 0.0F;
            } else {
                x = y * record->consts.still_grip * motion->gravity;
                x += push.x;
                if (x >= 0.0F) {
                    x = 0.0F;
                }
            }
            acc.x = x;
        }
    } else {
        grip = y * record->consts.still_grip * motion->gravity;
        if (record->speed.x < 0.0F && x < 0.051F && x > -0.051F) {
            acc.x += push.x;
            if (acc.x * push.x < 0.0F) {
                acc.x = 0.0F;
            }
        } else if (grip > 0.0F && push.x > grip) {
            x = push.x + x;
            if (x > grip) {
                x -= grip;
                motion->slip = x;
                x = grip;
            }
            acc.x = x;
        } else if (!pushed && record->speed.x <= record->consts.trot_speed &&
                   x < 0.051F && x > -0.051F) {
            acc.x = 0.0F;
        } else {
            acc.x += push.x;
        }
    }

    y = acc.y;
    if (record->speed.z == 0.0F) {
        grip = y * record->consts.still_grip * motion->gravity;
        if (grip > acc.z && acc.z > -grip) {
            acc.z = 0.0F;
        }
    } else {
        if (y < 0.0F) {
            grip = y * record->consts.side_grip * motion->gravity;
        } else {
            grip = 0.0F;
        }
        if (record->ground_flags & WORLD_GRIP_LEAST) {
            z = push.z + acc.z;
            if (z > 0.0F) {
                z -= grip;
                if (z < 0.0F) {
                    z = 0.0F;
                }
            } else if (z < 0.0F) {
                z += grip;
                if (z > 0.0F) {
                    z = 0.0F;
                }
            }
            acc.z = z;
        } else {
            if (acc.z > 0.0F) {
                push.z -= grip;
            } else if (acc.z < 0.0F) {
                push.z += grip;
            }
            z = acc.z + push.z;
            if (acc.z != 0.0F && push.z != 0.0F && acc.z * z < 0.0F) {
                z = 0.0F;
            }
            acc.z = z;
        }
    }
    acc.y += push.y;
    /* @hack redundant: z, set and never read, whose read of push keeps work
     * in R11 and push's address in R12 (0x8c070dc8, 0x8c070e80); without
     * it, the two trade and 35 of 1089 words differ */
    z = push.z;
    record->accel = acc;
}

static void player_accel_cling(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    NJS_VECTOR acc;
    NJS_VECTOR push;
    NJS_VECTOR forward;
    NJS_VECTOR axis;
    NJS_VECTOR heading;
    Angle stick;
    Angle turn;
    Angle ang;
    Float stroke;
    Bool pushed;
    int off;
    Float x;
    Float y;
    Float z;
    Float grip;
    Float excess;

    acc = motion->accel;
    push.x = push.y = push.z = 0.0F;
    acc.x -= record->ground_normal.x * record->consts.gravity;
    acc.y -= record->ground_normal.y * record->consts.gravity;
    acc.z -= record->ground_normal.z * record->consts.gravity;
    player_turn_back(work, &acc);
    pushed = player_read_stick(work, &ang, &stroke);

    if (!pushed && record->speed.x > record->consts.running_speed) {
        acc.x += record->consts.resist.x * record->speed.x;
    } else if (pushed && record->speed.x > record->consts.top_speed &&
               record->level > 0.96F) {
        acc.x += 1.7F * (record->consts.resist.x *
                         (record->speed.x - record->consts.top_speed));
    } else if (record->speed.x > record->consts.top_speed) {
        acc.x += record->consts.resist.x *
                 (record->speed.x - record->consts.top_speed);
    } else if (record->speed.x < 0.0F) {
        acc.x += record->consts.resist.x * record->speed.x;
    }
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;

    if (!pushed) {
        if (record->level < 0.71F) {
            player_roll_downhill(work, motion, record);
        } else if (record->speed.x > 0.0F) {
            push.x = record->consts.ground_drag;
        } else if (record->speed.x < 0.0F) {
            push.x = -record->consts.ground_drag;
        }
        motion->drive = 0.0F;
    } else {
        if (work->flags & PLAYER_ON_PATH) {
            player_follow_path(work, motion, record);
            ang = motion->aim.y;
            push.y -= 0.8F;
        }
        if (record->speed.x < record->consts.top_speed) {
            if (record->speed.x < record->consts.trot_speed) {
                if (stroke > 0.5F) {
                    stroke *= record->consts.ground_accel;
                } else if (record->speed.x < record->consts.trot_speed * 0.4F) {
                    stroke *= record->consts.ground_accel;
                } else {
                    stroke = 0.0F;
                }
            } else if (record->speed.x < record->consts.running_speed) {
                if (stroke > 0.7F) {
                    stroke *= record->consts.ground_accel;
                } else if (record->speed.x < (record->consts.running_speed +
                                              record->consts.running_speed) *
                                                 0.5F) {
                    stroke *= record->consts.ground_accel;
                } else {
                    stroke = 0.0F;
                }
            } else if (record->speed.x < record->consts.sprint_speed) {
                if (stroke > 0.9F) {
                    stroke *= record->consts.ground_accel;
                } else {
                    stroke *= record->consts.ground_accel * 0.3F;
                }
            } else {
                stroke *= record->consts.ground_accel;
            }
        } else {
            if (record->speed.x >= record->consts.top_speed &&
                record->level < 0.0F) {
                stroke *= record->consts.ground_accel;
            } else {
                stroke *= record->consts.ground_accel * 0.4F;
            }
        }

        off = angle_distance(work->ang.y, ang);
        if (record->speed.x == 0.0F && off > 0x1000) {
            motion->aim.y = ang;
            stroke = 0.0F;
            player_turn_on_course(work, motion, record, motion->aim.y);
        } else {
            stick =
                (Angle)(atan2f((Float)(pad_records[PLAYER_VARS(work)->player].x1
                                       << 8),
                               (Float)(pad_records[PLAYER_VARS(work)->player].y1
                                       << 8)) *
                        ANGLE_PER_RADIAN);
            axis.x = 0.0F;
            axis.y = 0.0F;
            axis.z = -1.0F;
            matrix_push_unit();
            TURN(njRotateY, camera_work->ang.y);
            TURN(njRotateX, camera_work->ang.x);
            njCalcVector(NULL, &axis, &forward);
            njPopMatrix(1);
            matrix_push_unit();
            njRotate(NULL, &record->ground_normal, stick);
            njCalcVector(NULL, &forward, &heading);
            njPopMatrix(1);
            axis.x = 1.0F;
            axis.y = 0.0F;
            axis.z = 0.0F;
            player_turn_to_world(work, &axis);
            turn = vector_angle(&axis, &heading, &record->ground_normal);
            motion->aim.y = (Uint16)(work->ang.y + turn);
            player_turn_at_speed(work, motion, record, motion->aim.y);
        }
        motion->drive = stroke;
        push.x = stroke;
    }

    y = acc.y;
    if (record->speed.x == 0.0F) {
        grip = y * record->consts.still_grip * motion->gravity;
        acc.x += push.x;
        if (!pushed && grip > acc.x && acc.x > -grip) {
            acc.x = 0.0F;
        }
        if (!pushed && acc.x < 0.051F && acc.x > -0.051F) {
            acc.x = 0.0F;
        }
    } else {
        x = acc.x;
        if (push.x < 0.0F) {
            if (record->ground_flags & WORLD_GRIP_LEAST) {
                if (y < 0.0F) {
                    grip = y * record->consts.moving_grip * motion->gravity;
                } else {
                    grip = 0.0F;
                }
                if (grip > 0.0F && -push.x > grip) {
                    x = push.x + x;
                    excess = x - grip;
                    if (x < 0.0F && -grip > x) {
                        motion->slip = excess;
                        x = -grip;
                    } else if (x > 0.0F && x > grip) {
                        motion->slip = excess;
                        x = grip;
                    }
                    acc.x = x;
                } else if (!pushed &&
                           record->speed.x <= record->consts.trot_speed &&
                           x < 0.051F && x > -0.051F) {
                    acc.x = 0.0F;
                } else {
                    acc.x += push.x;
                }
            } else {
                if (record->speed.x > 0.0F) {
                    x += push.x;
                    if (push.x * x < 0.0F) {
                        x = 0.0F;
                    }
                } else if (!pushed &&
                           record->speed.x <= record->consts.trot_speed &&
                           x < 0.051F && x > -0.051F) {
                    x = 0.0F;
                } else {
                    x = y * record->consts.still_grip * motion->gravity;
                    x += push.x;
                    if (x >= 0.0F) {
                        x = 0.0F;
                    }
                }
                acc.x = x;
            }
        } else {
            grip = y * record->consts.still_grip * motion->gravity;
            if (record->speed.x < 0.0F && x < 0.051F && x > -0.051F) {
                acc.x += push.x;
                if (acc.x * push.x < 0.0F) {
                    acc.x = 0.0F;
                }
            } else if (grip > 0.0F && push.x > grip) {
                x = push.x + x;
                if (x > grip) {
                    x -= grip;
                    motion->slip = x;
                    x = grip;
                }
                acc.x = x;
            } else if (!pushed &&
                       record->speed.x <= record->consts.trot_speed &&
                       x < 0.051F && x > -0.051F) {
                acc.x = 0.0F;
            } else {
                acc.x += push.x;
            }
        }
    }

    y = acc.y;
    if (record->speed.z == 0.0F) {
        grip = y * record->consts.still_grip * motion->gravity;
        if (grip > acc.z && acc.z > -grip) {
            acc.z = 0.0F;
        }
    } else {
        if (y < 0.0F) {
            grip = y * record->consts.side_grip * motion->gravity;
        } else {
            grip = 0.0F;
        }
        if (record->ground_flags & WORLD_GRIP_LEAST) {
            z = push.z + acc.z;
            if (z > 0.0F) {
                z -= grip;
                if (z < 0.0F) {
                    z = 0.0F;
                }
            } else if (z < 0.0F) {
                z += grip;
                if (z > 0.0F) {
                    z = 0.0F;
                }
            }
            acc.z = z;
        } else {
            if (acc.z > 0.0F) {
                push.z -= grip;
            } else if (acc.z < 0.0F) {
                push.z += grip;
            }
            z = acc.z + push.z;
            if (acc.z != 0.0F && push.z != 0.0F && acc.z * z < 0.0F) {
                z = 0.0F;
            }
            acc.z = z;
        }
    }
    acc.y += push.y;
    record->accel = acc;
}

static void player_accel_level(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    NJS_VECTOR acc;
    NJS_VECTOR push;
    Angle ang;
    Float stroke;
    Bool pushed;
    int off;
    Float x;
    Float y;
    Float z;
    Float sum;
    Float grip;

    acc = motion->accel;
    push.x = push.y = push.z = 0.0F;
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    pushed = player_read_stick(work, &ang, &stroke);

    if (!pushed && record->speed.x > record->consts.running_speed) {
        acc.x += record->consts.resist.x * record->speed.x;
    } else if (record->speed.x > record->consts.sprint_speed) {
        acc.x += record->consts.resist.x *
                 (record->speed.x - record->consts.sprint_speed);
    } else if (record->speed.x < 0.0F) {
        acc.x += record->consts.resist.x * record->speed.x;
    }
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;

    if (!pushed) {
        if (record->speed.x > 0.0F) {
            push.x = record->consts.ground_drag;
        } else if (record->speed.x < 0.0F) {
            push.x = -record->consts.ground_drag;
        }
        motion->drive = 0.0F;
    } else {
        if (record->consts.running_speed > record->speed.x) {
            stroke *= record->consts.ground_accel;
        } else {
            stroke *= record->consts.ground_accel * 0.4F;
        }

        off = angle_distance(work->ang.y, ang);
        if (record->speed.x == 0.0F && off > 0x1000) {
            motion->aim.y = ang;
            stroke = 0.0F;
            player_turn_on_course(work, motion, record, motion->aim.y);
        } else if (record->speed.x >= (record->consts.trot_speed +
                                       record->consts.running_speed) *
                                          0.5F &&
                   off > 0x1000) {
            motion->aim.y = ang;
            stroke = record->consts.ground_drag;
            player_turn_toward(work, motion, record, motion->aim.y);
        } else if (record->speed.x >= record->consts.trot_speed &&
                   off < 0x1000) {
            motion->aim.y = ang;
            player_turn_at_speed(work, motion, record, motion->aim.y);
        } else if (record->consts.trot_speed <= record->speed.x &&
                   record->speed.x <= record->consts.sprint_speed &&
                   off > 0x2000) {
            motion->aim.y = ang;
            stroke *= 0.8F;
            player_turn_toward(work, motion, record, motion->aim.y);
        } else {
            motion->aim.y = ang;
            player_turn_toward(work, motion, record, motion->aim.y);
        }
        motion->drive = stroke;
        push.x = stroke;
    }

    y = acc.y;
    if (record->speed.x == 0.0F) {
        grip = y * record->consts.still_grip * motion->gravity;
        acc.x += push.x;
        if (!pushed && grip > acc.x && acc.x > -grip) {
            acc.x = 0.0F;
        }
    } else {
        x = acc.x;
        if (push.x < 0.0F) {
            if (record->speed.x > 0.0F) {
                sum = x + push.x;
                if (push.x * sum < 0.0F) {
                    sum = 0.0F;
                }
            } else {
                sum = y * record->consts.still_grip * motion->gravity;
                sum += push.x + x;
                if (sum >= 0.0F) {
                    sum = 0.0F;
                }
            }
            acc.x = sum;
        } else {
            grip = y * record->consts.still_grip * motion->gravity;
            if (record->speed.x < 0.0F) {
                acc.x += push.x;
                if (acc.x * push.x < 0.0F) {
                    acc.x = 0.0F;
                }
            } else if (grip > 0.0F && push.x > grip) {
                sum = push.x + x;
                if (sum > grip) {
                    sum -= grip;
                    motion->slip = sum;
                    sum = grip;
                }
                acc.x = sum;
            } else {
                acc.x += push.x;
            }
        }
    }

    y = acc.y;
    if (record->speed.z == 0.0F) {
        grip = y * record->consts.still_grip * motion->gravity;
        if (grip > acc.z && acc.z > -grip) {
            acc.z = 0.0F;
        }
    } else {
        if (y < 0.0F) {
            grip = y * record->consts.side_grip * motion->gravity;
        } else {
            grip = 0.0F;
        }
        if (acc.z > 0.0F) {
            push.z -= grip;
        } else if (acc.z < 0.0F) {
            push.z += grip;
        }
        z = acc.z + push.z;
        if (acc.z != 0.0F && push.z != 0.0F && acc.z * z < 0.0F) {
            z = 0.0F;
        }
        acc.z = z;
    }
    acc.y += push.y;
    record->accel = acc;
}

void player_accel_air(struct task_work *work, struct task_motion *motion,
                      struct player *record)
{
    Angle ang;
    Float stroke;
    NJS_VECTOR acc;
    Angle diff;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        player_accel_air_event(work, motion, record);
        return;
    }
    acc = motion->accel;
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    acc.x += record->consts.air_resist * record->speed.x;
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;
    if (player_read_stick(work, &ang, &stroke) == FALSE) {
        stroke = 0.0F;
        motion->aim.y = work->ang.y;
    } else {
        diff = angle_distance(work->ang.y, ang);
        if (record->speed.x > record->consts.running_speed && diff > 0x6000) {
            stroke = record->consts.air_brake * stroke;
        } else if (diff > 0x1000) {
            motion->aim.y = ang;
            stroke = 0.0F;
        } else {
            motion->aim.y = ang;
            if (motion->speed.y < 0.0F) {
                stroke = record->consts.air_accel * stroke * 2.0F;
            } else {
                stroke = record->consts.air_accel * stroke;
            }
        }
        player_turn_toward(work, motion, record, motion->aim.y);
    }
    acc.x += stroke;
    record->accel = acc;
}

static void player_accel_ground_event(struct task_work *work,
                                      struct task_motion *motion,
                                      struct player *record)
{
    NJS_VECTOR push;
    NJS_VECTOR acc;
    NJS_VECTOR to;
    struct event_work *event;
    Float dist;
    Float speed;
    Float accel;
    Float limit;
    Float x;
    Float sum;
    Float friction;
    Float length;

    event = work->event;
    acc = motion->accel;
    push.x = push.y = push.z = 0.0F;
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    switch (event->action) {
    case EVENT_ACTION_RUN:
        to = event->move.pos;
        njSubVector(&to, &work->pos);
        to.y = 0.0F;
        dist = njScalor(&to);
        if (!(dist > 1.0F)) {
            player_clear_speed(NULL, record);
            event->action = EVENT_ACTION_DONE;
            break;
        }
        motion->aim.y = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN) & 0xffff;
        speed = event->speed;
        if (speed == 0.0F) {
            speed = record->consts.running_speed;
        }
        accel = event->accel;
        if (accel == 0.0F) {
            accel = record->consts.ground_accel;
        }
        if ((push.x = speed - record->speed.x) > 0.0F) {
            if (push.x > accel) {
                push.x = accel;
            }
        } else {
            push.x = 0.0F;
        }
        break;
    case EVENT_ACTION_MOVE:
        to = event->move.pos;
        to.y = 0.0F;
        length = njScalor(&to);
        if (length == 0.0F) {
            player_clear_speed(NULL, record);
            event->action = EVENT_ACTION_DONE;
            break;
        }
        motion->aim.y = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN) & 0xffff;
        if ((push.x = length - record->speed.x) > 0.0F) {
            if (push.x > record->consts.ground_accel) {
                push.x = record->consts.ground_accel;
            }
        } else {
            push.x = 0.0F;
        }
        break;
    case EVENT_ACTION_PUSH:
        to = event->move.pos;
        to.y = 0.0F;
        dist = njScalor(&to);
        if (dist == 0.0F) {
            player_clear_speed(NULL, record);
            event->action = EVENT_ACTION_DONE;
            break;
        }
        motion->aim.y = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN) & 0xffff;
        njAddVector(&acc, &to);
        break;
    case EVENT_ACTION_DONE:
        break;
    default:
        work->flags &= ~PLAYER_EVENT_DRIVEN;
        break;
    }
    player_turn_back(work, &acc);
    if (record->speed.x > record->consts.top_speed) {
        acc.x += record->consts.resist.x *
                 (record->speed.x - record->consts.top_speed);
    } else if (record->speed.x < 0.0F) {
        acc.x += record->consts.resist.x * record->speed.x;
    }
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;
    player_turn_on_course(work, motion, record, motion->aim.y);
    if (record->speed.x == 0.0F) {
        limit = acc.y * record->consts.still_grip * motion->gravity;
        acc.x += push.x;
        if (push.x == 0.0F && limit > acc.x && acc.x > -limit) {
            acc.x = 0.0F;
        }
        if (push.x == 0.0F && acc.x < 0.051F && acc.x > -0.051F) {
            acc.x = 0.0F;
        }
    } else if (push.x < 0.0F) {
        if (record->speed.x > 0.0F) {
            x = acc.x + push.x;
            if (push.x * x < 0.0F) {
                x = 0.0F;
            }
        } else if (push.x == 0.0F &&
                   record->speed.x <= record->consts.trot_speed &&
                   acc.x < 0.051F && acc.x > -0.051F) {
            x = 0.0F;
        } else {
            x = acc.y * record->consts.still_grip * motion->gravity;
            x += push.x;
            if (x >= 0.0F) {
                x = 0.0F;
            }
        }
        acc.x = x;
    } else {
        limit = acc.y * record->consts.still_grip * motion->gravity;
        if (record->speed.x < 0.0F && acc.x < 0.051F && acc.x > -0.051F) {
            acc.x += push.x;
            if (acc.x * push.x < 0.0F) {
                acc.x = 0.0F;
            }
        } else if (limit > 0.0F && push.x > limit) {
            x = push.x + acc.x;
            if (x > limit) {
                motion->slip = x - limit;
                x = limit;
            }
            acc.x = x;
        } else if (push.x == 0.0F &&
                   record->speed.x <= record->consts.trot_speed &&
                   acc.x < 0.051F && acc.x > -0.051F) {
            acc.x = 0.0F;
        } else {
            acc.x += push.x;
        }
    }
    if (record->speed.z == 0.0F) {
        friction = acc.y * record->consts.still_grip * motion->gravity;
        if (friction > acc.z && acc.z > -friction) {
            acc.z = 0.0F;
        }
    } else {
        if (acc.y < 0.0F) {
            friction = acc.y * record->consts.side_grip * motion->gravity;
        } else {
            friction = 0.0F;
        }
        if (acc.z > 0.0F) {
            push.z -= friction;
        } else if (acc.z < 0.0F) {
            push.z += friction;
        }
        sum = acc.z + push.z;
        if (acc.z != 0.0F && push.z != 0.0F && acc.z * sum < 0.0F) {
            sum = 0.0F;
        }
        acc.z = sum;
    }
    record->accel = acc;
}

static void player_accel_air_event(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record)
{
    NJS_VECTOR push;
    NJS_VECTOR acc;
    NJS_VECTOR to;
    struct event_work *event;
    Float dist;

    event = work->event;
    acc = motion->accel;
    push.x = push.y = push.z = 0.0F;
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    switch (event->action) {
    case EVENT_ACTION_RUN:
        to = event->move.pos;
        njSubVector(&to, &work->pos);
        to.y = 0.0F;
        dist = njScalor(&to);
        if (!(dist > 1.0F)) {
            player_clear_speed(NULL, record);
            event->action = EVENT_ACTION_DONE;
            break;
        }
        motion->aim.y = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN) & 0xffff;
        push.x = record->consts.running_speed - record->speed.x;
        if (push.x > 0.0F) {
            if (push.x > record->consts.ground_accel) {
                push.x = record->consts.ground_accel;
            }
        } else {
            push.x = 0.0F;
        }
        break;
    case EVENT_ACTION_MOVE:
        to = event->move.pos;
        to.y = 0.0F;
        dist = njScalor(&to);
        if (dist == 0.0F) {
            player_clear_speed(NULL, record);
            event->action = EVENT_ACTION_DONE;
            break;
        }
        motion->aim.y = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN) & 0xffff;
        if ((push.x = dist - record->speed.x) > 0.0F) {
            if (push.x > record->consts.ground_accel) {
                push.x = record->consts.ground_accel;
            }
        } else {
            push.x = 0.0F;
        }
        break;
    case EVENT_ACTION_PUSH:
        to = event->move.pos;
        to.y = 0.0F;
        dist = njScalor(&to);
        if (dist == 0.0F) {
            player_clear_speed(NULL, record);
            event->action = EVENT_ACTION_DONE;
            break;
        }
        motion->aim.y = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN) & 0xffff;
        njAddVector(&acc, &to);
        break;
    case EVENT_ACTION_DONE:
        break;
    default:
        work->flags &= ~PLAYER_EVENT_DRIVEN;
        break;
    }
    player_turn_back(work, &acc);
    acc.x += record->consts.air_resist * record->speed.x;
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;
    player_turn_on_course(work, motion, record, motion->aim.y);
    acc.x += push.x;
    record->accel = acc;
}

void player_accel_path(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    Float stroke;
    NJS_VECTOR acc;
    Bool pushed;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    acc.x = acc.y = acc.z = 0.0F;
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    acc.x += record->consts.resist.x * record->speed.x;
    if ((pushed = player_read_stick(work, NULL, &stroke)) == FALSE) {
        stroke = record->consts.ground_drag;
    } else {
        stroke = record->consts.ground_accel * stroke;
    }
    acc.x += stroke;
    if (record->speed.x == 0.0F) {
        Float grip = acc.y * record->consts.still_grip;

        if (!pushed && grip > acc.x && acc.x > -grip) {
            acc.x = 0.0F;
        }
    }
    record->accel = acc;
}

void player_accel_push(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    NJS_VECTOR acc;
    Float stroke;
    int wall;
    int way; /* 1 pushes, -1 pulls, and 0 lets go */

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    wall = player_stick_into_wall(work, motion);
    if (wall >= PLAYER_WALL_HEAD_ON) {
        way = 1;
    } else if (wall == PLAYER_WALL_AWAY) {
        way = -1;
    } else {
        way = 0;
    }
    collision_get_hit_normal(work, &acc);
    motion->aim.y = (Angle)(atan2f(-acc.z, -acc.x) * ANGLE_PER_RADIAN) & 0xffff;
    work->ang.y = motion->aim.y;
    acc.x = acc.y = acc.z = 0.0F;
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    acc.x += record->consts.resist.x * record->speed.x;
    if (way == 0) {
        stroke = record->consts.ground_drag;
    } else {
        player_read_stick(work, NULL, &stroke);
        stroke = record->consts.ground_accel * stroke;
    }
    acc.x += stroke;
    if (record->speed.x == 0.0F) {
        Float grip = acc.y * record->consts.still_grip;

        if (way == 0 && grip > acc.x && acc.x > -grip) {
            acc.x = 0.0F;
        }
    }
    if (way > 0) {
        record->accel.x = acc.x;
    } else if (way < 0) {
        record->accel.x = -acc.x;
    } else {
        acc.x = _builtin_fabsf(acc.x);
        if (record->speed.x > 0.0F) {
            record->accel.x = -acc.x;
        } else if (record->speed.x < 0.0F) {
            record->accel.x = acc.x;
        }
    }
}

/* move the player along its path by its speed, which the stick brings up
 * to full_speed, and turn it the way the path runs. the player leaves the
 * path where path_sample finds no point. it leaves it too where the way to
 * that point, or its own motion, flattens to nothing on the ground's
 * plane */
static void player_follow_path(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    struct path_sample sample;
    NJS_VECTOR ahead;
    NJS_VECTOR moving;
    NJS_VECTOR to;
    NJS_PLANE plane;
    Angle ang;

    if (record->speed.x < record->consts.full_speed &&
        player_read_stick(work, NULL, NULL)) {
        record->speed.x = record->consts.full_speed;
    }
    record->path_distance += record->speed.x;
    sample.distance = record->path_distance;
    if (path_sample(PLAYER_VARS(work)->arg.path, &sample)) {
        plane.vx = record->contact->normal.x;
        plane.vy = record->contact->normal.y;
        plane.vz = record->contact->normal.z;
        plane.px = plane.py = plane.pz = 0.0F;
        to.x = sample.pos.x - work->pos.x;
        to.y = sample.pos.y - work->pos.y;
        to.z = sample.pos.z - work->pos.z;
        njDistanceP2PL(&to, &plane, &ahead);
        to = motion->speed;
        njDistanceP2PL(&to, &plane, &moving);
        if (njScalor(&moving) == 0.0F || njScalor(&ahead) == 0.0F) {
            work->flags &= ~PLAYER_ON_PATH;
            return;
        }
        njUnitVector(&ahead);
        njUnitVector(&moving);
        ang = vector_angle(&ahead, &moving, &record->contact->normal);
        ang = (Uint16)(ang + work->ang.y);
        if (angle_distance(ang, motion->aim.y) > 0x4000) {
            ang = (Uint16)(0x8000 - ang);
        }
        motion->aim.y = ang;
        work->ang.y = ang;
    } else {
        work->flags &= ~PLAYER_ON_PATH;
    }
}

void player_accel_fly(struct task_work *work, struct task_motion *motion,
                      struct player *record)
{
    Float stroke;
    Angle ang;
    NJS_VECTOR acc;

    acc = motion->accel;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    acc.x += record->consts.air_resist * record->speed.x;
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;
    if (player_read_stick(work, &ang, &stroke) == FALSE) {
        stroke = 0.0F;
        motion->aim.y = work->ang.y;
    } else {
        Angle off = angle_distance(work->ang.y, ang);

        if (record->speed.x > record->consts.running_speed && off > 0x6000) {
            stroke = record->consts.air_brake * stroke;
        } else if (off > 0x1000) {
            motion->aim.y = ang;
            stroke = 0.0F;
        } else {
            motion->aim.y = ang;
            stroke *= 0.1F;
            if (record->upgrades & PLAYER_TAILS_JET_ANKLET) {
                stroke *= 1.5F;
            }
        }
        player_turn_toward(work, motion, record, motion->aim.y);
    }
    acc.x += stroke;
    record->accel = acc;
}

void player_accel_drive(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    NJS_VECTOR acc;
    Float across;
    Float down;
    Float up;
    Float push;
    Angle ang;

    acc = motion->accel;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    acc.x += 0.5F * (record->consts.resist.x * record->speed.x);
    acc.y += 0.5F * (record->consts.resist.y * record->speed.y);
    acc.z += 0.5F * (record->consts.resist.z * record->speed.z);
    if (!player_read_stick(work, NULL, NULL)) {
        push = 0.0F;
        motion->aim.y = work->ang.y;
    } else {
        /* @hack redundant: across is clamped at 0 and never read, but the
         * image computes it (FLOAT at 0x8c073916, FCMP/GT at 0x8c073922);
         * without it, 24 of 168 words differ */
        across = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
        down = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
        if (across < 0.0F) {
            across = 0.0F;
        }
        if (down >= 0.0F) {
            down -= PAD_DEAD_ZONE;
        } else {
            down += PAD_DEAD_ZONE;
        }
        ang = work->ang.y +
              ((pad_records[PLAYER_VARS(work)->player].x1 << 8) >> 6);
        up = down;
        if (down > 0.0F) {
            up = 0.0F;
        }
        push = (up * up) * (1.0F / (PAD_STICK_REACH * PAD_STICK_REACH));
        motion->aim.y = ang;
        work->ang.y = ang;
        push = record->consts.air_accel * push;
    }
    acc.x += push;
    record->accel = acc;
}

void player_accel_glide(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    Angle ang;
    Float stroke;
    NJS_VECTOR acc;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    motion->accel.y = record->consts.gravity * 0.95F;
    acc = motion->accel;
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    acc.x += 0.5F * (record->consts.resist.x * record->speed.x);
    acc.y += 0.5F * (record->consts.resist.y * record->speed.y);
    acc.z += 0.5F * (record->consts.resist.z * record->speed.z);
    if (player_read_stick(work, &ang, &stroke) == FALSE) {
        stroke = 0.0F;
        motion->aim.y = work->ang.y;
    } else {
        stroke = record->consts.air_accel * stroke;
        motion->aim.y = ang;
        player_turn_toward(work, motion, record, motion->aim.y);
    }
    acc.x += stroke;
    record->accel = acc;
}

void player_accel_climb(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    NJS_VECTOR acc;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    if (player_read_stick(work, NULL, NULL)) {
        PDS_PERIPHERAL *pad = &pad_records[PLAYER_VARS(work)->player];
        Float x = (Float)(pad->x1 << 8);
        Float y = (Float)(pad->y1 << 8);
        Float up = y * (1.0F / 40960.0F);
        Float side = x * (1.0F / 40960.0F);

        acc.x = -up;
        acc.y = 0.0F;
        acc.z = side;
    } else {
        acc.x = acc.y = acc.z = 0.0F;
    }
    record->accel = acc;
}

void player_accel_wall(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    NJS_VECTOR acc;
    NJS_VECTOR push;
    Float accel;
    Float x;
    Float y;
    Float grip;
    Float sum;
    Angle ang;
    Angle off;
    Bool pushed;

    acc = motion->accel;
    push.x = push.y = push.z = 0.0F;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    acc.y += -record->consts.gravity;
    acc.x += -record->consts.gravity;
    player_turn_back(work, &acc);
    pushed = player_read_stick(work, NULL, NULL);
    if (!(work->flags & PLAYER_ON_LAND) &&
        record->speed.x > record->consts.top_speed) {
        acc.x += record->consts.resist.x * record->speed.x;
    } else if (PLAYER_VARS(work)->character == CHARACTER_SONIC &&
               (work->flags & PLAYER_BALL) &&
               record->speed.x > record->consts.top_speed * 1.2F) {
        acc.x += record->consts.resist.x *
                 (record->speed.x - (record->consts.top_speed * 1.2F));
    } else if (!(work->flags & PLAYER_BALL) &&
               record->speed.x > record->consts.top_speed) {
        acc.x += record->consts.resist.x *
                 (record->speed.x - record->consts.top_speed);
    }
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;
    if ((work->flags & PLAYER_ON_LAND) &&
        record->speed.x < record->consts.top_speed) {
        accel = record->consts.ground_accel;
    } else {
        accel = record->consts.ground_accel * 0.3F;
    }
    motion->drive = accel;
    push.x = accel;
    if (!pushed) {
        motion->aim.y = 0;
        player_turn_on_course(work, motion, record, motion->aim.y);
    } else {
        x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
        y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
        if (x >= 0.0F) {
            x -= PAD_DEAD_ZONE;
        } else {
            x += PAD_DEAD_ZONE;
        }
        if (y >= 0.0F) {
            y -= PAD_DEAD_ZONE;
        } else {
            y += PAD_DEAD_ZONE;
        }
        ang = (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN);
        if (camera_work->ang.z >= 0x4000) {
            ang = ang - camera_work->ang.y;
        } else {
            ang = 0x8000 - ang - camera_work->ang.y;
        }
        off = angle_difference(ang, 0);
        if (off > 0x800) {
            if (work->flags & PLAYER_BALL) {
                ang = 0x400;
            } else {
                ang = 0x800;
            }
        } else if (off < -0x800) {
            if (work->flags & PLAYER_BALL) {
                ang = -0x400;
            } else {
                ang = -0x800;
            }
        }
        motion->aim.y = (Uint16)ang;
        player_turn_at_speed(work, motion, record, motion->aim.y);
    }
    grip = acc.y * record->consts.still_grip * motion->gravity;
    if (!(work->flags & PLAYER_ON_LAND)) {
        acc.x += push.x;
    } else if (grip > 0.0F && push.x > grip) {
        sum = push.x + acc.x;
        if (sum > grip) {
            motion->slip = sum - grip;
            sum = grip;
        }
        acc.x = sum;
    } else {
        acc.x += push.x;
    }
    if (record->speed.z == 0.0F) {
        grip = acc.y * record->consts.still_grip * motion->gravity;
        if (grip > acc.z && acc.z > -grip) {
            acc.z = 0.0F;
        }
    } else {
        if (acc.y < 0.0F) {
            grip = acc.y * record->consts.side_grip * motion->gravity;
        } else {
            grip = 0.0F;
        }
        if (acc.z > 0.0F) {
            push.z -= grip;
        } else if (acc.z < 0.0F) {
            push.z += grip;
        }
        sum = acc.z + push.z;
        if (acc.z != 0.0F && push.z != 0.0F && acc.z * sum < 0.0F) {
            sum = 0.0F;
        }
        acc.z = sum;
    }
    record->accel = acc;
}

void player_accel_wall_idle(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    NJS_VECTOR acc;
    NJS_VECTOR push;
    Float accel;

    acc = motion->accel;
    push.x = push.y = push.z = 0.0F;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    acc.y += -record->consts.gravity;
    acc.x += -record->consts.gravity;
    player_turn_back(work, &acc);
    if (record->speed.x > record->consts.top_speed) {
        acc.x += 2.0F * (record->consts.resist.x *
                         (record->speed.x - record->consts.top_speed));
    }
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;
    if (record->speed.x < record->consts.top_speed) {
        accel = record->consts.ground_accel * 0.3F;
    } else {
        accel = 0.0F;
    }
    motion->drive = accel;
    push.x = accel;
    motion->aim.y = 0;
    player_turn_on_course(work, motion, record, motion->aim.y);
    acc.x += push.x;
    record->accel = acc;
}

/* a board's speeds go up in steps of 0.92 */
#define BOARD_SPEED(n) ((n) * 0.92F)

void player_accel_board(struct task_work *work, struct task_motion *motion,
                        struct player *record, Float max_speed)
{
    Float stroke;
    Angle ang;
    Bool pushed;
    NJS_VECTOR acc;
    NJS_VECTOR push;
    NJS_VECTOR down;
    Angle turn;
    Angle step;
    Angle off;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Angle fall;
    Float power;
    Float keep;
    Float rest;
    Float grip;
    Float x;

    acc = motion->accel;
    push.x = push.y = push.z = 0.0F;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    acc.y += -record->consts.gravity;
    player_turn_back(work, &acc);
    pushed = player_read_stick(work, &ang, &stroke);
    if (record->speed.x > max_speed) {
        acc.x += record->consts.resist.x * record->speed.x;
        acc.z += 1.5F * (record->consts.resist.x * record->speed.z);
    } else if (record->speed.x < 0.0F) {
        acc.x += 1.5F * (record->consts.resist.x * record->speed.x);
        acc.z += 2.0F * (record->consts.resist.x * record->speed.z);
    }
    acc.y += record->consts.resist.y * record->speed.y;
    down.x = down.z = 0.0F;
    down.y = -1.0F;
    player_turn_back(work, &down);
    if ((down.x * down.x) + (down.z * down.z) > 0.01F) {
        /* @hack redundant: fall, the heading down the slope, is never read,
         * but the image calls atan2f for it (JSR at 0x8c0742c0). down is a
         * turned unit vector, and the test above keeps its x and z from both
         * being 0, so the call sets no errno and changes nothing; without
         * the call, 441 of 573 words differ */
        fall = (Angle)(atan2f(down.z, down.x) * ANGLE_PER_RADIAN);
    } else {
        /* @hack redundant: fall set to 0 in an else, whose jump loads the
         * stick's result for the test below into R3 (MOV.L @(20,R15),R3 at
         * 0x8c0742c4); without it, R2, and 2 of 573 words differ */
        fall = 0;
    }
    /* @hack redundant: fall, set from eight fields and never read, whose
     * reads put njSqrt's two operands at R15+16 and R15+12 (0x8c074494,
     * 0x8c07449c); without them, the two trade and 3 of 573 words differ */
    fall = motion->aim.x;
    fall = motion->aim.y;
    fall = motion->aim.z;
    fall = motion->phase;
    fall = work->ang.x;
    fall = work->ang.y;
    fall = work->ang.z;
    fall = work->flags;
    if (pushed) {
        off = angle_distance(work->ang.y, ang);
        if (record->speed.x == 0.0F && off > 0x1000) {
            motion->aim.y = ang;
            player_turn_on_course(work, motion, record, motion->aim.y);
        } else {
            turn = angle_difference(0xc000 - camera_work->ang.y, ang);
            if (camera_work->submode) {
                turn = 0x10000 - turn;
            }
            motion->drive = stroke * njSin(turn);
            if (record->anim.state == PLAYER_ANIM_DRIVEN) {
                x = record->anim.frame /
                    (Float)record->anim.actions[0].action->motion->nbFrame;
                stroke *= 0.5F;
                stroke += 0.005F * x;
            }
            power = stroke * njSin(turn);
            step = 0x200 - (Angle)((record->speed.x - BOARD_SPEED(10)) *
                                   (256.0F / BOARD_SPEED(10)));
            if (step < 0x120) {
                step = 0x120;
            } else if (step > 0x280) {
                step = 0x280;
            }
            step = (Angle)(step * power) + work->ang.y;
            motion->aim.y = step;
            ang = angle_distance(work->ang.y, motion->aim.y) >> 4;
            work->ang.y =
                angle_approach(work->ang.y, motion->aim.y, ang + 0x100);
        }
        if (record->speed.x < record->consts.top_speed) {
            stroke *= record->consts.ground_accel;
        } else {
            stroke *= record->consts.ground_accel * 0.4F;
        }
        if (record->speed.x <= record->consts.trot_speed) {
            push.x = stroke;
        }
    }
    down.x = njSqrt((record->speed.x * record->speed.x) +
                    (record->speed.z * record->speed.z));
    down.y = record->speed.y;
    down.z = 0.0F;
    if (record->speed.x < 0.0F) {
        down.x = -down.x;
    }
    keep = 0.6F;
    if (record->speed.x < BOARD_SPEED(5)) {
        rest = 0.4F;
    } else if (record->speed.x < BOARD_SPEED(10)) {
        keep += (record->speed.x - BOARD_SPEED(5)) * (0.3F / (BOARD_SPEED(5)));
        rest = 1.0F - keep;
    } else {
        keep = 0.9F;
        rest = 0.1F;
    }
    record->speed.x = (keep * record->speed.x) + (down.x * rest);
    record->speed.y = (keep * record->speed.y) + (down.y * rest);
    record->speed.z = (keep * record->speed.z) + (down.z * rest);
    if (push.x < 0.0F) {
        if (record->speed.x > 0.0F) {
            x = acc.x + push.x;
            if (push.x * x < 0.0F) {
                x = 0.0F;
            }
            acc.x = x;
        } else {
            if (acc.y < 0.0F) {
                grip = acc.y * record->consts.still_grip * motion->gravity;
            } else {
                grip = 0.0F;
            }
            x = grip + push.x;
            if (x >= 0.0F) {
                x = 0.0F;
            }
            acc.x = x;
        }
    } else {
        if (acc.y < 0.0F) {
            grip = acc.y * record->consts.still_grip * motion->gravity;
        } else {
            grip = 0.0F;
        }
        acc.x += push.x;
        if (acc.x > 0.0F) {
            if (grip > acc.x) {
                acc.x = 0.0F;
            } else {
                acc.x -= grip;
            }
        } else if (acc.x < 0.0F) {
            if (acc.x > -grip) {
                acc.x = 0.0F;
            } else {
                acc.x += grip;
            }
        }
    }
    if (record->speed.z == 0.0F) {
        grip = acc.y * record->consts.still_grip * motion->gravity;
        if (grip < 0.0F) {
            grip = 0.0F;
        }
        acc.z += push.z;
        if (grip > acc.z && acc.z > -grip) {
            acc.z = 0.0F;
        }
    } else {
        grip = acc.y * record->consts.side_grip * motion->gravity;
        if (grip < 0.0F) {
            grip = 0.0F;
        }
        acc.z += push.z;
        if (acc.z > 0.0F) {
            if (grip > acc.z) {
                acc.z = 0.0F;
            } else {
                acc.z -= grip;
            }
        } else if (acc.z < 0.0F) {
            if (acc.z > -grip) {
                acc.z = 0.0F;
            } else {
                acc.z += grip;
            }
        }
    }
    record->accel = acc;
}
void player_accel_board_air(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    Float stroke;
    Angle ang;
    Angle off;
    NJS_VECTOR acc;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        player_accel_air_event(work, motion, record);
        return;
    }
    acc = motion->accel;
    acc.y += -record->consts.gravity;
    player_turn_back(work, &acc);
    acc.x += record->consts.air_resist * record->speed.x;
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;
    if (player_read_stick(work, &ang, &stroke) == FALSE) {
        stroke = 0.0F;
        motion->aim.y = work->ang.y;
    } else {
        off = angle_distance(work->ang.y, ang);
        if (record->speed.x > record->consts.running_speed && off > 0x6000) {
            stroke = record->consts.air_brake * stroke;
        } else {
            motion->aim.y = ang;
            stroke = record->consts.air_accel * stroke;
        }
        player_turn_at_speed(work, motion, record, motion->aim.y);
    }
    acc.x += stroke;
    record->accel = acc;
}

void player_accel_board_trick(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    Float stroke;
    Angle ang;
    Angle off;
    NJS_VECTOR acc;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        player_accel_air_event(work, motion, record);
        return;
    }
    acc = motion->accel;
    acc.y += -record->consts.gravity - 0.05F;
    player_turn_back(work, &acc);
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;
    if (player_read_stick(work, &ang, &stroke) == FALSE) {
        stroke = 0.0F;
        motion->aim.y = work->ang.y;
    } else {
        off = angle_distance(work->ang.y, ang);
        if (record->speed.x > record->consts.running_speed && off > 0x6000) {
            stroke = record->consts.air_brake * stroke;
        } else {
            stroke = record->consts.air_accel * stroke;
        }
        player_turn_at_speed(work, motion, record, motion->aim.y);
    }
    acc.x += stroke;
    record->accel = acc;
}

void player_accel_board_brake(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    Float stroke;
    Angle heading;
    Angle ang;
    Bool pushed;
    NJS_VECTOR acc;
    NJS_VECTOR push;
    NJS_VECTOR speed;
    NJS_VECTOR turned;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Angle turn;
    Angle rot;
    Float power;
    Float keep;
    Float rest;
    Float grip;
    Float x;

    acc = motion->accel;
    push.z = 0.0F;
    push.y = 0.0F;
    push.x = 0.0F;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    acc.y += -record->consts.gravity;
    player_turn_back(work, &acc);
    pushed = player_read_stick(work, &ang, &stroke);
    if (record->speed.x > BOARD_SPEED(9)) {
        acc.x += record->consts.resist.x * record->speed.x;
        acc.z += 1.5F * (record->consts.resist.x * record->speed.z);
    } else if (record->speed.x < 0.0F) {
        acc.x += 1.5F * (record->consts.resist.x * record->speed.x);
        acc.z += 2.0F * (record->consts.resist.x * record->speed.z);
    }
    acc.y += record->consts.resist.y * record->speed.y;
    speed.x = speed.z = 0.0F;
    speed.y = -1.0F;
    player_turn_back(work, &speed);
    heading =
        (Angle)(atan2f(speed.z, speed.x) * ANGLE_PER_RADIAN) + work->ang.y;
    speed = record->speed;
    player_turn_to_world(work, &speed);
    motion->aim.y =
        (Angle)(atan2f(speed.z, speed.x) * ANGLE_PER_RADIAN) & 0xffff;
    work->ang.y = angle_approach(work->ang.y, motion->aim.y, 0x200);
    player_turn_back(work, &speed);
    record->speed = speed;
    if (!pushed) {
        speed = record->speed;
        player_turn_to_world(work, &speed);
        motion->aim.y = heading;
        work->ang.y = angle_approach(work->ang.y, motion->aim.y, 0x400);
        player_turn_back(work, &speed);
        keep = 0.8F;
        rest = 1.0F - keep;
        record->speed.x = (keep * record->speed.x) + (speed.x * rest);
        record->speed.y = (keep * record->speed.y) + (speed.y * rest);
        record->speed.z = (keep * record->speed.z) + (speed.z * rest);
    } else {
        turn = angle_difference(0xc000 - camera_work->ang.y, ang);
        if (record->speed.x < BOARD_SPEED(3)) {
            power = stroke * 1.5F;
        } else {
            power = stroke * 0.1F;
        }
        motion->drive = power;
        speed = record->speed;
        player_turn_to_world(work, &speed);
        rot = work->ang.y - (Angle)(power * 1024.0F);
        turned.x = speed.x;
        turned.y = speed.y;
        turned.z = speed.z;
        matrix_push_unit();
        TURN(njRotateY, rot);
        TURN(njRotateX, -work->ang.x);
        TURN(njRotateZ, -work->ang.z);
        njCalcVector(NULL, &turned, &speed);
        njPopMatrix(1);
        keep = 0.5F;
        rest = 1.0F - keep;
        record->speed.x = (keep * record->speed.x) + (speed.x * rest);
        record->speed.y = (keep * record->speed.y) + (speed.y * rest);
        record->speed.z = (keep * record->speed.z) + (speed.z * rest);
        if (record->speed.x < record->consts.top_speed) {
            stroke *= -record->consts.ground_accel;
        } else {
            stroke *= -(record->consts.ground_accel * 0.4F);
        }
        push.x = stroke;
    }
    if (push.x < 0.0F) {
        if (record->speed.x > 0.0F) {
            x = acc.x + push.x;
            if (push.x * x < 0.0F) {
                x = 0.0F;
            }
            acc.x = x;
        } else {
            if (acc.y < 0.0F) {
                grip = acc.y * record->consts.still_grip * motion->gravity;
            } else {
                grip = 0.0F;
            }
            x = grip + push.x;
            if (x >= 0.0F) {
                x = 0.0F;
            }
            acc.x = x;
        }
    } else {
        if (acc.y < 0.0F) {
            grip = acc.y * record->consts.still_grip * motion->gravity;
        } else {
            grip = 0.0F;
        }
        acc.x += push.x;
        if (acc.x > 0.0F) {
            if (grip > acc.x) {
                acc.x = 0.0F;
            } else {
                acc.x -= grip;
            }
        } else if (acc.x < 0.0F) {
            if (acc.x > -grip) {
                acc.x = 0.0F;
            } else {
                acc.x += grip;
            }
        }
    }
    if (record->speed.z == 0.0F) {
        grip = acc.y * record->consts.still_grip * motion->gravity;
        if (grip < 0.0F) {
            grip = 0.0F;
        }
        acc.z += push.z;
        if (grip > acc.z && acc.z > -grip) {
            acc.z = 0.0F;
        }
    } else {
        grip = acc.y * record->consts.side_grip * motion->gravity;
        if (grip < 0.0F) {
            grip = 0.0F;
        }
        acc.z += push.z;
        if (acc.z > 0.0F) {
            if (grip > acc.z) {
                acc.z = 0.0F;
            } else {
                acc.z -= grip;
            }
        } else if (acc.z < 0.0F) {
            if (acc.z > -grip) {
                acc.z = 0.0F;
            } else {
                acc.z += grip;
            }
        }
    }
    record->accel = acc;
}

void player_accel_brake(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    NJS_VECTOR acc;
    NJS_VECTOR brake;
    Float grip;
    Float x;

    acc = motion->accel;
    brake.x = brake.y = brake.z = 0.0F;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }

    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    acc.x += record->consts.resist.x * record->speed.x;
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;

    if (record->speed.x > 0.0F) {
        brake.x += record->consts.brake_drag * motion->gravity;
    } else if (record->speed.x < 0.0F) {
        brake.x -= record->consts.brake_drag * motion->gravity;
    }
    if (record->speed.z > 0.0F) {
        brake.z += record->consts.brake_drag * motion->gravity;
    } else if (record->speed.z < 0.0F) {
        brake.z -= record->consts.brake_drag * motion->gravity;
    }

    if (work->flags & PLAYER_ON_GROUND) {
        if (record->speed.x == 0.0F) {
            grip = acc.y * record->consts.still_grip * motion->gravity;
            if (grip > acc.x && acc.x > -grip) {
                acc.x = 0.0F;
            }
        } else {
            if (acc.y < 0.0F) {
                grip = acc.y * record->consts.moving_grip * motion->gravity;
            } else {
                grip = 0.0F;
            }
            x = acc.x + brake.x;
            if (x > 0.0F) {
                x -= grip;
                if (x < 0.0F) {
                    x = 0.0F;
                }
            } else if (x < 0.0F) {
                x += grip;
                if (x > 0.0F) {
                    x = 0.0F;
                }
            }
            acc.x = x;
            if (record->speed.x > 0.0F && -acc.x > record->speed.x) {
                acc.x = -record->speed.x;
            } else if (record->speed.x < 0.0F && -acc.x < record->speed.x) {
                acc.x = -record->speed.x;
            }
        }

        if (record->speed.z == 0.0F) {
            grip = acc.y * record->consts.still_grip * motion->gravity;
            if (grip > acc.z && acc.z > -grip) {
                acc.z = 0.0F;
            }
        } else {
            if (acc.y < 0.0F) {
                grip = acc.y * record->consts.side_grip * motion->gravity;
            } else {
                grip = 0.0F;
            }
            if (acc.z > 0.0F) {
                brake.z -= grip;
            } else if (acc.z < 0.0F) {
                brake.z += grip;
            }
            acc.z += brake.z;
            if (record->speed.z > 0.0F && -acc.z > record->speed.z) {
                acc.z = -record->speed.z;
            } else if (record->speed.z < 0.0F && -acc.z < record->speed.z) {
                acc.z = -record->speed.z;
            }
        }
    }
    player_turn_toward(work, motion, record, motion->aim.y);
    record->accel = acc;
}

void player_accel_launched(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    NJS_VECTOR acc;
    NJS_VECTOR brake;
    Float grip;
    Float scale;
    struct polygon_ball *ball;
    NJS_VECTOR push;

    acc = motion->accel;
    brake.x = brake.y = brake.z = 0.0F;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }

    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    if (work->flags & PLAYER_ON_GROUND) {
        ball = record->contact;
        scale =
            record->consts.top_speed / (record->consts.gravity * 0.7142857F);
        push.x = ball->hit.normal_speed.x / scale;
        push.y = ball->hit.normal_speed.y / scale;
        push.z = ball->hit.normal_speed.z / scale;
        ball->hit.normal_speed.y = 0.0F;
        ball->hit.normal_speed.x = 0.0F;
        ball->hit.normal_speed.z = 0.0F;
        player_turn_back(work, &push);
        acc.y += push.y;
        acc.z += push.z;
    }
    acc.x += record->consts.resist.x * record->speed.x;
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;

    if (work->flags & PLAYER_ON_GROUND) {
        if (record->speed.x == 0.0F) {
            grip = acc.y * record->consts.still_grip * motion->gravity;
            if (grip > acc.x && acc.x > -grip) {
                acc.x = 0.0F;
            }
        } else {
            if (acc.y < 0.0F) {
                grip = acc.y * record->consts.moving_grip * motion->gravity;
            } else {
                grip = 0.0F;
            }
            if (acc.x > 0.0F) {
                brake.x -= grip;
            } else if (acc.x < 0.0F) {
                brake.x += grip;
            }
            acc.x += brake.x;
            if (record->speed.x > 0.0F && -acc.x > record->speed.x) {
                acc.x = -record->speed.x;
            } else if (record->speed.x < 0.0F && -acc.x < record->speed.x) {
                acc.x = -record->speed.x;
            }
        }

        if (record->speed.z == 0.0F) {
            grip = acc.y * record->consts.still_grip * motion->gravity;
            if (grip > acc.z && acc.z > -grip) {
                acc.z = 0.0F;
            }
        } else {
            if (acc.y < 0.0F) {
                grip = acc.y * record->consts.side_grip * motion->gravity;
            } else {
                grip = 0.0F;
            }
            if (acc.z > 0.0F) {
                brake.z -= grip;
            } else if (acc.z < 0.0F) {
                brake.z += grip;
            }
            acc.z += brake.z;
            if (record->speed.z > 0.0F && -acc.z > record->speed.z) {
                acc.z = -record->speed.z;
            } else if (record->speed.z < 0.0F && -acc.z < record->speed.z) {
                acc.z = -record->speed.z;
            }
        }
    }
    record->accel = acc;
}

void player_accel_roll(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    NJS_VECTOR acc;
    NJS_VECTOR push;
    Float scale;
    struct polygon_ball *ball;

    acc = motion->accel;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);

    /* @hack spelling: the image compares with minus zero, which tests as
     * 0.0F does. its pool holds it at 0x8c0757d4 for FMOV.S to load
     * (0x8c07570e); as 0.0F, 140 of 196 words differ */
    if (record->speed.x > record->consts.running_speed &&
        record->level < -0.0F) {
        acc.y = -(acc.y * 8.0F);
    }

    if (work->flags & PLAYER_ON_GROUND) {
        ball = record->contact;
        scale =
            record->consts.top_speed / (record->consts.gravity * 0.7142857F);
        push.x = ball->hit.normal_speed.x / scale;
        push.y = ball->hit.normal_speed.y / scale;
        push.z = ball->hit.normal_speed.z / scale;
        ball->hit.normal_speed.y = 0.0F;
        ball->hit.normal_speed.x = 0.0F;
        ball->hit.normal_speed.z = 0.0F;
        player_turn_back(work, &push);
        acc.y += push.y;
        acc.z += push.z;
    }

    if (work->flags & PLAYER_BALL) {
        if (record->level > 0.98F) {
            acc.x += record->consts.resist.x * record->speed.x;
        } else {
            acc.x += -(record->speed.x * 0.0002F);
        }
    } else {
        acc.x += record->consts.resist.x * record->speed.x;
    }
    acc.y += record->consts.resist.y * record->speed.y;
    acc.z += record->consts.resist.z * record->speed.z;
    record->accel = acc;
}

void player_accel_gravity(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    NJS_VECTOR acc;

    acc = motion->accel;
    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    acc.x += world_gravity.x * record->consts.gravity;
    acc.y += world_gravity.y * record->consts.gravity;
    acc.z += world_gravity.z * record->consts.gravity;
    player_turn_back(work, &acc);
    record->accel = acc;
}

void player_turn_to_stick(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    NJS_VECTOR forward;
    NJS_VECTOR axis;
    NJS_VECTOR heading;
    Angle stick;
    Angle ang;
    Angle turn;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    if (work->flags & PLAYER_ON_PATH) {
        player_follow_path(work, motion, record);
    } else if (!player_read_stick(work, &ang, NULL)) {
        if (work->flags & PLAYER_ON_GROUND) {
            /* @hack redundant: both arms roll the player downhill, but the
             * image still tests the speed (FCMP/GT at 0x8c075954); as one
             * call, 159 of 189 words differ */
            if (record->speed.x > 2.0F) {
                player_roll_downhill(work, motion, record);
            } else {
                player_roll_downhill(work, motion, record);
            }
            return;
        } else {
            motion->aim.y = work->ang.y;
        }
    } else if (record->ground_flags & WORLD_CLING) {
        stick = (Angle)(atan2f((Float)(pad_records[PLAYER_VARS(work)->player].x1
                                       << 8),
                               (Float)(pad_records[PLAYER_VARS(work)->player].y1
                                       << 8)) *
                        ANGLE_PER_RADIAN);
        axis.x = 0.0F;
        axis.y = 0.0F;
        axis.z = -1.0F;
        matrix_push_unit();
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x);
        njCalcVector(NULL, &axis, &forward);
        njPopMatrix(1);
        matrix_push_unit();
        njRotate(NULL, &record->ground_normal, stick);
        njCalcVector(NULL, &forward, &heading);
        njPopMatrix(1);
        axis.x = 1.0F;
        axis.y = 0.0F;
        axis.z = 0.0F;
        player_turn_to_world(work, &axis);
        turn = vector_angle(&axis, &heading, &record->ground_normal);
        motion->aim.y = (Uint16)(work->ang.y + turn);
        player_turn_at_speed(work, motion, record, motion->aim.y);
    } else {
        motion->aim.y = ang;
    }
    player_turn_toward(work, motion, record, motion->aim.y);
}

static void player_roll_downhill(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    NJS_VECTOR down;
    NJS_VECTOR speed;
    Angle ang;
    Angle step;

    if (record->launch_steps) {
        return;
    }
    if (record->ground_flags & WORLD_NO_SLOPE) {
        return;
    }
    down = world_gravity;
    speed = motion->speed;
    if (njScalor(&speed) > record->consts.trot_speed) {
        njUnitVector(&speed);
        if (njInnerProduct(&world_gravity, &speed) < -0.86F) {
            return;
        }
    }
    player_turn_back(work, &down);
    if (down.y < 0.0F && down.y > -0.87F) {
        if (down.x < 0.0F) {
            down.x = -down.x;
        }
        ang = work->ang.y + (Angle)(atan2f(down.z, down.x) * ANGLE_PER_RADIAN);
        if (down.z < 0.0F) {
            down.z = -down.z;
        }
        if (work->flags & PLAYER_BALL) {
            step = (Angle)(down.z * 3072.0F);
        } else {
            step = (Angle)(down.z * 1536.0F);
        }
        motion->aim.y = angle_approach(motion->aim.y, ang, step);
    } else {
        motion->aim.y = work->ang.y;
    }
    player_turn_toward(work, motion, record, motion->aim.y);
}

static void player_steer_downhill(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record)
{
    NJS_VECTOR down;
    NJS_VECTOR speed;
    Angle ang;
    Angle step;

    if (record->launch_steps != 0) {
        return;
    }
    down = world_gravity;
    speed = motion->speed;
    if (njScalor(&speed) > record->consts.trot_speed) {
        njUnitVector(&speed);
        if (njInnerProduct(&world_gravity, &speed) < -0.86F) {
            return;
        }
    } else {
        return;
    }
    player_turn_back(work, &down);
    if (down.y > -0.87F) {
        if (down.x < 0.0F) {
            down.x = -down.x;
        }
        ang = work->ang.y + (Angle)(atan2f(down.z, down.x) * ANGLE_PER_RADIAN);
        if (down.z < 0.0F) {
            down.z = -down.z;
        }
        if (work->flags & PLAYER_BALL) {
            step = (Angle)(down.z * record->speed.x * 4096.0F /
                           record->consts.trot_speed);
        } else {
            step = (Angle)(down.z * 2048.0F);
        }
        motion->aim.y = angle_approach(motion->aim.y, ang, step);
    } else {
        motion->aim.y = work->ang.y;
    }
    player_turn_on_course(work, motion, record, motion->aim.y);
}

void player_accel_slope(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    NJS_VECTOR to;
    NJS_PLANE plane;
    Angle ang;

    if (record->launch_steps != 0) {
        return;
    }
    to = world_gravity;
    if (work->flags & PLAYER_ON_LAND) {
        plane.px = plane.py = plane.pz = 0.0F;
        plane.vx = record->ground_normal.x;
        plane.vy = record->ground_normal.y;
        plane.vz = record->ground_normal.z;
        njDistanceP2PL(&world_gravity, &plane, &to);
        if (njScalor(&to) != 0.0F) {
            njUnitVector(&to);
        } else {
            to = world_gravity;
        }
    }
    to.x *= record->consts.gravity * 0.4F;
    to.y *= record->consts.gravity * 0.4F;
    to.z *= record->consts.gravity * 0.4F;
    njAddVector(&to, &motion->accel);
    njAddVector(&motion->speed, &to);
    to = motion->speed;
    to.x *= record->consts.resist.x;
    to.y *= record->consts.resist.x;
    to.z *= record->consts.resist.x;
    njAddVector(&motion->speed, &to);
    to = motion->speed;
    player_turn_back(work, &to);
    record->speed = to;
    njAddVector(&motion->speed, &record->nudge);
    player_limit_speed(work, motion, record);
    player_stop_creep(work, motion);
    to = motion->speed;
    if (njScalor(&to) != 0.0F) {
        njUnitVector(&to);
        ang = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
        motion->aim.y = ang;
        work->ang.y = ang;
    }
}

void player_turn_upright(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    NJS_VECTOR speed;
    Float squared;

    speed = record->speed;
    if (!(record->upgrades & PLAYER_SONIC_SUPER)) {
        squared =
            (speed.x * speed.x) + (speed.y * speed.y) + (speed.z * speed.z);
        if (squared > record->consts.full_speed * record->consts.full_speed) {
            return;
        }
    }
    player_turn_to_world(work, &speed);
    work->ang.x = angle_approach(work->ang.x, world_gravity_ang_x, 0x800);
    work->ang.z = angle_approach(work->ang.z, world_gravity_ang_z, 0x800);
    player_turn_back(work, &speed);
    record->speed = speed;
}

void player_turn_to_wall(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    NJS_VECTOR speed;
    Float squared;

    speed = record->speed;
    squared = (speed.x * speed.x) + (speed.y * speed.y) + (speed.z * speed.z);
    if (squared > record->consts.full_speed * record->consts.full_speed) {
        return;
    }
    player_turn_to_world(work, &speed);
    work->ang.x = angle_approach(work->ang.x, 0, 0x800);
    work->ang.z = angle_approach(work->ang.z, 0xc000, 0x800);
    player_turn_back(work, &speed);
    record->speed = speed;
}

static void player_turn_toward(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record, Angle ang)
{
    NJS_VECTOR speed;
    Angle diff;
    Float keep;
    Float rest;
    Float x;
    Float pushed_rest;

    speed = record->speed;
    player_turn_to_world(work, &speed);
    diff = angle_distance(work->ang.y, ang);
    if (diff > 0x2000) {
        work->ang.y = angle_approach(work->ang.y, ang, 0x800);
    } else if (diff > 0x1000) {
        diff >>= 2;
        work->ang.y = angle_approach(work->ang.y, ang, diff);
    } else {
        diff >>= 3;
        work->ang.y = angle_approach(work->ang.y, ang, diff);
    }
    player_turn_back(work, &speed);
    if (!(work->flags & PLAYER_ON_GROUND)) {
        rest = 0.9F;
        keep = 0.1F;
    } else if (!player_read_stick(work, NULL, NULL)) {
        keep = 0.05F;
        if (motion->gravity < 1.0F) {
            keep *= motion->gravity;
        }
        rest = 1.0F - keep;
    } else {
        goto stroke;
    }
    record->speed.x = (record->speed.x * keep) + (speed.x * rest);
    record->speed.y = (record->speed.y * keep) + (speed.y * rest);
    record->speed.z = (record->speed.z * keep) + (speed.z * rest);
/* @hack redundant: the label end, which nothing jumps to, keeps the blend of
 * record->speed above it from sharing its last product and store with the
 * blend after stroke (0x8c076230 to 0x8c07623a); without it, SHC joins
 * them and 26 of 157 words differ */
end: /* NOLINT(clang-diagnostic-unused-label) */
    return;
stroke:
    x = speed.x;
    if (record->level > 0.4F) {
        keep = 0.99F;
        if (motion->gravity < 1.0F) {
            keep *= motion->gravity;
        }
    } else {
        keep = 0.5F;
        if (motion->gravity < 1.0F) {
            keep *= motion->gravity;
        }
    }
    pushed_rest = 1.0F - keep;
    x = x * pushed_rest;
    record->speed.x = (record->speed.x * keep) + x;
    record->speed.y = (record->speed.y * keep) + (speed.y * pushed_rest);
    record->speed.z = (record->speed.z * keep) + (speed.z * pushed_rest);
}

static void player_turn_on_course(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record, Angle ang)
{
    NJS_VECTOR speed;

    speed = record->speed;
    player_turn_to_world(work, &speed);
    work->ang.y = angle_approach(work->ang.y, ang, 0x2000);
    player_turn_back(work, &speed);
    record->speed = speed;
}

static void player_turn_at_speed(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record, Angle ang)
{
    NJS_VECTOR speed;
    Angle step;
    Float keep;
    Float rest;

    speed = record->speed;
    player_turn_to_world(work, &speed);
    step = 0x100;
    if (record->speed.x > record->consts.full_speed) {
        step -=
            njSqrt((record->speed.x - record->consts.full_speed) * 0.0625F) *
            256.0F;
    }
    work->ang.y = angle_approach(work->ang.y, ang, step);
    player_turn_back(work, &speed);
    if (record->level > 0.4F) {
        keep = 0.99F;
        rest = 1.0F - keep;
    } else {
        keep = 0.5F;
        rest = 1.0F - keep;
    }
    record->speed.x = (record->speed.x * keep) + (speed.x * rest);
    record->speed.y = (record->speed.y * keep) + (speed.y * rest);
    record->speed.z = (record->speed.z * keep) + (speed.z * rest);
}

void player_apply_accel(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    NJS_VECTOR speed;
    Float old;
    Float grip;

    old = record->speed.x;
    record->speed.x += record->accel.x;
    grip = record->consts.trot_speed * 0.87F;
    if (old > 0.0F) {
        if (record->speed.x < 0.0F) {
            record->speed.x = 0.0F;
        } else if (!player_read_stick(work, NULL, NULL) &&
                   record->speed.x < grip && record->level > 0.7F) {
            record->speed.x = 0.0F;
        }
    } else if (old < 0.0F) {
        /* @hack spelling: the image compares with minus zero, which tests as
         * 0.0F does. its pool holds it at 0x8c076510 for FMOV.S to load
         * (0x8c0764ec); as 0.0F, 10 of 227 words differ */
        if (record->speed.x > -0.0F) {
            record->speed.x = 0.0F;
        } else if (!player_read_stick(work, NULL, NULL) &&
                   record->speed.x > -grip && record->level > 0.7F) {
            record->speed.x = 0.0F;
        }
    }
    record->speed.y += record->accel.y;
    old = record->speed.z;
    record->speed.z += record->accel.z;
    if (old > 0.0F) {
        if (old < grip && old == record->speed.z) {
            record->speed.z = 0.0F;
        } else if (old >= grip && record->speed.z < grip) {
            record->speed.z = 0.0F;
        }
    } else if (old < 0.0F) {
        if (old > -grip && old == record->speed.z) {
            record->speed.z = 0.0F;
        } else if (old <= -grip && record->speed.z > -grip) {
            record->speed.z = 0.0F;
        }
    }
    if (record->shadow.above_flags & WORLD_THICK_LIQUID) {
        if (record->speed.x >= record->consts.trot_speed * 0.8F) {
            record->speed.x += record->consts.brake_drag;
        }
    } else if (record->shadow.above_flags & WORLD_LIQUID) {
        if (record->speed.x >=
            (record->consts.running_speed + record->consts.trot_speed) * 0.5F) {
            record->speed.x += record->consts.brake_drag;
        }
    }
    speed = record->speed;
    if (record->ground_flags & WORLD_NO_SLOPE) {
        ground_turn_to_world(&record->ground_normal, &speed, work->ang.y);
    } else {
        player_turn_to_world(work, &speed);
    }
    motion->speed = speed;
    njAddVector(&motion->speed, &record->nudge);
    player_limit_speed(work, motion, record);
    player_stop_creep(work, motion);
}

static void player_limit_speed(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    /* @hack redundant: both arms clamp the speed alike, but the image still
     * tests PLAYER_HOLDING (TST at 0x8c0766c4); as one arm, 125 of 133 words
     * differ */
    if (work->flags & PLAYER_HOLDING) {
        if (record->consts.speed_limit < motion->speed.x) {
            motion->speed.x = record->consts.speed_limit;
        }
        if (motion->speed.x < -record->consts.speed_limit) {
            motion->speed.x = -record->consts.speed_limit;
        }
        if (record->consts.speed_limit < motion->speed.z) {
            motion->speed.z = record->consts.speed_limit;
        }
        if (motion->speed.z < -record->consts.speed_limit) {
            motion->speed.z = -record->consts.speed_limit;
        }
        if (record->consts.fall_limit < motion->speed.y) {
            motion->speed.y = record->consts.fall_limit;
        }
        if (motion->speed.y < -record->consts.fall_limit) {
            motion->speed.y = -record->consts.fall_limit;
        }
    } else {
        if (record->consts.speed_limit < motion->speed.x) {
            motion->speed.x = record->consts.speed_limit;
        }
        if (motion->speed.x < -record->consts.speed_limit) {
            motion->speed.x = -record->consts.speed_limit;
        }
        if (record->consts.speed_limit < motion->speed.z) {
            motion->speed.z = record->consts.speed_limit;
        }
        if (motion->speed.z < -record->consts.speed_limit) {
            motion->speed.z = -record->consts.speed_limit;
        }
        if (record->consts.fall_limit < motion->speed.y) {
            motion->speed.y = record->consts.fall_limit;
        }
        if (motion->speed.y < -record->consts.fall_limit) {
            motion->speed.y = -record->consts.fall_limit;
        }
    }
}

static void player_stop_creep(struct task_work *work,
                              struct task_motion *motion)
{
    if (!player_read_stick(work, NULL, NULL)) {
        if (motion->speed.x < 0.02F && motion->speed.x > -0.02F) {
            motion->speed.x = 0.0F;
        }
        if (motion->speed.y < 0.02F && motion->speed.y > -0.02F) {
            motion->speed.y = 0.0F;
        }
        if (motion->speed.z < 0.02F && motion->speed.z > -0.02F) {
            motion->speed.z = 0.0F;
        }
    }
}

void player_clear_speed(struct task_motion *motion, struct player *record)
{
    if (record != NULL) {
        record->speed.z = 0.0F;
        record->speed.y = 0.0F;
        record->speed.x = 0.0F;
    }
    if (motion != NULL) {
        motion->speed.z = 0.0F;
        motion->speed.y = 0.0F;
        motion->speed.x = 0.0F;
    }
}

void player_apply_path_accel(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    NJS_VECTOR speed;
    Float old;

    old = record->speed.x;
    record->speed.x += record->accel.x;
    if (old > 0.0F) {
        if (record->speed.x < 0.0F) {
            record->speed.x = 0.0F;
        } else if (old >= 0.1F && record->speed.x < 0.1F) {
            record->speed.x = 0.0F;
        }
    } else if (old < 0.0F) {
        /* @hack spelling: the image compares with minus zero, which tests as
         * 0.0F does. its pool holds it at 0x8c0769b0 for FMOV.S to load
         * (0x8c076904); as 0.0F, 58 of 87 words differ */
        if (record->speed.x > -0.0F) {
            record->speed.x = 0.0F;
        } else if (old <= -0.1F && record->speed.x > -0.1F) {
            record->speed.x = 0.0F;
        }
    }
    record->speed.y = 0.0F;
    record->speed.z = 0.0F;
    speed = record->speed;
    player_turn_to_world(work, &speed);
    motion->speed = speed;
    player_limit_speed(work, motion, record);
}

void player_apply_push(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    struct task *pushed_task;
    struct task_motion *other;
    NJS_VECTOR speed;
    Float old;

    if (record->held == NULL) {
        pushed_task = work->collision->hit->owner;
    } else {
        pushed_task = record->held;
    }
    other = pushed_task->motion;
    if (!(player_carry_flags(work) & PLAYER_CAN_PUSH)) {
        player_clear_speed(motion, record);
        return;
    }
    old = record->speed.x;
    record->speed.x += record->accel.x;
    if (old > 0.0F) {
        if (record->speed.x < 0.0F) {
            record->speed.x = 0.0F;
        } else if (old >= 0.1F && record->speed.x < 0.1F) {
            record->speed.x = 0.0F;
        }
    } else if (old < 0.0F) {
        /* @hack spelling: the image compares with minus zero, which tests as
         * 0.0F does. its pool holds it at 0x8c076a48 for FMOV.S to load
         * into FR7 (0x8c0769f0); as 0.0F, 95 of 181 words differ */
        if (record->speed.x > -0.0F) {
            record->speed.x = 0.0F;
        } else if (old <= -0.1F && record->speed.x > -0.1F) {
            record->speed.x = 0.0F;
        }
    }
    if (record->consts.push_speed < record->speed.x) {
        record->speed.x = record->consts.push_speed;
    }
    if (record->speed.x < -record->consts.push_speed) {
        record->speed.x = -record->consts.push_speed;
    }
    record->speed.y += record->accel.y;
    old = record->speed.z;
    record->speed.z += record->accel.z;
    if (old > 0.0F) {
        if (record->speed.z < 0.0F) {
            record->speed.z = 0.0F;
        } else if (old >= 0.1F && record->speed.z < 0.1F) {
            record->speed.z = 0.0F;
        }
    } else if (old < 0.0F) {
        /* @hack spelling: the image compares with minus zero, which tests as
         * 0.0F does, in the FR7 that the test of speed.x above loads
         * (FCMP/GT at 0x8c076ad6); as 0.0F, 41 of 181 words differ */
        if (record->speed.z > -0.0F) {
            record->speed.z = 0.0F;
        } else if (old <= -0.1F && record->speed.z > -0.1F) {
            record->speed.z = 0.0F;
        }
    }
    speed = record->speed;
    player_turn_to_world(work, &speed);
    motion->speed = speed;
    if (other != NULL) {
        other->accel.x = speed.x;
        other->accel.y = 0.0F;
        other->accel.z = speed.z;
    }
    player_limit_speed(work, motion, record);
}

void player_apply_climb_accel(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    NJS_VECTOR speed;

    record->speed = record->accel;
    speed = record->speed;
    player_turn_to_world(work, &speed);
    motion->speed = speed;
    player_limit_speed(work, motion, record);
}

void player_apply_glide_accel(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    NJS_VECTOR speed;

    record->speed.x += record->accel.x;
    record->speed.y += record->accel.y;
    record->speed.z += record->accel.z;
    if (record->speed.x < 1.5F) {
        if (record->speed.x < 0.0F) {
            record->speed.x = 0.0F;
        }
        record->speed.x += 0.15F;
    }
    if (record->speed.x > 3.5F) {
        record->speed.x = 3.5F;
    }
    speed = record->speed;
    player_turn_to_world(work, &speed);
    motion->speed = speed;
    player_limit_speed(work, motion, record);
}

static void player_slide_along_wall(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record)
{
    NJS_VECTOR normal;
    NJS_PLANE wall;
    NJS_PLANE ground;
    union line_view line;
    NJS_VECTOR before;
    NJS_VECTOR after;
    Float speed;
    int flags;
    Float (*length)(NJS_VECTOR *vector);

    if (!(work->flags & PLAYER_ON_GROUND)) {
        return;
    }
    /* @hack spelling: njScalor called through length, a local, which keeps its
     * address in R11 and njUnitVector's in R10 (0x8c076cc0, 0x8c076d0a);
     * called by name, the two trade and 10 of 234 words differ */
    length = njScalor;
    if (length(&motion->speed) < 1.0F) {
        return;
    }
    flags = work->collision->flags;
    if (!(flags & COLLISION_PUSHED) || (work->flags & PLAYER_HURT) ||
        !(flags & COLLISION_HIT_SIDE)) {
        return;
    }
    collision_get_hit_normal(work, &normal);
    record->wall_normal = normal;
    if (length(&normal) == 0.0F) {
        normal.x = 0.0F;
        normal.y = 0.0F;
        normal.z = 0.0F;
    } else {
        njUnitVector(&normal);
    }
    wall.px = 0.0F;
    wall.py = 0.0F;
    wall.pz = 0.0F;
    wall.vx = record->wall_normal.x;
    wall.vy = record->wall_normal.y;
    wall.vz = record->wall_normal.z;
    ground.px = 0.0F;
    ground.py = 0.0F;
    ground.pz = 0.0F;
    ground.vx = record->ground_normal.x;
    ground.vy = record->ground_normal.y;
    ground.vz = record->ground_normal.z;
    /* @hack type: the union's own address as the NJS_LINE, which SHC forms from
     * R15 at each use; as &line.line, it keeps the address in a saved register,
     * the function runs 32 bytes short and 198 of 234 words differ */
    njDistancePL2PL(&wall, &ground, (NJS_LINE *)&line);
    before = motion->speed;
    if (length(&before) != 0.0F) {
        njUnitVector(&before);
    }
    if (njInnerProduct(&line.split.direction, &before) < 0.0F) {
        line.line.vx = -line.line.vx;
        line.line.vy = -line.line.vy;
        line.line.vz = -line.line.vz;
    }
    speed = length(&motion->speed);
    before = motion->speed;
    motion->speed.x = speed * line.line.vx;
    motion->speed.y = speed * line.line.vy;
    motion->speed.z = speed * line.line.vz;
    after = motion->speed;
    njUnitVector(&before);
    njUnitVector(&after);
    work->ang.y -= vector_angle(&before, &after, &record->ground_normal);
    if (angle_distance(motion->aim.y, work->ang.y) > 0x6000) {
        work->ang.y += 0x8000;
    }
    motion->aim.y = work->ang.y;
}

int player_collide(struct task_work *work, struct task_motion *motion,
                   struct player *record)
{
    NJS_VECTOR speed;
    NJS_VECTOR normal;
    Float reach;
    Float length;
    struct collision_work *touched;
    struct collision_shape *own;
    struct collision_shape *hit;
    struct task_work *other;
    Float dot;
    Angle turn;
    Angle heading;
    Bool none;
    int result = 0;
    int ret;
    void (*calc_vector)(NJS_MATRIX *, NJS_VECTOR *, NJS_VECTOR *);

    record->wall_normal.x = record->wall_normal.y = record->wall_normal.z =
        0.0F;
    record->last_ground_flags = record->ground_flags;
    record->ground_flags = 0;
    /* @hack spelling: njCalcVector called through calc_vector, a local, which
     * keeps its address in R9 and a 2 in R8 (MOV #2,R8 at 0x8c076f18);
     * called by name, the two trade and 10 of 648 words differ */
    calc_vector = njCalcVector;
    if ((work->collision->flags & COLLISION_PUSHED) &&
        !(work->flags & PLAYER_HURT) &&
        (work->collision->flags & COLLISION_HIT_SIDE) &&
        (work->collision->hit->shapes[work->collision->hit_shape].push &
         PUSH_ALL)) {
        own = work->collision->shapes;
        touched = work->collision->hit;
        hit = &touched->shapes[touched->current];
        speed = motion->speed;
        collision_get_hit_normal(work, &normal);
        record->wall_normal = normal;
        if (njScalor(&normal) == 0.0F) {
            normal.x = 0.0F;
            normal.y = 0.0F;
            normal.z = 0.0F;
        } else {
            njUnitVector(&normal);
        }
        if (njScalor(&speed) == 0.0F) {
            speed.x = 0.0F;
            speed.y = 0.0F;
            speed.z = 0.0F;
        } else {
            njUnitVector(&speed);
        }
        dot = njInnerProduct(&speed, &normal);
        if (record->speed.x >= 15.0F) {
            result = PLAYER_CRASHED;
            player_clear_speed(motion, record);
        } else if (record->speed.x >= record->consts.crash_speed &&
                   dot < -0.98F) {
            result = PLAYER_CRASHED;
            motion->speed.z = 0.0F;
            motion->speed.x = 0.0F;
        } else if (record->speed.x >= record->consts.running_speed &&
                   dot < 0.0F) {
            result = 1;
            if (((hit->flags & SHAPE_KIND) == SHAPE_SPHERE ||
                 (hit->flags & SHAPE_KIND) == SHAPE_CYLINDER) &&
                hit->radius > own->radius &&
                (hit->levels & SHAPE_ATTACK) <= (own->levels & SHAPE_GUARD) >>
                    2) {
                reach = own->radius + hit->radius;
                length = njSqrt((speed.x * speed.x) + (speed.z * speed.z));
                turn = (Angle)(acosf(length / reach) * ANGLE_PER_RADIAN);
                heading =
                    (Angle)(atan2f(normal.z, normal.x) * ANGLE_PER_RADIAN) +
                    0x8000;
                if (angle_difference(heading, (Angle)(atan2f(speed.z, speed.x) *
                                                      ANGLE_PER_RADIAN)) < 0) {
                    turn = 0x10000 - turn;
                }
                matrix_push_unit();
                TURN(njRotateY, -turn);
                speed = motion->speed;
                calc_vector(NULL, &speed, &normal);
                njPopMatrix(1);
                work->ang.y += turn;
                motion->speed.x = normal.x;
                motion->speed.z = normal.z;
            } else if (/* @bug logic: the test asks a shape's kind to be both
                        * SHAPE_RECTANGLE and SHAPE_RECTANGLE2, so it never
                        * holds (CMP/EQ #4,R0 at 0x8c077208, then CMP/EQ #5,R0
                        * at 0x8c077212). a player running into a rectangle
                        * too slowly for the arms above to crash it keeps its
                        * speed, where this arm would stop it or slide it
                        * along */
                       (hit->flags & SHAPE_KIND) == SHAPE_RECTANGLE &&
                       (hit->flags & SHAPE_KIND) == SHAPE_RECTANGLE2) {
                if ((work->collision->flags & COLLISION_PUSHED) &&
                    !(work->flags & PLAYER_HURT) &&
                    !(work->collision->hit->shapes[work->collision->hit_shape]
                          .flags &
                      SHAPE_NO_PUSH) &&
                    (work->collision->flags & COLLISION_HIT_SIDE)) {
                    motion->speed.z = 0.0F;
                    motion->speed.x = 0.0F;
                } else {
                    player_slide_along_wall(work, motion, record);
                }
            }
        } else {
            result = 1;
            if (dot < 0.0F) {
                if (player_carry_flags(work) & PLAYER_CAN_PUSH) {
                    motion->speed.x = -(record->speed.x * normal.x);
                    motion->speed.z = -(record->speed.x * normal.z);
                } else {
                    if ((work->collision->flags & COLLISION_PUSHED) &&
                        !(work->flags & PLAYER_HURT) &&
                        !(work->collision->hit
                              ->shapes[work->collision->hit_shape]
                              .flags &
                          SHAPE_NO_PUSH) &&
                        (work->collision->flags & COLLISION_HIT_SIDE)) {
                        speed.x += normal.x;
                        speed.y = 0.0F;
                        speed.z += normal.z;
                        if (njScalor(&speed) != 0.0F) {
                            njUnitVector(&speed);
                            motion->speed.x = record->speed.x * speed.x;
                            motion->speed.z = record->speed.x * speed.z;
                        } else {
                            motion->speed.x = 0.0F;
                            motion->speed.z = 0.0F;
                        }
                    } else {
                        player_slide_along_wall(work, motion, record);
                    }
                }
            }
        }
    } else if (work->collision->flags & COLLISION_HIT) {
        other = work->collision->hit->owner->work;
        none = other == NULL;
        /* @hack spelling: the tests in a loop of one pass, which they break
         * out of. the loop keeps njCalcVector's address in R9 and a 1 in R10
         * (0x8c076f20, 0x8c076f2a); as an if, the two trade and 178 of 648
         * words differ */
        do {
            if (none) {
                break;
            }
            if (other->id != OTTOTTO_ID) {
                break;
            }
            if ((work->flags & PLAYER_ON_LAND) && !(record->speed.y > 0.0F) &&
                (record->speed.x >= record->consts.running_speed ||
                 (work->flags & PLAYER_BALL))) {
                if (angle_distance(0x4000 - work->ang.y, other->ang.y) <
                    0x3000) {
                    result = PLAYER_TEETERING;
                    player_clear_speed(motion, record);
                }
            }
        } while (0);
        /* @hack redundant: turn, set from none and never read, which keeps
         * speed.x's copy at R15+16 and its length at R15+8 (0x8c0770f8,
         * 0x8c077112); without it, the two trade and 3 of 648 words differ */
        turn = none ? work->ang.y : work->ang.x;
    }
    if (work->flags & PLAYER_ON_TASK) {
        ret = player_collide_ground(work, motion, record);
        if (!result) {
            result = ret;
        }
        if (!(work->flags & PLAYER_ON_LAND)) {
            matrix_push_unit();
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, -work->ang.y);
            speed.x = speed.z = 0.0F;
            speed.y = 1.0F;
            calc_vector(NULL, &speed, &normal);
            record->ground_normal = normal;
            speed.x = speed.z = 0.0F;
            speed.y = -0.5F;
            calc_vector(NULL, &speed, &normal);
            njPopMatrix(1);
            work->pos.y += normal.y;
            motion->speed.y = 0.0F;
        }
    } else {
        ret = player_collide_ground(work, motion, record);
        if (!result) {
            result = ret;
        }
    }
    return result;
}

static Uint32 player_collide_ground(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record)
{
    if (stage_get_stage_act() == STAGE_ACT(STAGE_CASINO, 2) ||
        stage_get_stage_act() == STAGE_ACT(STAGE_CASINO, 3)) {
        return player_collide_pinball(work, motion, record);
    }
    return player_collide_world(work, motion, record);
}

static Uint32 player_collide_world(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record)
{
    NJS_VECTOR offset;
    NJS_VECTOR normal;
    NJS_VECTOR position;
    NJS_POINT3 old_pos;
    struct world_object candidates[16];
    struct world_object sorted[16];
    struct polygon_ball *ball;
    struct world_object *world;
    struct world_object *entry;
    struct world_object *last;
    struct world_object *list;
    struct polygon_contact *contact;
    struct task *owner;
    NJS_OBJECT *object;
    int old_flags;
    int i;
    int j;
    int k;
    int taken;
    int count;
    int saved;
    Bool hit;
    Bool grounded;
    Uint32 result;
    int found;
    int ang;
    int mask;
    int pitch;
    int roll;
    int other_pitch;
    int other_roll;
    int pitch_distance;
    int roll_distance;
    Float stroke;
    Float length;
    Float dot;
    Float excess;
    Float magnitude;
    Float gap;

    ball = record->contact;
    result = 0;
    no_slope_near = FALSE;
    old_flags = record->last_flags;
    record->last_flags = work->flags;
    work->flags &= ~PLAYER_ON_LAND;
    ball->normal.x = ball->normal.z = 0.0F;
    ball->normal.y = 1.0F;
    ball->hit.ang_x = work->ang.x;
    ball->hit.ang_z = work->ang.z;
    if (record->last_ground_flags & WORLD_NO_SLOPE) {
        ground_turn_to_world(&record->ground_normal, &ball->normal,
                             work->ang.y);
    } else {
        player_turn_to_world(work, &ball->normal);
    }
    offset.x = offset.z = 0.0F;
    offset.y = record->consts.height * 0.5F;
    normal = offset;
    if (record->last_ground_flags & WORLD_NO_SLOPE) {
        ground_turn_to_world(&record->ground_normal, &offset, work->ang.y);
    } else {
        player_turn_to_world(work, &offset);
    }
    ball_drop(motion, record, ball, &normal);
    if (record->last_ground_flags & WORLD_NO_SLOPE) {
        ground_turn_to_world(&record->ground_normal, &normal, work->ang.y);
    } else {
        player_turn_to_world(work, &normal);
    }
    ball->pos.x = work->pos.x + normal.x;
    ball->pos.y = work->pos.y + normal.y;
    ball->pos.z = work->pos.z + normal.z;
    ball->radius = record->consts.height * 0.5F;
    ball->speed = motion->speed;
    old_pos = work->pos;
    polygon_collect_near(ball);
    if (world_count == 0) {
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        player_set_ground(work, motion, record, 0);
        record->ground_task = NULL;
        record->ground_object = NULL;
        return 0;
    }
    grounded = FALSE;
    hit = FALSE;
    world = world_objects;
    object = NULL;
    saved = 0;
    count = 0;
    for (i = 0; i < world_count; i++, world++) {
        if ((world->flags & WORLD_LIQUID) &&
            PLAYER_VARS(work)->character != CHARACTER_E102) {
            continue;
        }
        if (world->owner != NULL &&
            !(world->owner->work->flags & WORLD_TESTED)) {
            continue;
        }
        if (polygon_sweep(ball, world->object, world->flags)) {
            if (record->last_flags & PLAYER_ON_GROUND) {
                if (ball_stop_at_walls(work, motion, record, ball,
                                       world->flags) == TRUE) {
                    hit = TRUE;
                    saved = count;
                }
            } else if (ball_stop_at_ground(work, motion, record, ball) ==
                       TRUE) {
                hit = TRUE;
                saved = count;
            }
        }
        candidates[count] = *world;
        count++;
        if (world->flags & WORLD_NO_SLOPE) {
            no_slope_near = TRUE;
        }
    }
    if (count == 0) {
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        player_set_ground(work, motion, record, 0);
        record->ground_task = NULL;
        record->ground_object = NULL;
        return 0;
    }
    if (event_in_control()) {
        no_slope_near = FALSE;
    }
    taken = 0;
    if (hit == TRUE) {
        sorted[taken] = candidates[saved];
        taken++;
    }
    last = &sorted[count - 1];
    k = count - 1;
    if (record->ground_task != NULL) {
        for (j = 0; j < count; j++) {
            if (hit == TRUE && j == saved) {
                hit = FALSE;
            } else {
                entry = &candidates[j];
                if (record->ground_object == entry->object) {
                    *last = *entry;
                } else {
                    sorted[taken] = *entry;
                    taken++;
                }
            }
        }
    } else {
        for (j = count - 1; j >= 0; j--) {
            if (hit == TRUE && j == saved) {
                hit = FALSE;
            } else {
                entry = &candidates[j];
                if (record->ground_object == entry->object) {
                    *last = *entry;
                } else {
                    sorted[taken] = *entry;
                    taken++;
                }
            }
        }
    }
    ball->pos.x = work->pos.x + normal.x;
    ball->pos.y = work->pos.y + normal.y;
    ball->pos.z = work->pos.z + normal.z;
    ball->speed = motion->speed;
    record->ground_task = NULL;
    list = sorted;
    for (i = 0; i < count; i++) {
        entry = &list[i];
        owner = entry->owner;
        if (!polygon_sweep(ball, entry->object, entry->flags)) {
            continue;
        }
        mask = 1;
        if ((work->flags & PLAYER_ON_LAND) && object != entry->object) {
            position.x =
                ball->hit.pos.x + ball->hit.speed.x - offset.x - work->pos.x;
            position.y =
                ball->hit.pos.y + ball->hit.speed.y - offset.y - work->pos.y;
            position.z =
                ball->hit.pos.z + ball->hit.speed.z - offset.z - work->pos.z;
            length = njScalor(&position);
            if (length != 0.0F) {
                njUnitVector(&position);
                dot = njInnerProduct(&position, &ball->hit.normal);
                if (dot < -0.8F || length > record->consts.height) {
                    mask = 0;
                }
            }
        }
        mask |= 4;
        /* @hack redundant: ang, set five times and never read, whose reads
         * keep 0.0F in FR15 and R9's copy at R15+20 (0x8c0775c6, 0x8c0775d0);
         * without them, 77 of 1781 words differ */
        ang = entry->flags;
        ang = entry->object != NULL;
        ang = entry->owner != NULL;
        ang = ball->hit.ang_x;
        ang = ball->hit.ang_z;
        if ((work->flags & PLAYER_ON_GROUND) &&
            !(work->flags & PLAYER_ON_PATH)) {
            NJS_VECTOR push;
            NJS_VECTOR move;

            push.x = ball->hit.pos.x + ball->hit.speed.x - ball->pos.x;
            push.y = 0.0F;
            push.z = ball->hit.pos.z + ball->hit.speed.z - ball->pos.z;
            if (njScalor(&push) != 0.0F) {
                njUnitVector(&push);
                move = motion->speed;
                move.y = 0.0F;
                if (njScalor(&move) != 0.0F) {
                    njUnitVector(&move);
                    if (njInnerProduct(&push, &move) < 0.0F) {
                        mask &= ~4;
                    }
                }
            }
        }
        ang = vector_angle(&ball->normal, &ball->hit.normal, NULL);
        if (ang > FACING_TURN) {
            player_step_up(work, ball, entry);
            ang = vector_angle(&ball->normal, &ball->hit.normal, NULL);
            mask |= 2;
        }
        if (!(work->flags & PLAYER_ON_TASK) && !(entry->flags & 0x200040) &&
            ((ang <= FACING_TURN && (mask & 5) == 5) ||
             njInnerProduct(&world_gravity, &ball->normal) >
                 njInnerProduct(&world_gravity, &ball->hit.normal) ||
             ((entry->flags & WORLD_NO_SLOPE) && (mask & 5) == 5)) &&
            (!(old_flags & PLAYER_ON_GROUND) ||
             (record->last_flags & PLAYER_ON_GROUND)) &&
            (njInnerProduct(&world_gravity, &ball->hit.normal) <= 0.0F ||
             (record->last_flags & PLAYER_ON_GROUND)) &&
            ((mask & 2) || player_step_up(work, ball, entry))) {
            if (njScalor(&motion->speed) >= record->consts.running_speed &&
                !(entry->flags & WORLD_NO_SLOPE)) {
                position = motion->speed;
                njUnitVector(&position);
                if (njInnerProduct(&position, &ball->hit.normal) > 0.52F) {
                    result =
                        player_hit_wall(work, motion, record, ball, &old_pos,
                                        &offset, result, entry->flags);
                    continue;
                }
            }
            if (record->speed.y <= 0.0F ||
                (record->speed.y > 0.0F &&
                 !(record->last_flags & PLAYER_ON_GROUND))) {
                NJS_PLANE plane;
                NJS_POINT3 point;
                struct world_object swap;

                grounded = TRUE;
                object = entry->object;
                found = ball_slide_along_walls(work, motion, record, ball,
                                               entry->flags);
                if (found) {
                    if (!result) {
                        result = found;
                    } else if (found == PLAYER_CRASHED) {
                        result = found;
                    }
                }
                if (entry->flags & WORLD_NO_SLOPE) {
                    work->ang.x = work->ang.z = 0;
                } else if ((entry->flags & (WORLD_CLING | WORLD_SLIPPERY)) ||
                           (record->last_flags & PLAYER_ON_PATH)) {
                    ball->hit.ang_z =
                        -(int)(atan2f(ball->hit.normal.x, ball->hit.normal.y) *
                               ANGLE_PER_RADIAN);
                    ball->hit.ang_x =
                        (int)(asinf(ball->hit.normal.z) * ANGLE_PER_RADIAN);
                    if (motion->speed.x != 0.0F || motion->speed.z != 0.0F ||
                        !(record->last_flags & PLAYER_ON_LAND) ||
                        njInnerProduct(&world_gravity, &ball->hit.normal) <
                            njInnerProduct(&world_gravity, &ball->normal)) {
                        player_tilt_to_ground(work, record, ball);
                    }
                } else {
                    ball->hit.ang_x =
                        (int)(asinf(ball->hit.normal.z) * ANGLE_PER_RADIAN);
                    if (ball->hit.normal.y >= 0.0F) {
                        ball->hit.ang_z = -(int)(asinf(ball->hit.normal.x) *
                                                 ANGLE_PER_RADIAN);
                    } else {
                        ball->hit.ang_z = (int)(asinf(ball->hit.normal.x) *
                                                ANGLE_PER_RADIAN) +
                                          0x8000;
                    }
                    if (motion->speed.x != 0.0F || motion->speed.z != 0.0F ||
                        !(record->last_flags & PLAYER_ON_LAND) ||
                        njInnerProduct(&world_gravity, &ball->hit.normal) <
                            njInnerProduct(&world_gravity, &ball->normal)) {
                        pitch =
                            (int)(asinf(ball->hit.normal.z) * ANGLE_PER_RADIAN);
                        roll = -(int)(asinf(ball->hit.normal.x) *
                                      ANGLE_PER_RADIAN);
                        if (ball->hit.normal.y < 0.0F) {
                            other_pitch = 0x8000 - pitch;
                            other_roll = 0x8000 - roll;
                            pitch_distance =
                                angle_distance(work->ang.x, other_pitch);
                            roll_distance =
                                angle_distance(work->ang.z, other_roll);
                            if (pitch_distance < roll_distance) {
                                ball->hit.ang_x = other_pitch;
                                ball->hit.ang_z = roll;
                            } else {
                                ball->hit.ang_x = pitch;
                                ball->hit.ang_z = other_roll;
                            }
                        }
                        player_tilt_to_ground(work, record, ball);
                    }
                }
                if (!result) {
                    ball->normal.x = ball->normal.z = 0.0F;
                    ball->normal.y = 1.0F;
                    ball->hit.ang_x = work->ang.x;
                    ball->hit.ang_z = work->ang.z;
                    if (entry->flags & WORLD_NO_SLOPE) {
                        ground_turn_to_world(&ball->hit.normal, &ball->normal,
                                             work->ang.y);
                    } else {
                        player_turn_to_world(work, &ball->normal);
                    }
                    offset.x = offset.z = 0.0F;
                    offset.y = record->consts.height * 0.5F;
                    normal = offset;
                    if (entry->flags & WORLD_NO_SLOPE) {
                        ground_turn_to_world(&ball->hit.normal, &offset,
                                             work->ang.y);
                    } else {
                        player_turn_to_world(work, &offset);
                    }
                    ball_drop(motion, record, ball, &normal);
                    if (entry->flags & WORLD_NO_SLOPE) {
                        ground_turn_to_world(&ball->hit.normal, &normal,
                                             work->ang.y);
                    } else {
                        player_turn_to_world(work, &normal);
                    }
                }
                motion->aim.x = ball->hit.ang_x;
                motion->aim.z = ball->hit.ang_z;
                record->ground_normal = ball->hit.normal;
                if (entry->flags & WORLD_NO_SLOPE) {
                    position = ball->hit.speed;
                    if (njScalor(&position) != 0.0F) {
                        length = njScalor(&ball->speed);
                        njUnitVector(&position);
                        ball->hit.speed.x = position.x * length;
                        ball->hit.speed.y = position.y * length;
                        ball->hit.speed.z = position.z * length;
                    }
                }
                /* @hack redundant: a block that never runs, behind a 0 &&
                 * that SHC doesn't fold away. the image tests R9, which
                 * holds 0, at 0x8c07822c, and keeps the block, with its test
                 * of the hit speed's length against 2.1 at 0x8c078244. under
                 * a plain 0, the function runs 224 bytes short and 649 of
                 * 1781 words differ, and without it, 288 bytes short and
                 * 1419 of 1781 words differ */
                if (0 && (magnitude = njScalor(&ball->hit.speed)) > 2.1F) {
                    njUnitVector(&ball->hit.speed);
                    magnitude = njScalor(&ball->speed);
                    excess = magnitude - 2.1F;
                    ball->speed.x = ball->hit.speed.x * excess;
                    ball->speed.y = ball->hit.speed.y * excess;
                    ball->speed.z = ball->hit.speed.z * excess;
                    magnitude = 2.1F;
                    ball->hit.speed.x *= magnitude;
                    ball->hit.speed.y *= magnitude;
                    ball->hit.speed.z *= magnitude;
                    swap = list[k];
                    list[k] = *entry;
                    *entry = swap;
                    i = -1;
                } else {
                    ball->speed.x = ball->speed.y = ball->speed.z = 0.0F;
                }
                ball->pos.x = ball->hit.pos.x + ball->hit.speed.x;
                ball->pos.y = ball->hit.pos.y + ball->hit.speed.y;
                ball->pos.z = ball->hit.pos.z + ball->hit.speed.z;
                contact = &polygon_contacts[ball->hit_index];
                plane.vx = ball->normal.x;
                plane.vy = ball->normal.y;
                plane.vz = ball->normal.z;
                plane.px = contact->hit.point.x;
                plane.py = contact->hit.point.y;
                plane.pz = contact->hit.point.z;
                njDistanceP2PL(&ball->pos, &plane, &point);
                ball->pos.x = offset.x + point.x;
                ball->pos.y = offset.y + point.y;
                ball->pos.z = offset.z + point.z;
                if (owner != NULL) {
                    record->ground_task = owner;
                    player_ride_task(work, record, ball, &offset);
                } else {
                    record->ground_task = NULL;
                }
                position.x = ball->pos.x - offset.x;
                position.y = ball->pos.y - offset.y;
                position.z = ball->pos.z - offset.z;
                gap = njDistanceP2P(&work->pos, &position);
                if (owner != NULL || record->speed.x != 0.0F ||
                    record->speed.z != 0.0F || gap > 1.0F) {
                    if (gap > 0.0001F) {
                        work->pos = position;
                    }
                }
                if (record->speed.x >= 1.85F || record->speed.x <= -1.85F ||
                    njInnerProduct(&world_gravity, &ball->normal) < -0.34F ||
                    (entry->flags & WORLD_NO_SLOPE)) {
                    work->flags |= PLAYER_ON_LAND;
                    player_set_ground(work, motion, record, entry->flags);
                } else if ((record->speed.x >= 1.16F ||
                            record->speed.x <= -1.16F) &&
                           njInnerProduct(&world_gravity, &ball->normal) <
                               -0.34F) {
                    work->flags |= PLAYER_ON_LAND;
                    player_set_ground(work, motion, record, entry->flags);
                } else {
                    player_set_ground(work, motion, record, 0);
                    if (record->speed.x > 0.0F) {
                        player_slip_off(work, motion, ball);
                    }
                }
            } else {
                result = player_hit_wall(work, motion, record, ball, &old_pos,
                                         &offset, result, entry->flags);
                continue;
            }
        } else {
            result = player_hit_wall(work, motion, record, ball, &old_pos,
                                     &offset, result, entry->flags);
        }
    }
    if (grounded != TRUE) {
        stroke = 0.0F;
        if ((record->last_flags & PLAYER_ON_LAND) &&
            !(record->last_flags & PLAYER_EVENT_DRIVEN) &&
            (!player_read_stick(work, NULL, &stroke) ||
             (stroke <= 0.9F && !record->launch_steps)) &&
            record->speed.y <= 0.0F &&
            record->speed.x <= record->consts.running_speed &&
            !(record->last_ground_flags & WORLD_SLIPPERY) &&
            record->level > 0.7F) {
            work->pos.x -= motion->speed.x * 0.5F;
            work->pos.y -= motion->speed.y * 0.5F;
            work->pos.z -= motion->speed.z * 0.5F;
            motion->speed.x = 0.0F;
            motion->speed.y = 0.0F;
            motion->speed.z = 0.0F;
        } else {
            work->pos.x += ball->speed.x;
            work->pos.y += ball->speed.y;
            work->pos.z += ball->speed.z;
        }
        player_set_ground(work, motion, record, 0);
    } else {
        record->ground_object = object;
        work->pos.x += ball->speed.x;
        work->pos.y += ball->speed.y;
        work->pos.z += ball->speed.z;
    }
    return result;
}

static Bool player_step_up(struct task_work *work, struct polygon_ball *ball,
                           struct world_object *world)
{
    struct task_motion *motion;
    struct player *record;
    struct polygon_ball saved;
    NJS_VECTOR head;
    NJS_VECTOR foot;
    NJS_VECTOR dir;
    int ang;
    Float length;

    motion = player_motion[PLAYER_VARS(work)->player];
    record = player_records[PLAYER_VARS(work)->player];
    saved = *ball;
    foot.x = foot.z = 0.0F;
    foot.y = 4.0F;
    if (no_slope_near && PLAYER_VARS(work)->player == 5) {
        foot.y = step_up_foot;
    }
    ball_drop(motion, record, ball, &foot);
    if (!no_slope_near || (work->flags & PLAYER_ON_PATH)) {
        player_turn_to_world(work, &foot);
    } else {
        ground_turn_to_world(&ball->hit.normal, &foot, work->ang.y);
    }
    head.x = head.z = 0.0F;
    head.y = ball->radius;
    if (no_slope_near && PLAYER_VARS(work)->player == 5) {
        head.y = step_up_head;
    }
    if (!no_slope_near || (work->flags & PLAYER_ON_PATH)) {
        player_turn_to_world(work, &head);
    } else {
        ground_turn_to_world(&ball->hit.normal, &head, work->ang.y);
    }
    if (!no_slope_near || (work->flags & PLAYER_ON_PATH)) {
        ball->pos.x = ball->hit.pos.x - ball->pos.x + work->pos.x + foot.x;
        ball->pos.y = ball->hit.pos.y - ball->pos.y + work->pos.y + foot.y;
        ball->pos.z = ball->hit.pos.z - ball->pos.z + work->pos.z + foot.z;
    } else if (world->flags & WORLD_NO_SLOPE) {
        ball->pos.x = ball->hit.speed.x + ball->hit.pos.x - head.x + foot.x;
        ball->pos.y = ball->hit.speed.y + ball->hit.pos.y - head.y + foot.y;
        ball->pos.z = ball->hit.speed.z + ball->hit.pos.z - head.z + foot.z;
        ball->speed.x = ball->speed.y = ball->speed.z = 0.0F;
    } else {
        ball->pos.x = ball->hit.pos.x - head.x + foot.x;
        ball->pos.y = ball->hit.pos.y - head.y + foot.y;
        ball->pos.z = ball->hit.pos.z - head.z + foot.z;
        ang = vector_angle(&ball->normal, &ball->hit.normal, NULL);
        if (ang >= FACING_TURN) {
            dir = ball->hit.speed;
            if (njScalor(&dir) >= 1.0F) {
                njUnitVector(&dir);
                length = njScalor(&ball->speed);
                ball->speed.x = dir.x * length;
                ball->speed.y = dir.y * length;
                ball->speed.z = dir.z * length;
            }
        }
    }
    if (record->speed.x >= record->consts.full_speed ||
        (record->last_ground_flags & WORLD_NO_SLOPE) ||
        (record->speed.y <= 0.0F && (world->flags & WORLD_NO_SLOPE))) {
        ball->radius = 5.1F;
    } else if (record->consts.trot_speed <= record->speed.x &&
               record->speed.x <= record->consts.full_speed) {
        ball->radius =
            ((record->speed.x - record->consts.trot_speed) /
             (record->consts.full_speed - record->consts.trot_speed)) +
            4.1F;
    } else if (record->speed.x < record->consts.trot_speed) {
        /* @hack redundant: both arms set 4.1F, but the image still tests the
         * speed (FCMP/GT at 0x8c078b1a); as one store, 67 of 469 words
         * differ */
        ball->radius = 4.1F;
    } else {
        ball->radius = 4.1F;
    }
    if (debug_hidden >= 0) {
        njPushMatrix(NULL);
        njTranslate(NULL, ball->pos.x, ball->pos.y, ball->pos.z);
        draw_set_material(0.1F, 1.0F, 0.4F, 0.8F);
        njScale(NULL, 0.41F, 0.41F, 0.41F);
        draw_tree(&marker_object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
    }
    if (!polygon_sweep(ball, world->object, world->flags)) {
        *ball = saved;
        return FALSE;
    }
    ang = vector_angle(&ball->normal, &ball->hit.normal, NULL);
    if ((ang <= FACING_TURN ||
         njInnerProduct(&world_gravity, &ball->normal) >
             njInnerProduct(&world_gravity, &ball->hit.normal) ||
         (world->flags & WORLD_NO_SLOPE)) &&
        (njInnerProduct(&world_gravity, &ball->hit.normal) <= 0.0F ||
         (record->last_flags & PLAYER_ON_GROUND))) {
        ball->radius = saved.radius;
        ball->speed = saved.speed;
    } else {
        *ball = saved;
        return FALSE;
    }
    return TRUE;
}

static Uint32 player_hit_wall(struct task_work *work,
                              struct task_motion *motion, struct player *record,
                              struct polygon_ball *ball, NJS_POINT3 *old_pos,
                              NJS_VECTOR *offset, Uint32 result, Uint32 flags)
{
    NJS_VECTOR dir;
    int ang;
    Float dot;
    Float bound;

    record->wall_normal = ball->hit.normal;
    if (work->flags & PLAYER_ON_LAND) {
        dir.x = old_pos->x - work->pos.x;
        dir.y = old_pos->y - work->pos.y;
        dir.z = old_pos->z - work->pos.z;
        if (njScalor(&dir) != 0.0F) {
            njUnitVector(&dir);
            ang = vector_angle(&dir, &ball->hit.normal, NULL);
            if (flags & WORLD_DEFLECT) {
                if (ang > 0x4000) {
                    return result;
                }
                result = 1;
                player_deflect(work, record, ball, &dir);
            } else if (record->speed.x >= 15.0F) {
                result = PLAYER_CRASHED;
                ball->hit.speed.x = ball->hit.speed.y = ball->hit.speed.z =
                    0.0F;
                player_clear_speed(NULL, record);
            } else if (ang <= 0x1000) {
                if (record->speed.x >= record->consts.crash_speed) {
                    result = PLAYER_CRASHED;
                } else {
                    result = 1;
                    player_deflect(work, record, ball, &dir);
                }
            } else if (ang <= 0x4000) {
                result = 1;
                player_deflect(work, record, ball, &dir);
            }
        }
    } else {
        bound = 4.0F;
        dir.x = -motion->speed.x;
        dir.y = -motion->speed.y;
        dir.z = -motion->speed.z;
        if (njScalor(&dir) != 0.0F) {
            njUnitVector(&dir);
            ang = vector_angle(&dir, &ball->hit.normal, NULL);
            dot = njInnerProduct(&dir, &ball->hit.normal);
            if (flags & WORLD_DEFLECT) {
                if (ang > 0x4000) {
                    return result;
                }
                result = 1;
                player_deflect(work, record, ball, &dir);
            } else if (record->speed.x >= 15.0F && dot >= bound) {
                result = PLAYER_CRASHED;
                ball->hit.speed.x = ball->hit.speed.y = ball->hit.speed.z =
                    0.0F;
                player_clear_speed(NULL, record);
            } else if (dot >= bound) {
                if (record->speed.x >= record->consts.crash_speed) {
                    result = PLAYER_CRASHED;
                } else {
                    result = 1;
                }
            } else if (PLAYER_VARS(work)->character == CHARACTER_KNUCKLES &&
                       ang <= 0x3400 && !(flags & 0x201000)) {
                result = 1;
            } else if (ang >= 0x4000) {
                return result;
            } else {
                player_deflect(work, record, ball, &dir);
            }
        }
    }
    ball->pos = ball->hit.pos;
    work->pos.x = ball->hit.pos.x - offset->x;
    work->pos.y = ball->hit.pos.y - offset->y;
    work->pos.z = ball->hit.pos.z - offset->z;
    ball->speed = ball->hit.speed;
    return result;
}

static Uint32 pinball_hit_wall(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record, struct polygon_ball *ball,
                               NJS_POINT3 *old_pos, NJS_VECTOR *offset,
                               Uint32 result, Uint32 flags)
{
    NJS_VECTOR dir;
    int ang;

    record->wall_normal = ball->hit.normal;
    if (work->flags & PLAYER_ON_LAND) {
        dir.x = old_pos->x - work->pos.x;
        dir.y = old_pos->y - work->pos.y;
        dir.z = old_pos->z - work->pos.z;
        if (njScalor(&dir) != 0.0F) {
            njUnitVector(&dir);
            ang = vector_angle(&dir, &ball->hit.normal, NULL);
            if (ang < 0x4000) {
                result = 1;
                pinball_deflect(work, record, ball, &dir);
            } else {
                return result;
            }
        } else {
            dir.x = -motion->speed.x;
            dir.y = -motion->speed.y;
            dir.z = -motion->speed.z;
            if (njScalor(&dir) == 0.0F) {
                return result;
            }
            njUnitVector(&dir);
            ang = vector_angle(&dir, &ball->hit.normal, NULL);
            if (ang < 0x4000) {
                result = 1;
                pinball_deflect(work, record, ball, &dir);
            } else {
                return result;
            }
        }
    } else {
        dir.x = -motion->speed.x;
        dir.y = -motion->speed.y;
        dir.z = -motion->speed.z;
        if (njScalor(&dir) != 0.0F) {
            njUnitVector(&dir);
            ang = vector_angle(&dir, &ball->hit.normal, NULL);
            njInnerProduct(&dir, &ball->hit.normal);
            if (flags & WORLD_DEFLECT) {
                if (ang < 0x4000) {
                    result = 1;
                    pinball_deflect(work, record, ball, &dir);
                } else {
                    return result;
                }
            } else {
                if (ang >= 0x4000) {
                    return result;
                } else {
                    pinball_deflect(work, record, ball, &dir);
                }
            }
        }
    }
    ball->pos = ball->hit.pos;
    work->pos.x = ball->hit.pos.x - offset->x;
    work->pos.y = ball->hit.pos.y - offset->y;
    work->pos.z = ball->hit.pos.z - offset->z;
    ball->speed = ball->hit.speed;
    return result;
}

static void ball_drop(struct task_motion *motion, struct player *record,
                      struct polygon_ball *ball, NJS_VECTOR *position)
{
    NJS_VECTOR speed;
    Float reach;
    Float level;

    reach = record->consts.snap;
    position->x = position->z = 0.0F;
    if ((record->last_ground_flags & WORLD_NO_SLOPE) &&
        motion->speed.y <= 0.0F) {
        position->y -= reach * 0.6F;
    } else if ((record->last_ground_flags & WORLD_NO_SLOPE) &&
               motion->speed.y > 0.0F) {
        position->y -= reach * 0.2F;
    } else if (record->last_ground_flags & WORLD_CLING) {
        position->y -= reach;
    } else if (record->speed.y <= 0.0F) {
        if (record->last_ground_flags & WORLD_GRIP_LOW) {
            /* @hack redundant: both arms take 0.3 of the reach, but the image
             * still tests the flags (TST #3 at 0x8c0792ae); as one product,
             * 79 of 197 words differ */
            if (record->last_flags & PLAYER_ON_GROUND) {
                reach *= 0.3F;
            } else {
                reach *= 0.3F;
            }
        } else {
            level = -njInnerProduct(&world_gravity, &ball->normal);
            if (record->last_flags & PLAYER_ON_PATH) {
                if (level <= -0.6F) {
                    reach *= 1.8F;
                } else if (level <= 0.05F) {
                    reach *= 1.65F;
                } else {
                    reach *= 1.4F;
                }
            } else if (record->speed.x < record->consts.full_speed ||
                       !(record->last_flags & PLAYER_ON_GROUND)) {
                if (level > -0.1F) {
                    if (level > -0.1F && level < 0.2F) {
                        speed = motion->speed;
                        if (njScalor(&speed) == 0.0F) {
                            speed.y = 0.0F;
                        } else {
                            njUnitVector(&speed);
                        }
                        if (0.25F > _builtin_fabsf(speed.y)) {
                            reach *= 1.5F;
                        } else {
                            reach *= 0.3F;
                        }
                    } else if (record->speed.x <=
                               record->consts.running_speed) {
                        reach *= 0.5F;
                    } else if (record->last_flags & PLAYER_ON_GROUND) {
                        reach *= record->speed.x / record->consts.full_speed;
                    }
                }
            }
        }
        position->y -= reach;
    } else if (record->last_flags & PLAYER_ON_PATH) {
        position->y -= reach;
    }
}

static void player_tilt_to_ground(struct task_work *work, struct player *record,
                                  struct polygon_ball *ball)
{
    NJS_VECTOR normal;
    Angle step;

    if (njInnerProduct(&world_gravity, &ball->normal) >
            njInnerProduct(&world_gravity, &ball->hit.normal) ||
        !(record->last_flags & PLAYER_ON_LAND) ||
        (record->last_flags & PLAYER_ON_PATH)) {
        work->ang.x = ball->hit.ang_x;
        work->ang.z = ball->hit.ang_z;
        return;
    }
    normal = ball->hit.normal;
    player_turn_back(work, &normal);
    normal.x = njSqrt((normal.z * normal.z) + (normal.x * normal.x));
    normal.z = 0.0F;
    step = (Angle)(atan2f(normal.x, normal.y) * ANGLE_PER_RADIAN) & 0xffff;
    if (step > 0x8000) {
        step = 0x10000 - step;
    }
    if (!(work->flags & PLAYER_ON_PATH) &&
        record->speed.x < record->consts.full_speed) {
        if (record->speed.x >= record->consts.sprint_speed) {
            if (step > 0x2000) {
                step = 0x2000;
            }
        } else if (record->speed.x >= record->consts.running_speed) {
            if (step > 0x1500) {
                step = 0x1500;
            }
        } else if (record->speed.x >= record->consts.trot_speed) {
            if (step > 0x1000) {
                step = 0x1000;
            }
        } else {
            step = 0x400;
        }
    }
    work->ang.x = angle_approach(work->ang.x, ball->hit.ang_x, step);
    work->ang.z = angle_approach(work->ang.z, ball->hit.ang_z, step);
}

static void player_ride_task(struct task_work *work, struct player *record,
                             struct polygon_ball *ball, NJS_VECTOR *offset)
{
    struct task_force *force = record->ground_task->force;
    NJS_POINT3 pos;

    if (force != NULL) {
        if (PLAYER_VARS(work)->player) {
            force++;
        }
        if (njScalor(&record->speed) == 0.0F) {
            ball->pos.x = work->pos.x + offset->x;
            ball->pos.y = work->pos.y + offset->y;
            ball->pos.z = work->pos.z + offset->z;
        }
        if (force->carry != NULL) {
            pos = work->pos;
            work->pos = ball->pos;
            force->carry(player_tasks[PLAYER_VARS(work)->player], work, force);
            work->pos = pos;
        }
        ball->pos.x += force->speed.x;
        ball->pos.y += force->speed.y;
        ball->pos.z += force->speed.z;
        work->ang.x = (Uint16)(work->ang.x + force->turn.x);
        work->ang.y = (Uint16)(work->ang.y + force->turn.y);
        work->ang.z = (Uint16)(work->ang.z + force->turn.z);
    }
}

static void player_slip_off(struct task_work *work, struct task_motion *motion,
                            struct polygon_ball *ball)
{
    work->pos.x += 3.0F * ball->hit.front.x;
    work->pos.y += 3.0F * ball->hit.front.y;
    work->pos.z += 3.0F * ball->hit.front.z;
    motion->speed.x += 2.0F * ball->hit.front.x;
    motion->speed.z += 2.0F * ball->hit.front.z;
}

static void player_set_ground(struct task_work *work,
                              struct task_motion *motion, struct player *record,
                              Uint32 flags)
{
    record->ground_flags |= flags;

    if (record->shadow.above_flags & WORLD_WATER) {
        if (PLAYER_VARS(work)->character == CHARACTER_AMY) {
            record->consts.gravity =
                character_constants[PLAYER_VARS(work)->character].gravity *
                0.5F;
        } else {
            record->consts.gravity =
                character_constants[PLAYER_VARS(work)->character].gravity *
                0.45F;
        }
        record->consts.resist.y =
            character_constants[PLAYER_VARS(work)->character].resist.y * 1.5F;
        record->consts.ground_accel =
            character_constants[PLAYER_VARS(work)->character].ground_accel *
            0.65F;
    } else if (record->shadow.above_flags & WORLD_THICK_LIQUID) {
        record->consts.gravity =
            character_constants[PLAYER_VARS(work)->character].gravity * 0.6F;
        record->consts.resist.y =
            character_constants[PLAYER_VARS(work)->character].resist.y * 2.0F;
        record->consts.ground_accel =
            character_constants[PLAYER_VARS(work)->character].ground_accel *
            0.45F;
    } else if ((record->shadow.above_flags & WORLD_WATER) == 0) {
        record->consts.gravity =
            character_constants[PLAYER_VARS(work)->character].gravity;
        record->consts.resist.y =
            character_constants[PLAYER_VARS(work)->character].resist.y;
        record->consts.ground_accel =
            character_constants[PLAYER_VARS(work)->character].ground_accel;
    }

    if (flags & WORLD_GRIP_LOW) {
        motion->gravity = 0.15F;
    } else if (flags & WORLD_GRIP_LEAST) {
        motion->gravity = 0.1F;
    } else if (flags & WORLD_SAND) {
        motion->gravity = 0.5F;
    } else if (flags & WORLD_GRIP_HIGH) {
        motion->gravity = 1.5F;
    } else {
        motion->gravity = 1.0F;
    }
}

static Bool ball_stop_at_walls(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record, struct polygon_ball *ball,
                               Uint32 flags)
{
    Float distances[16];
    int contact_flags[16];
    NJS_VECTOR normal_sum;
    NJS_VECTOR direction;
    int i;
    int ang;
    int count = 0;
    int nearest = -1;
    Float minimum = 1000.0F;
    Float speed;
    Float dot;
    Float grip;
    struct polygon_contact *contact;

    normal_sum.x = normal_sum.y = normal_sum.z = 0.0F;
    for (i = 0; i < ball->count; ++i) {
        contact = &polygon_contacts[i];
        distances[i] = njDistanceP2P(&contact->hit.pos, &ball->pos);
        contact_flags[i] = 0;
        ang = vector_angle(&ball->normal, &contact->hit.normal, NULL);
        if (ang <= FACING_TURN ||
            njInnerProduct(&world_gravity, &ball->normal) >
                njInnerProduct(&world_gravity, &contact->hit.normal)) {
            contact_flags[i] |= CONTACT_GROUND;
        } else {
            direction = ball->speed;
            if (njScalor(&direction) != 0.0F) {
                njUnitVector(&direction);
            }
            if ((dot = njInnerProduct(&direction, &contact->hit.normal)) <
                0.0F) {
                contact_flags[i] |= CONTACT_WALL;
                ++count;
                if (distances[i] < minimum) {
                    minimum = distances[i];
                    nearest = i;
                }
                if (!(flags & 0x800000)) {
                    if (dot < -njCos(0x600)) {
                        motion->speed.x = motion->speed.y = motion->speed.z =
                            0.0F;
                    }
                    speed = njInnerProduct(&ball->speed, &contact->hit.normal);
                    if (speed < -4.5F) {
                        grip = record->consts.running_speed;
                        if (njScalor(&motion->speed) > grip) {
                            njUnitVector(&motion->speed);
                            motion->speed.x *= grip;
                            motion->speed.y *= grip;
                            motion->speed.z *= grip;
                        }
                    }
                }
                njAddVector(&normal_sum, &contact->hit.normal);
            }
        }
    }
    if (count > 0) {
        speed = njScalor(&ball->speed);
        if (speed > distances[nearest]) {
            if (njScalor(&ball->speed) == 0.0F) {
                ball->speed.x = ball->speed.y = ball->speed.z = 0.0F;
            } else {
                njUnitVector(&ball->speed);
                ball->speed.x *= distances[nearest];
                ball->speed.y *= distances[nearest];
                ball->speed.z *= distances[nearest];
            }
            return TRUE;
        }
    }
    return FALSE;
}

static Bool pinball_stop_at_walls(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record,
                                  struct polygon_ball *ball, Uint32 flags)
{
    Float distances[16];
    int contact_flags[16];
    NJS_VECTOR direction;
    int i;
    int ang;
    int count = 0;
    int nearest = -1;
    Float minimum = 1000.0F;
    Float speed;
    struct polygon_contact *contact;

    for (i = 0; i < ball->count; ++i) {
        contact = &polygon_contacts[i];
        distances[i] = njDistanceP2P(&contact->hit.pos, &ball->pos);
        contact_flags[i] = 0;
        ang = vector_angle(&ball->normal, &contact->hit.normal, NULL);
        if (ang <= FACING_TURN ||
            njInnerProduct(&world_gravity, &ball->normal) >
                njInnerProduct(&world_gravity, &contact->hit.normal)) {
            contact_flags[i] |= CONTACT_GROUND;
            /* @hack redundant: the continue in an if (1), whose jump SHC
             * places before deleting it: the flag's store fills the BRA's
             * delay slot (0x8c079d64) and direction copies through R0 and R1
             * (0x8c079da2); as a bare continue, 12 of 195 words differ */
            if (1) {
                continue;
            }
        }
        direction = ball->speed;
        if (njScalor(&direction) != 0.0F) {
            njUnitVector(&direction);
        }
        if (ang >= 0x7000) {
            continue;
        }
        if (njInnerProduct(&direction, &contact->hit.normal) < 0.0F) {
            contact_flags[i] |= CONTACT_WALL;
            ++count;
            if (distances[i] < minimum) {
                minimum = distances[i];
                nearest = i;
            }
        }
    }
    if (count > 0) {
        speed = njScalor(&ball->speed);
        if (speed > distances[nearest]) {
            if (njScalor(&ball->speed) == 0.0F) {
                ball->speed.x = ball->speed.y = ball->speed.z = 0.0F;
            } else {
                njUnitVector(&ball->speed);
                ball->speed.x *= distances[nearest];
                ball->speed.y *= distances[nearest];
                ball->speed.z *= distances[nearest];
            }
            return TRUE;
        }
    }
    return FALSE;
}

static Bool ball_stop_at_ground(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record,
                                struct polygon_ball *ball)
{
    Float distances[16];
    int contact_flags[16];
    NJS_VECTOR direction;
    int i;
    int ang;
    int count = 0;
    int nearest = -1;
    Float minimum = 1000.0F;
    Float speed;
    struct polygon_contact *contact;

    for (i = 0; i < ball->count; ++i) {
        contact = &polygon_contacts[i];
        distances[i] = njDistanceP2P(&contact->hit.pos, &ball->pos);
        contact_flags[i] = 0;
        ang = vector_angle(&ball->normal, &contact->hit.normal, NULL);
        if (ang <= FACING_TURN ||
            njInnerProduct(&world_gravity, &ball->normal) >
                njInnerProduct(&world_gravity, &contact->hit.normal)) {
            contact_flags[i] |= CONTACT_GROUND;
            ++count;
            if (distances[i] < minimum) {
                minimum = distances[i];
                nearest = i;
            }
        } else {
            direction = ball->speed;
            if (njScalor(&direction) != 0.0F) {
                njUnitVector(&direction);
            }
            if (njInnerProduct(&direction, &contact->hit.normal) < -0.2F) {
                contact_flags[i] |= CONTACT_WALL;
            }
        }
    }
    if (count > 0) {
        speed = njScalor(&ball->speed);
        if (speed > distances[nearest]) {
            if (njScalor(&ball->speed) == 0.0F) {
                ball->speed.x = ball->speed.y = ball->speed.z = 0.0F;
            } else {
                njUnitVector(&ball->speed);
                ball->speed.x *= distances[nearest];
                ball->speed.y *= distances[nearest];
                ball->speed.z *= distances[nearest];
            }
            return TRUE;
        }
    }
    return FALSE;
}

static int ball_slide_along_walls(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record,
                                  struct polygon_ball *ball, Uint32 flags)
{
    NJS_PLANE first;
    NJS_PLANE second;
    union line_view line;
    NJS_VECTOR axis;
    NJS_VECTOR reverse_speed;
    NJS_VECTOR normal_sum;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_VECTOR normal;
    Float distances[16];
    int contact_flags[16];
    NJS_VECTOR direction;
    int i;
    int ang;
    int count = 0;
    int nearest = -1;
    Float minimum = 1000.0F;
    Float speed;
    Float dot;
    Float scale;
    struct polygon_contact *contact;
    int ignore_wall;

    normal_sum.x = normal_sum.y = normal_sum.z = 0.0F;
    /* @hack redundant: normal, a copy of normal_sum, is never read, but the
     * image makes the copy (MOV.L at 0x8c07a128 to 0x8c07a136); without it,
     * 549 of 551 words differ */
    normal = normal_sum;
    /* @hack spelling: i set ahead of ignore_wall, which keeps i's MOV #0,R13 at
     * 0x8c07a10a; set in the for, 32 of 551 words differ */
    i = 0;
    ignore_wall = flags & WORLD_NO_SLOPE;
    for (; i < ball->count; ++i) {
        contact = &polygon_contacts[i];
        distances[i] = njDistanceP2P(&contact->hit.pos, &ball->pos);
        contact_flags[i] = 0;
        ang = vector_angle(&ball->normal, &contact->hit.normal, NULL);
        if (ang <= FACING_TURN ||
            njInnerProduct(&world_gravity, &ball->normal) >
                njInnerProduct(&world_gravity, &contact->hit.normal) ||
            ignore_wall) {
            contact_flags[i] |= CONTACT_GROUND;
        } else {
            direction = ball->speed;
            if (njScalor(&direction) != 0.0F) {
                njUnitVector(&direction);
            }
            if ((dot = njInnerProduct(&direction, &contact->hit.normal)) <
                0.0F) {
                speed = njInnerProduct(&ball->speed, &contact->hit.normal);
                if (dot < -njCos(0x600) || speed < -4.5F) {
                    contact_flags[i] |= CONTACT_WALL;
                    ++count;
                    if (distances[i] < minimum) {
                        minimum = distances[i];
                        nearest = i;
                    }
                }
                njAddVector(&normal_sum, &contact->hit.normal);
            }
        }
    }
    if (count <= 0) {
        return 0;
    }
    speed = njScalor(&ball->speed);
    if (!(speed > distances[nearest]) ||
        (record->last_ground_flags & WORLD_NO_SLOPE)) {
        return 0;
    }
    if (njScalor(&ball->hit.speed) == 0.0F) {
        ball->hit.speed.x = ball->hit.speed.y = ball->hit.speed.z = 0.0F;
        return 0;
    } else {
        /* @hack redundant: the rest in an else after the return, which loads
         * njUnitVector's address into R2 (0x8c07a320); after the if, into
         * R3, and 2 of 551 words differ */
        njUnitVector(&ball->hit.speed);
        ball->hit.speed.x *= distances[nearest];
        ball->hit.speed.y *= distances[nearest];
        ball->hit.speed.z *= distances[nearest];
        contact = &polygon_contacts[nearest];
        record->wall_normal = contact->hit.normal;
        reverse_speed.x = -motion->speed.x;
        reverse_speed.y = -motion->speed.y;
        reverse_speed.z = -motion->speed.z;
        if (njScalor(&reverse_speed) == 0.0F) {
            return 0;
        }
        njUnitVector(&reverse_speed);
        ang = vector_angle(&reverse_speed, &record->wall_normal, NULL);
        if (ang <= 0x1000) {
            if (record->speed.x >= record->consts.crash_speed) {
                ball->hit.speed.x = ball->hit.speed.y = ball->hit.speed.z =
                    0.0F;
                return PLAYER_CRASHED;
            }
            if (record->speed.x > record->consts.trot_speed) {
                speed = record->consts.trot_speed * 0.2F;
                /* @hack spelling: speed copied to scale, which keeps the
                 * hoisted 0.0F in FR12 (FLDI0 FR12 at 0x8c07a0fe) and speed
                 * in FR14 (0x8c07a256); scaled by speed, 14 of 551 words
                 * differ */
                scale = speed;
                njUnitVector(&motion->speed);
                motion->speed.x *= scale;
                motion->speed.y *= scale;
                motion->speed.z *= scale;
            }
            return 1;
        }
        njOuterProduct(&ball->hit.speed, &record->wall_normal, &axis);
        if (njScalor(&axis) == 0.0F) {
            axis = ball->normal;
        } else {
            njUnitVector(&axis);
        }
        first.px = contact->hit.point.x;
        first.py = contact->hit.point.y;
        first.pz = contact->hit.point.z;
        first.vx = record->wall_normal.x;
        first.vy = record->wall_normal.y;
        first.vz = record->wall_normal.z;
        second.px = ball->hit.pos.x;
        second.py = ball->hit.pos.y;
        second.pz = ball->hit.pos.z;
        second.vx = axis.x;
        second.vy = axis.y;
        second.vz = axis.z;
        /* @hack type: the union's own address as the NJS_LINE, which SHC forms
         * from R15 at each use; as &line.line, it keeps the address in a saved
         * register, the function runs 30 bytes long and 492 of 551 words
         * differ */
        njDistancePL2PL(&first, &second, (NJS_LINE *)&line);
        direction = ball->hit.speed;
        if (njScalor(&direction) != 0.0F) {
            njUnitVector(&direction);
        }
        if (njInnerProduct(&line.split.direction, &direction) < 0.0F) {
            line.line.vx = -line.line.vx;
            line.line.vy = -line.line.vy;
            line.line.vz = -line.line.vz;
        }
        speed = njScalor(&motion->speed);
        motion->speed.x = line.line.vx * speed;
        motion->speed.y = line.line.vy * speed;
        motion->speed.z = line.line.vz * speed;
        ball->hit.speed = motion->speed;
        work->ang.y =
            (int)(atan2f(line.line.vz, line.line.vx) * ANGLE_PER_RADIAN) &
            0xffff;
        if (angle_distance(motion->aim.y, work->ang.y) > 0x6000) {
            work->ang.y += 0x8000;
        }
        motion->aim.y = work->ang.y;
    }
    /* @bug uninitialized: this path returns no value, and player_collide_world
     * takes R0 as the result at 0x8c077e9c, which player_collide can return in
     * turn. R0 keeps what the turn test above leaves there (0x8c07a5d4 to
     * 0x8c07a5e4). angle_distance gives the distance from motion->aim.y to
     * work->ang.y. up to 0x6000, R0 keeps that distance. past 0x6000, R0 keeps
     * work->ang.y once the half turn, 0x8000, is added */
} /* NOLINT(clang-diagnostic-return-type) */

static int pinball_slide_along_walls(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record,
                                     struct polygon_ball *ball, Uint32 flags)
{
    NJS_PLANE first;
    NJS_PLANE second;
    union line_view line;
    NJS_VECTOR axis;
    NJS_VECTOR reverse_speed;
    Float distances[16];
    int contact_flags[16];
    NJS_VECTOR direction;
    int i;
    int ang;
    int count = 0;
    int nearest = -1;
    Float minimum = 1000.0F;
    Float speed;
    Float dot;
    struct polygon_contact *contact;

    for (i = 0; i < ball->count; ++i) {
        contact = &polygon_contacts[i];
        distances[i] = njDistanceP2P(&contact->hit.pos, &ball->pos);
        contact_flags[i] = 0;
        ang = vector_angle(&ball->normal, &contact->hit.normal, NULL);
        if (ang <= FACING_TURN || ball->normal.y < contact->hit.normal.y) {
            contact_flags[i] |= CONTACT_GROUND;
            /* @hack redundant: the continue in an if (1), as in
             * pinball_stop_at_walls (0x8c07a710, 0x8c07a742); as a bare
             * continue, 10 of 407 words differ */
            if (1) {
                continue;
            }
        }
        direction = ball->speed;
        if (njScalor(&direction) != 0.0F) {
            njUnitVector(&direction);
        }
        if (ang >= 0x7000) {
            continue;
        }
        if ((dot = njInnerProduct(&direction, &contact->hit.normal)) < 0.0F) {
            speed = njInnerProduct(&ball->speed, &contact->hit.normal);
            if (dot < -njCos(0x600) || speed < -4.5F) {
                contact_flags[i] |= CONTACT_WALL;
                ++count;
                if (distances[i] < minimum) {
                    minimum = distances[i];
                    nearest = i;
                }
            }
        }
    }
    if (count <= 0) {
        return 0;
    }
    speed = njScalor(&ball->speed);
    if (!(speed > distances[nearest]) ||
        (record->last_ground_flags & WORLD_NO_SLOPE)) {
        return 0;
    }
    if (njScalor(&ball->hit.speed) == 0.0F) {
        ball->hit.speed.x = ball->hit.speed.y = ball->hit.speed.z = 0.0F;
        return 0;
    } else {
        /* @hack redundant: the sums in an else after the return, which take FR2
         * and FR1 (0x8c07a846, 0x8c07a848); after the if, FR3 and FR2, and
         * 12 of 407 words differ */
        ball->hit.speed.x += ball->hit.normal_speed.x;
        ball->hit.speed.y += ball->hit.normal_speed.y;
        ball->hit.speed.z += ball->hit.normal_speed.z;
    }
    contact = &polygon_contacts[nearest];
    record->wall_normal = contact->hit.normal;
    reverse_speed.x = -motion->speed.x;
    reverse_speed.y = -motion->speed.y;
    reverse_speed.z = -motion->speed.z;
    if (njScalor(&reverse_speed) == 0.0F) {
        return 0;
    }
    njOuterProduct(&ball->hit.speed, &record->wall_normal, &axis);
    if (njScalor(&axis) == 0.0F) {
        axis = ball->normal;
    } else {
        njUnitVector(&axis);
    }
    first.px = contact->hit.point.x;
    first.py = contact->hit.point.y;
    first.pz = contact->hit.point.z;
    first.vx = contact->hit.normal.x;
    first.vy = contact->hit.normal.y;
    first.vz = contact->hit.normal.z;
    second.px = ball->hit.pos.x;
    second.py = ball->hit.pos.y;
    second.pz = ball->hit.pos.z;
    second.vx = axis.x;
    second.vy = axis.y;
    second.vz = axis.z;
    /* @hack type: the union's own address as the NJS_LINE, which SHC forms from
     * R15 at each use; as &line.line, it keeps the address in a saved register,
     * the function runs 36 bytes short and 132 of 407 words differ */
    njDistancePL2PL(&first, &second, (NJS_LINE *)&line);
    direction = ball->hit.speed;
    if (njScalor(&direction) != 0.0F) {
        njUnitVector(&direction);
    }
    if (njInnerProduct(&line.split.direction, &direction) < 0.0F) {
        line.line.vx = -line.line.vx;
        line.line.vy = -line.line.vy;
        line.line.vz = -line.line.vz;
    }
    speed = njScalor(&motion->speed);
    motion->speed.x = line.line.vx * speed;
    motion->speed.y = line.line.vy * speed;
    motion->speed.z = line.line.vz * speed;
    ball->hit.speed = motion->speed;
    /* a slide returns 12, and player_collide can return it in turn */
    return 12;
}

/* turn the ball's hit speed onto the line where the wall it hit meets a
 * plane through the ball. the line points away from vector, and the
 * player's heading turns with the speed. the plane holds vector and the
 * wall's normal, unless their cross product is 0.15 long or less: the
 * plane's normal is then the ball's up. nothing changes in the air at trot
 * speed or less, or where vector runs along the wall's normal */
/* @hack redundant: register on record and ball, which puts record in R13
 * and work in R12 (0x8c07aab0, 0x8c07aab2); without them, the two trade and
 * 20 of 280 words differ */
static void player_deflect(struct task_work *work,
                           register struct player *record,
                           register struct polygon_ball *ball,
                           NJS_VECTOR *vector)
{
    NJS_PLANE first;
    NJS_PLANE second;
    union line_view line;
    NJS_VECTOR axis;
    NJS_VECTOR direction;
    NJS_VECTOR next_direction;
    Float speed;

    if (!(record->last_flags & PLAYER_ON_GROUND) &&
        njScalor(&record->speed) <= record->consts.trot_speed) {
        return;
    }
    if (njOuterProduct(vector, &ball->hit.normal, &axis) == 0.0F) {
        return;
    }

    if (njScalor(&axis) <= 0.15F) {
        axis = ball->normal;
    } else {
        njUnitVector(&axis);
    }

    first.px = ball->hit.point.x;
    first.py = ball->hit.point.y;
    first.pz = ball->hit.point.z;
    first.vx = ball->hit.normal.x;
    first.vy = ball->hit.normal.y;
    first.vz = ball->hit.normal.z;
    second.px = ball->pos.x;
    second.py = ball->pos.y;
    second.pz = ball->pos.z;
    second.vx = axis.x;
    second.vy = axis.y;
    second.vz = axis.z;
    /* @hack type: the union's own address as the NJS_LINE, which SHC forms from
     * R15 at each use; as &line.line, it keeps the address in a saved register,
     * 145 of 280 words differ */
    njDistancePL2PL(&first, &second, (NJS_LINE *)&line);

    direction = *vector;
    if (njScalor(&direction) != 0.0F) {
        njUnitVector(&direction);
    }
    if (njInnerProduct(&line.split.direction, &direction) > 0.0F) {
        line.line.vx = -line.line.vx;
        line.line.vy = -line.line.vy;
        line.line.vz = -line.line.vz;
    }

    if (!(record->last_flags & PLAYER_ON_GROUND)) {
        speed = njScalor(&ball->hit.speed);
    } else {
        speed = njScalor(&ball->speed);
    }
    ball->hit.speed.x = speed * line.line.vx;
    ball->hit.speed.y = speed * line.line.vy;
    ball->hit.speed.z = speed * line.line.vz;

    if (record->last_ground_flags & 0x800000) {
        struct task_motion *motion = player_motion[PLAYER_VARS(work)->player];
        /* @hack spelling: speed's statement joined to the next by a comma,
         * which keeps njScalor's address in R11 and njUnitVector's in R10
         * (0x8c07aaea, 0x8c07aaee); as two statements, the two trade and 10
         * of 280 words differ */
        speed = njScalor(&motion->speed),
        motion->speed.x = speed * line.line.vx;
        motion->speed.y = speed * line.line.vy;
        motion->speed.z = speed * line.line.vz;
    }

    direction = ball->speed;
    next_direction = ball->hit.speed;
    njUnitVector(&direction);
    njUnitVector(&next_direction);
    work->ang.y -= vector_angle(&direction, &next_direction, &ball->normal);
    if (angle_distance(player_motion[PLAYER_VARS(work)->player]->aim.y,
                       work->ang.y) > 0x6000) {
        work->ang.y += 0x8000;
    }
    player_motion[PLAYER_VARS(work)->player]->aim.y = work->ang.y;
}

static void pinball_deflect(struct task_work *work, struct player *record,
                            struct polygon_ball *ball, NJS_VECTOR *vector)
{
    NJS_VECTOR direction;
    NJS_VECTOR first;
    NJS_VECTOR second;
    struct task_motion *motion;
    Float speed;

    if (!(record->last_flags & PLAYER_ON_GROUND) &&
        !(njScalor(&record->speed) > record->consts.trot_speed)) {
        return;
    }

    if (njOuterProduct(vector, &ball->hit.normal, &direction) == 0.0F) {
        return;
    }

    first = ball->speed;
    second = ball->hit.speed;
    if (njScalor(&second) == 0.0F) {
        return;
    }

    njUnitVector(&first);
    njUnitVector(&second);
    work->ang.y -= vector_angle(&first, &second, &ball->normal);
    if (angle_distance(player_motion[PLAYER_VARS(work)->player]->aim.y,
                       work->ang.y) > 0x6000) {
        work->ang.y += 0x8000;
    }
    player_motion[PLAYER_VARS(work)->player]->aim.y = work->ang.y;

    direction = ball->hit.speed;
    if (njScalor(&direction) < 0.4F) {
        return;
    }

    motion = player_motion[PLAYER_VARS(work)->player];
    speed = njScalor(&motion->speed);
    if (speed > 0.6F) {
        speed *= 0.96F;
    }
    njUnitVector(&direction);
    motion->speed.x = direction.x * speed;
    motion->speed.y = direction.y * speed;
    motion->speed.z = direction.z * speed;
}

/* move pos out of a wall by half of push */
/* @hack spelling: push and pos stepped across their vectors' members, a walk
 * ISO C leaves undefined, which keeps push in R4 and pos in R5 (0x8c07b136
 * to 0x8c07b13e); through the members, player_collide_climb runs 32 bytes
 * long and 858 of its 1104 words differ */
#pragma inline(push_out)
static void push_out(Float *push, Float *pos)
{
    *pos += *push * 0.5F;
    pos++;
    push = push + 1;
    *pos += *push * 0.5F;
    pos++;
    push++;
    *pos += *push * 0.5F;
}

int player_collide_climb(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    NJS_VECTOR up;
    struct polygon_hit hit;
    NJS_VECTOR normal;
    NJS_VECTOR direction;
    NJS_VECTOR side;
    NJS_VECTOR axis;
    NJS_VECTOR cross;
    struct polygon_ball *ball;
    struct world_object *world;
    struct world_object *hit_world;
    struct polygon_contact *contact;
    struct task_force *force;
    struct task *owner;
    int i;
    Sint16 j;
    Bool found;
    int result = 0;
    Angle ang_x;
    Angle ang_y;
    Angle ang_z;
    Float dot;

    if (record->ground_task != NULL) {
        force = record->ground_task->force;
        if (force != NULL) {
            work->pos.x += force->speed.x;
            work->pos.y += force->speed.y;
            work->pos.z += force->speed.z;
        }
    }
    if ((work->collision->flags & COLLISION_PUSHED) &&
        !(work->flags & PLAYER_HURT) &&
        (work->collision->flags & COLLISION_HIT_SIDE) &&
        (work->collision->hit->shapes[work->collision->hit_shape].push &
         PUSH_ALL)) {
        collision_get_hit_normal(work, &normal);
        direction = motion->speed;
        if (njScalor(&direction) != 0.0F) {
            njUnitVector(&direction);
        }
        dot = njInnerProduct(&normal, &direction);
        if (dot < 0.0F) {
            motion->speed.x = 0.0F;
            motion->speed.z = 0.0F;
        }
        if (record->speed.x >= record->consts.running_speed) {
            result = PLAYER_CRASHED;
        } else {
            result = 1;
        }
    }
    if (work->flags & PLAYER_ON_TASK) {
        work->flags &= ~PLAYER_ON_GROUND;
        work->pos.x += motion->speed.x;
        work->pos.z += motion->speed.z;
        return result;
    }
    work->flags &= ~PLAYER_ON_GROUND;
    ball = record->contact;
    up.x = up.z = 0.0F;
    up.y = 1.0F;
    player_turn_to_world(work, &up);
    ball->radius = record->consts.height * 0.75F;
    ball->hit.ang_x = work->ang.x;
    ball->hit.ang_z = work->ang.z;
    ball->speed = motion->speed;
    polygon_collect_near(ball);
    ball->pos.x = work->pos.x + motion->speed.x + (ball->radius * up.x);
    ball->pos.y = work->pos.y + motion->speed.y + (ball->radius * up.y);
    ball->pos.z = work->pos.z + motion->speed.z + (ball->radius * up.z);
    for (i = world_count, world = world_objects; i > 0; i--, world++) {
        if (polygon_sweep(ball, world->object, world->flags) == TRUE) {
            if (world->flags & 0x201000) {
                return result;
            }
            for (j = ball->count, contact = polygon_contacts; j > 0;
                 j--, contact++) {
                /* @hack redundant: dot, set and never read, which, with the
                 * same store in the last loop, keeps normal.x in FR5 and
                 * normal.z in FR4 for axis.y's sum (0x8c07b6c8, 0x8c07b6cc);
                 * without either, 6 of 1104 words differ */
                dot = contact->hit.normal.z;
                if (contact->hit.normal.y < -0.86F) {
                    if (!player_read_stick(work, NULL, NULL)) {
                        push_out(&contact->hit.normal.x, &work->pos.x);
                    }
                    return result;
                }
                if (contact->hit.normal.y < 0.86F &&
                    njInnerProduct(&up, &contact->hit.normal) <= 0.259F) {
                    if (!player_read_stick(work, NULL, NULL)) {
                        push_out(&contact->hit.normal.x, &work->pos.x);
                    }
                    return result;
                }
            }
        }
    }
    if (motion->speed.y < -0.01F) {
        ball->pos.x = work->pos.x + motion->speed.x +
                      (record->consts.height * up.x * 0.25F);
        ball->pos.y = work->pos.y + motion->speed.y +
                      (((up.y * 0.25F) - 0.5F) * record->consts.height);
        ball->pos.z = work->pos.z + motion->speed.z +
                      (record->consts.height * up.z * 0.25F);
        for (i = world_count, world = world_objects; i > 0; i--, world++) {
            if (polygon_sweep(ball, world->object, world->flags) == TRUE) {
                for (j = ball->count, contact = polygon_contacts; j > 0;
                     j--, contact++) {
                    if (contact->hit.normal.y >= 0.86F) {
                        ball->hit.normal.x = up.x;
                        ball->hit.normal.z = up.z;
                        ball->hit.normal.y = contact->hit.normal.y;
                        return result;
                    }
                }
            }
        }
    }
    found = FALSE;
    ball->pos.x = work->pos.x + motion->speed.x + (up.x * 0.3F);
    ball->pos.y = work->pos.y + motion->speed.y + (up.y * 0.3F);
    ball->pos.z = work->pos.z + motion->speed.z + (up.z * 0.3F);
    for (i = world_count, world = world_objects; i > 0; i--, world++) {
        if (polygon_sweep(ball, world->object, world->flags) != TRUE ||
            (world->flags & 0x201000)) {
            continue;
        }
        for (j = ball->count, contact = polygon_contacts; j > 0;
             j--, contact++) {
            if (contact->hit.normal.y >= -0.86F &&
                contact->hit.normal.y <= 0.71F) {
                if (njInnerProduct(&up, &contact->hit.normal) > 0.259F) {
                    if (!found) {
                        found = TRUE;
                        hit = contact->hit;
                        hit_world = world;
                    } else {
                        normal.x = contact->hit.point.x - hit.point.x;
                        normal.y = contact->hit.point.y - hit.point.y;
                        normal.z = contact->hit.point.z - hit.point.z;
                        if (njScalor(&normal) < 0.0001F) {
                            direction.x = up.x - hit.normal.x;
                            direction.y = up.y - hit.normal.y;
                            direction.z = up.z - hit.normal.z;
                            side.x = up.x - contact->hit.normal.x;
                            side.y = up.y - contact->hit.normal.y;
                            side.z = up.z - contact->hit.normal.z;
                            if (njScalor2(&direction) < njScalor2(&side)) {
                                hit = contact->hit;
                                hit_world = world;
                            }
                        } else if (njInnerProduct(&up, &normal) >= 0.0F) {
                            hit = contact->hit;
                            hit_world = world;
                        }
                    }
                }
            }
        }
    }
    if (found == TRUE) {
        normal.x = (up.x + hit.normal.x) * 0.5F;
        normal.y = (up.y + hit.normal.y) * 0.5F;
        normal.z = (up.z + hit.normal.z) * 0.5F;
        if (njScalor(&normal) == 0.0F) {
            normal.x = 1.0F;
            normal.y = normal.z = 0.0F;
        } else {
            njUnitVector(&normal);
        }
        ang_x = (int)(asinf(normal.z) * ANGLE_PER_RADIAN);
        ang_z = -(int)(atan2f(normal.x, normal.y) * ANGLE_PER_RADIAN);
        direction.x = 1.0F;
        direction.y = direction.z = 0.0F;
        matrix_push_unit();
        TURN(njRotateZ, ang_z);
        TURN(njRotateX, ang_x);
        njCalcVector(NULL, &direction, &side);
        njPopMatrix(1);
        if (normal.y * normal.y < 0.0001F) {
            axis.x = axis.z = 0.0F;
            axis.y = 1.0F;
        } else {
            if (normal.y > 0.0F) {
                axis.x = -normal.x;
                axis.y =
                    ((normal.x * normal.x) + (normal.z * normal.z)) / normal.y;
                axis.z = -normal.z;
            } else {
                axis.x = normal.x;
                axis.y =
                    ((-normal.x * normal.x) - (normal.z * normal.z)) / normal.y;
                axis.z = normal.z;
            }
            njUnitVector(&axis);
        }
        if (_builtin_fabsf(njOuterProduct(&axis, &side, &cross)) < 0.01F) {
            if (njInnerProduct(&side, &axis) > 0.0F) {
                ang_y = 0;
            } else {
                ang_y = 0x8000;
            }
        } else {
            njUnitVector(&cross);
            if (njInnerProduct(&normal, &cross) >= 0.0F) {
                ang_y = (int)(acosf(njInnerProduct(&side, &axis)) *
                              ANGLE_PER_RADIAN);
            } else {
                ang_y = -(int)(acosf(njInnerProduct(&side, &axis)) *
                               ANGLE_PER_RADIAN);
            }
        }
        /* @hack spelling: ang_x, ang_y and ang_z cut to 16 bits ahead of the
         * stores, which extends them into R5, R4 and R6 (EXTU.W at
         * 0x8c07b7fc to 0x8c07b800); cut in the stores, 19 of 1104 words
         * differ */
        ang_x = (Uint16)ang_x;
        ang_y = (Uint16)ang_y;
        ang_z = (Uint16)ang_z;
        work->ang.x = ang_x;
        work->ang.y = ang_y;
        work->ang.z = ang_z;
        motion->aim.x = work->ang.x;
        motion->aim.z = work->ang.z;
        ball->pos = hit.point;
        owner = hit_world->owner;
        if (owner != NULL) {
            njAddVector(&ball->pos,
                        &((struct task_force *)owner->force)->speed);
            work->ang.x += ((struct task_force *)owner->force)->turn.x;
            work->ang.y += ((struct task_force *)owner->force)->turn.y;
            work->ang.z += ((struct task_force *)owner->force)->turn.z;
        }
        record->ground_task = hit_world->owner;
        if (njDistanceP2P(&work->pos, &ball->pos) > 0.3F) {
            work->pos = ball->pos;
        } else {
            record->speed.x = record->speed.y = record->speed.z = 0.0F;
            motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        }
        ball->speed.x = ball->speed.y = ball->speed.z = 0.0F;
        work->flags |= PLAYER_ON_LAND;
        record->ground_object = hit_world->object;
        return result;
    }
    if (motion->speed.y > 0.01F) {
        ball->pos.x = work->pos.x + motion->speed.x -
                      (record->consts.height * up.x * 0.25F);
        ball->pos.y = work->pos.y + motion->speed.y +
                      (((up.y * 0.25F) + 0.5F) * record->consts.height);
        ball->pos.z = work->pos.z + motion->speed.z -
                      (record->consts.height * up.z * 0.25F);
        for (i = world_count, world = world_objects; i > 0; i--, world++) {
            if (polygon_sweep(ball, world->object, world->flags) == TRUE) {
                for (j = ball->count, contact = polygon_contacts; j > 0;
                     j--, contact++) {
                    /* @hack redundant: dot, set and never read, as in the
                     * first loop; without it, 6 of 1104 words differ */
                    dot = contact->hit.normal.z;
                    if (contact->hit.normal.y >= 0.86F) {
                        ball->hit.normal.x = up.x;
                        ball->hit.normal.z = up.z;
                        ball->hit.normal.y = contact->hit.normal.y;
                        return result;
                    }
                }
            }
        }
    }
    return result;
}

int player_collide_simple(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    NJS_VECTOR offset;
    NJS_VECTOR normal;
    NJS_VECTOR position;
    NJS_VECTOR direction;
    struct polygon_ball *ball;
    struct world_object *world;
    Sint16 old_flags;
    Uint16 i;
    struct task *owner;
    Uint8 grounded;
    int result = 0;
    int ang;
    int pitch;
    int roll;
    int other_pitch;
    int other_roll;
    int pitch_distance;
    int roll_distance;
    Float dot;

    if ((work->collision->flags & COLLISION_PUSHED) &&
        !(work->flags & PLAYER_HURT) &&
        (work->collision->flags & COLLISION_HIT_SIDE) &&
        (work->collision->hit->shapes[work->collision->hit_shape].push &
         PUSH_ALL)) {
        collision_get_hit_normal(work, &normal);
        direction = motion->speed;
        if (njScalor(&direction) != 0.0F) {
            njUnitVector(&direction);
        }
        dot = njInnerProduct(&normal, &direction);
        if (dot < 0.0F) {
            motion->speed.x = 0.0F;
            motion->speed.z = 0.0F;
        }
        if (record->speed.x >= record->consts.running_speed) {
            result = PLAYER_CRASHED;
        } else {
            result = 1;
        }
    }
    if (work->flags & PLAYER_ON_TASK) {
        work->flags &= ~PLAYER_ON_GROUND;
        work->pos.x += motion->speed.x;
        work->pos.z += motion->speed.z;
    } else {
        old_flags = work->flags;
        work->flags &= ~PLAYER_ON_GROUND;
        grounded = FALSE;
        world = world_objects;
        ball = record->contact;
        matrix_push_unit();
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, -work->ang.y);
        position.x = position.z = 0.0F;
        position.y = 1.0F;
        ball->hit.ang_x = work->ang.x;
        ball->hit.ang_z = work->ang.z;
        njCalcVector(NULL, &position, &ball->normal);
        position.x = position.z = 0.0F;
        position.y = record->consts.height * 0.5F;
        njCalcVector(NULL, &position, &offset);
        position.x = position.z = 0.0F;
        position.y = (record->consts.height * 0.5F) - record->consts.snap;
        njCalcVector(NULL, &position, &normal);
        njPopMatrix(1);
        ball->radius = record->consts.height * 0.5F;
        ball->pos.x = work->pos.x + normal.x;
        ball->pos.y = work->pos.y + normal.y;
        ball->pos.z = work->pos.z + normal.z;
        ball->speed = motion->speed;
        polygon_collect_near(ball);
        for (i = 0; i < world_count; i++) {
            if (polygon_sweep(ball, world[i].object, world[i].flags)) {
                ang = vector_angle(&ball->normal, &ball->hit.normal, NULL);
                if (ang <= FACING_TURN || ball->normal.y < ball->hit.normal.y) {
                    if (record->speed.y <= 0.0F ||
                        (record->speed.y > 0.0F &&
                         !(old_flags & PLAYER_ON_GROUND))) {
                        grounded = TRUE;
                        if (motion->speed.x != 0.0F ||
                            motion->speed.z != 0.0F) {
                            /* @hack redundant: owner, set here and again
                             * before it's read, which gives the frame 88
                             * bytes (ADD #-88,R15 at 0x8c07ba16); without
                             * it, the function runs 32 bytes short and 410
                             * of 635 words differ */
                            owner = world[i].owner;
                            pitch = (int)(asinf(ball->hit.normal.z) *
                                          ANGLE_PER_RADIAN);
                            roll = -(int)(asinf(ball->hit.normal.x) *
                                          ANGLE_PER_RADIAN);
                            if (ball->hit.normal.y < 0.0F) {
                                other_pitch = (Uint16)(0x8000 - pitch);
                                other_roll = (Uint16)(0x8000 - roll);
                                pitch_distance =
                                    angle_distance(work->ang.x, other_pitch);
                                roll_distance =
                                    angle_distance(work->ang.z, other_roll);
                                if (pitch_distance < roll_distance) {
                                    ball->hit.ang_x = other_pitch;
                                    ball->hit.ang_z = roll;
                                } else {
                                    ball->hit.ang_x = pitch;
                                    ball->hit.ang_z = other_roll;
                                }
                            }
                            work->ang.x = angle_approach(
                                work->ang.x, ball->hit.ang_x, 0x1000);
                            work->ang.z = angle_approach(
                                work->ang.z, ball->hit.ang_z, 0x1000);
                        }
                        motion->aim.x = ball->hit.ang_x;
                        motion->aim.z = ball->hit.ang_z;
                        ball->pos.x = ball->hit.pos.x + ball->hit.speed.x;
                        ball->pos.y = ball->hit.pos.y + ball->hit.speed.y;
                        ball->pos.z = ball->hit.pos.z + ball->hit.speed.z;
                        owner = world[i].owner;
                        if (owner != NULL) {
                            record->ground_task = owner;
                            player_ride_task(work, record, ball, &offset);
                        }
                        position.x = ball->pos.x - offset.x;
                        position.y = ball->pos.y - offset.y;
                        position.z = ball->pos.z - offset.z;
                        direction.x = work->pos.x - position.x;
                        direction.y = work->pos.y - position.y;
                        direction.z = work->pos.z - position.z;
                        if (njScalor2(&direction) > 0.0001F) {
                            work->pos = position;
                        }
                        ball->speed.x = ball->speed.y = ball->speed.z = 0.0F;
                        work->flags |= PLAYER_ON_LAND;
                    }
                } else {
                    direction.x = -motion->speed.x;
                    direction.y = -motion->speed.y;
                    direction.z = -motion->speed.z;
                    if (njScalor(&direction) != 0.0F) {
                        njUnitVector(&direction);
                        ang = vector_angle(&direction, &ball->hit.normal, NULL);
                        if (ang <= FACING_TURN) {
                            if (record->speed.x >= record->consts.crash_speed) {
                                result = PLAYER_CRASHED;
                            } else {
                                result = 1;
                            }
                        }
                    }
                    ball->pos = ball->hit.pos;
                    work->pos.x = ball->hit.pos.x - offset.x;
                    work->pos.y = ball->hit.pos.y - offset.y;
                    work->pos.z = ball->hit.pos.z - offset.z;
                    ball->speed = ball->hit.speed;
                    motion->speed = ball->hit.speed;
                }
            }
        }
        if (grounded != TRUE) {
            work->pos.x += motion->speed.x;
            work->pos.y += motion->speed.y;
            work->pos.z += motion->speed.z;
        }
    }
    return result;
}

static Uint32 player_collide_pinball(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record)
{
    NJS_VECTOR offset;
    NJS_VECTOR normal;
    NJS_VECTOR position;
    NJS_POINT3 old_pos;
    struct world_object candidates[16];
    struct world_object sorted[16];
    NJS_PLANE plane;
    NJS_POINT3 point;
    struct world_object swap;
    struct polygon_ball *ball;
    struct world_object *world;
    struct world_object *entry;
    struct world_object *last;
    struct world_object *list;
    struct polygon_contact *contact;
    struct task *owner;
    NJS_OBJECT *object;
    int old_flags;
    int i;
    int j;
    int k;
    int taken;
    int count;
    int saved;
    Bool hit;
    Bool grounded;
    Uint32 result;
    int found;
    int ang;
    Float stroke;
    Float length;

    ball = record->contact;
    result = 0;
    old_flags = record->last_flags;
    record->last_flags = work->flags;
    work->flags &= ~PLAYER_ON_LAND;
    matrix_push_unit();
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    position.x = position.z = 0.0F;
    position.y = 1.0F;
    ball->hit.ang_x = work->ang.x;
    ball->hit.ang_z = work->ang.z;
    njCalcVector(NULL, &position, &ball->normal);
    position.x = position.z = 0.0F;
    position.y = record->consts.height * 0.5F;
    njCalcVector(NULL, &position, &offset);
    ball_drop(motion, record, ball, &position);
    njCalcVector(NULL, &position, &normal);
    njPopMatrix(1);
    ball->pos.x = work->pos.x + normal.x;
    ball->pos.y = work->pos.y + normal.y;
    ball->pos.z = work->pos.z + normal.z;
    ball->radius = record->consts.height * 0.5F;
    ball->speed = motion->speed;
    old_pos = work->pos;
    polygon_collect_near(ball);
    if (world_count == 0) {
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        player_set_ground(work, motion, record, 0);
        record->ground_task = NULL;
        record->ground_object = NULL;
        return 0;
    }
    hit = FALSE;
    grounded = FALSE;
    world = world_objects;
    object = NULL;
    saved = 0;
    count = 0;
    for (i = 0; i < world_count; i++, world++) {
        if ((world->flags & WORLD_LIQUID) &&
            PLAYER_VARS(work)->character != CHARACTER_E102) {
            continue;
        }
        if (world->owner != NULL &&
            !(world->owner->work->flags & WORLD_TESTED)) {
            continue;
        }
        if (polygon_sweep(ball, world->object, world->flags)) {
            if (record->last_flags & PLAYER_ON_GROUND) {
                if (pinball_stop_at_walls(work, motion, record, ball,
                                          world->flags) == TRUE) {
                    hit = TRUE;
                    saved = count;
                }
            } else if (ball_stop_at_ground(work, motion, record, ball) ==
                       TRUE) {
                hit = TRUE;
                saved = count;
            }
        }
        candidates[count] = *world;
        count++;
    }
    if (count == 0) {
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        player_set_ground(work, motion, record, 0);
        record->ground_task = NULL;
        record->ground_object = NULL;
        return 0;
    }
    taken = 0;
    if (hit == TRUE) {
        sorted[taken] = candidates[saved];
        taken++;
    }
    last = &sorted[count - 1];
    k = count - 1;
    if (record->ground_task != NULL) {
        for (j = 0; j < count; j++) {
            if (hit == TRUE && j == saved) {
                hit = FALSE;
            } else {
                entry = &candidates[j];
                if (record->ground_object == entry->object) {
                    *last = *entry;
                } else {
                    sorted[taken] = *entry;
                    taken++;
                }
            }
        }
    } else {
        for (j = count - 1; j >= 0; j--) {
            if (hit == TRUE && j == saved) {
                hit = FALSE;
            } else {
                entry = &candidates[j];
                if (record->ground_object == entry->object) {
                    *last = *entry;
                } else {
                    sorted[taken] = *entry;
                    taken++;
                }
            }
        }
    }
    ball->pos.x = work->pos.x + normal.x;
    ball->pos.y = work->pos.y + normal.y;
    ball->pos.z = work->pos.z + normal.z;
    ball->speed = motion->speed;
    record->ground_task = NULL;
    list = sorted;
    for (i = 0; i < count; i++) {
        entry = &list[i];
        owner = entry->owner;
        if (!polygon_sweep(ball, entry->object, entry->flags)) {
            continue;
        }
        ang = vector_angle(&ball->normal, &ball->hit.normal, NULL);
        if (!(work->flags & PLAYER_ON_TASK) && !(entry->flags & 0x200040) &&
            (ang <= FACING_TURN || ball->normal.y < ball->hit.normal.y) &&
            (!(old_flags & PLAYER_ON_GROUND) ||
             (record->last_flags & PLAYER_ON_GROUND)) &&
            (njInnerProduct(&world_gravity, &ball->hit.normal) <= 0.0F ||
             (record->last_flags & PLAYER_ON_GROUND))) {
            if (record->speed.y <= 0.0F ||
                (record->speed.y > 0.0F &&
                 !(record->last_flags & PLAYER_ON_GROUND))) {
                grounded = TRUE;
                object = entry->object;
                found = pinball_slide_along_walls(work, motion, record, ball,
                                                  entry->flags);
                if (found) {
                    if (!result) {
                        result = found;
                    } else if (found == PLAYER_CRASHED) {
                        result = found;
                    }
                }
                ball->hit.ang_z =
                    -(int)(atan2f(ball->hit.normal.x, ball->hit.normal.y) *
                           ANGLE_PER_RADIAN);
                ball->hit.ang_x =
                    (int)(asinf(ball->hit.normal.z) * ANGLE_PER_RADIAN);
                if (motion->speed.x != 0.0F || motion->speed.z != 0.0F ||
                    !(record->last_flags & PLAYER_ON_LAND) ||
                    njInnerProduct(&world_gravity, &ball->hit.normal) <
                        njInnerProduct(&world_gravity, &ball->normal)) {
                    player_tilt_to_ground(work, record, ball);
                }
                if (!result) {
                    matrix_push_unit();
                    TURN(njRotateZ, work->ang.z);
                    TURN(njRotateX, work->ang.x);
                    TURN(njRotateY, -work->ang.y);
                    position.x = position.z = 0.0F;
                    position.y = 1.0F;
                    ball->hit.ang_x = work->ang.x;
                    ball->hit.ang_z = work->ang.z;
                    njCalcVector(NULL, &position, &ball->normal);
                    position.x = position.z = 0.0F;
                    position.y = record->consts.height * 0.5F;
                    njCalcVector(NULL, &position, &offset);
                    ball_drop(motion, record, ball, &position);
                    njCalcVector(NULL, &position, &normal);
                    njPopMatrix(1);
                }
                motion->aim.x = ball->hit.ang_x;
                motion->aim.z = ball->hit.ang_z;
                record->ground_normal = ball->hit.normal;
                if (njScalor(&ball->hit.speed) > 4.1F) {
                    njUnitVector(&ball->hit.speed);
                    length = njScalor(&ball->speed) - 4.1F;
                    ball->speed.x = ball->hit.speed.x * length;
                    ball->speed.y = ball->hit.speed.y * length;
                    ball->speed.z = ball->hit.speed.z * length;
                    /* @hack spelling: 4.1F copied to length, which keeps
                     * it in FR14 (0x8c07c0c6); by 4.1F directly, FR13, and
                     * 50 of 1203 words differ */
                    length = 4.1F;
                    ball->hit.speed.x *= length;
                    ball->hit.speed.y *= length;
                    ball->hit.speed.z *= length;
                    swap = list[k];
                    list[k] = *entry;
                    *entry = swap;
                    i = -1;
                } else {
                    ball->speed.x = ball->speed.y = ball->speed.z = 0.0F;
                }
                ball->pos.x = ball->hit.pos.x + ball->hit.speed.x;
                ball->pos.y = ball->hit.pos.y + ball->hit.speed.y;
                ball->pos.z = ball->hit.pos.z + ball->hit.speed.z;
                contact = &polygon_contacts[ball->hit_index];
                plane.vx = ball->normal.x;
                plane.vy = ball->normal.y;
                plane.vz = ball->normal.z;
                plane.px = contact->hit.point.x;
                plane.py = contact->hit.point.y;
                plane.pz = contact->hit.point.z;
                njDistanceP2PL(&ball->pos, &plane, &point);
                ball->pos.x = offset.x + point.x;
                ball->pos.y = offset.y + point.y;
                ball->pos.z = offset.z + point.z;
                if (owner != NULL) {
                    record->ground_task = owner;
                    player_ride_task(work, record, ball, &offset);
                } else {
                    record->ground_task = NULL;
                }
                position.x = ball->pos.x - offset.x;
                position.y = ball->pos.y - offset.y;
                position.z = ball->pos.z - offset.z;
                length = njDistanceP2P(&work->pos, &position);
                if (owner != NULL || record->speed.x != 0.0F ||
                    record->speed.z != 0.0F || length > 1.0F) {
                    if (length > 0.0001F) {
                        work->pos = position;
                    }
                }
                work->flags |= PLAYER_ON_LAND;
                player_set_ground(work, motion, record, entry->flags);
            } else {
                result = pinball_hit_wall(work, motion, record, ball, &old_pos,
                                          &offset, result, entry->flags);
            }
            continue;
        } else {
            result = pinball_hit_wall(work, motion, record, ball, &old_pos,
                                      &offset, result, entry->flags);
        }
    }
    /* @hack redundant: ang, set from grounded and never read, which keeps
     * ANGLE_PER_RADIAN in FR13 (FMUL FR13,FR0 at 0x8c07c674); without it,
     * FR12, and 4 of 1203 words differ */
    ang = grounded ? ball->hit.ang_x : ball->hit.ang_z;
    if (grounded != TRUE) {
        stroke = 0.0F;
        if ((record->last_flags & PLAYER_ON_LAND) &&
            !(record->last_flags & PLAYER_EVENT_DRIVEN) &&
            (!player_read_stick(work, NULL, &stroke) ||
             (stroke <= 0.9F && !record->launch_steps)) &&
            record->speed.y <= 0.0F &&
            record->speed.x <= record->consts.running_speed &&
            record->level > 0.7F) {
            work->pos.x -= motion->speed.x * 0.5F;
            work->pos.y -= motion->speed.y * 0.5F;
            work->pos.z -= motion->speed.z * 0.5F;
            motion->speed.x = 0.0F;
            motion->speed.y = 0.0F;
            motion->speed.z = 0.0F;
        } else {
            work->pos.x += ball->speed.x;
            work->pos.y += ball->speed.y;
            work->pos.z += ball->speed.z;
        }
        player_set_ground(work, motion, record, 0);
    } else {
        record->ground_object = object;
        work->pos.x += ball->speed.x;
        work->pos.y += ball->speed.y;
        work->pos.z += ball->speed.z;
    }
    return result;
}

void player_move_on_path(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    struct path_sample sample;
    NJS_VECTOR normal;
    NJS_VECTOR up;
    Angle heading;

    work->flags &= ~(PLAYER_ON_PATH | PLAYER_ON_GROUND);
    sample.distance = record->path_distance;
    if (path_sample(PLAYER_VARS(work)->arg.path, &sample)) {
        if (angle_distance(motion->aim.y,
                           (Angle)(atan2f(sample.ahead.z, sample.ahead.x) *
                                   ANGLE_PER_RADIAN)) > 0x4000) {
            record->path_distance -= record->speed.x;
        } else {
            record->path_distance += record->speed.x;
        }
    } else {
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        return;
    }
    sample.distance = record->path_distance;
    if (path_sample(PLAYER_VARS(work)->arg.path, &sample)) {
        heading =
            (Angle)(atan2f(sample.ahead.z, sample.ahead.x) * ANGLE_PER_RADIAN);
        up.x = up.z = 0.0F;
        up.y = 1.0F;
        matrix_push_unit();
        TURN(njRotateZ, sample.smooth_z);
        TURN(njRotateX, sample.smooth_x);
        njCalcVector(NULL, &up, &normal);
        njPopMatrix(1);
        if (normal.y < 0.0F) {
            heading = (Uint16)(0x8000 - heading);
        }
        if (angle_distance(motion->aim.y, heading) > 0x4000) {
            heading += 0x8000;
        }
        motion->aim.y = heading;
        work->ang.y = angle_approach(work->ang.y, heading, 0x800);
        work->ang.x = sample.smooth_x;
        work->ang.z = sample.smooth_z;
        work->pos.x = sample.pos.x;
        work->pos.y = sample.pos.y;
        work->pos.z = sample.pos.z;
        work->flags |= PLAYER_ON_PATH | PLAYER_ON_LAND;
    } else {
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
    }
}

Bool player_slide_on_path(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    struct path_sample sample;
    NJS_VECTOR normal;
    NJS_VECTOR up;
    Angle heading;
    struct path *path = PLAYER_VARS(work)->arg.path;

    motion->drive = 0.0F;
    sample.distance = record->path_distance;
    if (path_sample(path, &sample)) {
        if (angle_distance(motion->aim.y,
                           (Angle)(atan2f(sample.ahead.z, sample.ahead.x) *
                                   ANGLE_PER_RADIAN)) > 0x4000) {
            record->path_distance -= record->speed.x;
        } else {
            record->path_distance += record->speed.x;
        }
        motion->drive = 1.0F;
    } else {
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        motion->drive = 0.0F;
        return FALSE;
    }
    if (path->count == 1) {
        record->path_distance = 0.0F;
        motion->drive = 0.0F;
    } else if (path->length <= record->path_distance) {
        record->path_distance = path->length - 0.01F;
        motion->drive = 0.0F;
    } else if (record->path_distance <= 0.0F) {
        record->path_distance = 0.01F;
        motion->drive = 0.0F;
    }
    sample.distance = record->path_distance;
    if (path_sample(path, &sample)) {
        heading =
            (Angle)(atan2f(sample.ahead.z, sample.ahead.x) * ANGLE_PER_RADIAN);
        up.x = up.z = 0.0F;
        up.y = 1.0F;
        matrix_push_unit();
        TURN(njRotateZ, sample.smooth_z);
        TURN(njRotateX, sample.smooth_x);
        njCalcVector(NULL, &up, &normal);
        njPopMatrix(1);
        if (normal.y < 0.0F) {
            heading = (Uint16)(0x8000 - heading);
        }
        if (angle_distance(motion->aim.y, heading) > 0x4000) {
            heading += 0x8000;
        }
        motion->aim.y = heading;
        work->ang.y = angle_approach(work->ang.y, heading, 0x800);
        work->ang.x = sample.smooth_x;
        work->ang.z = sample.smooth_z;
        work->pos.x = sample.pos.x;
        work->pos.y = sample.pos.y;
        work->pos.z = sample.pos.z;
        return TRUE;
    }
    return FALSE;
}

Bool player_shift_on_path(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    struct path_sample sample;
    struct path *path = PLAYER_VARS(work)->arg.path;

    motion->drive = 0.0F;
    sample.distance = record->path_distance;
    if (path_sample(path, &sample)) {
        record->path_distance += record->speed.x;
        motion->drive = 1.0F;
    } else {
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        motion->drive = 0.0F;
        return FALSE;
    }
    if (path->count == 1) {
        record->path_distance = 0.0F;
        motion->drive = 0.0F;
    } else if (path->length <= record->path_distance) {
        record->path_distance = path->length - 0.01F;
        motion->drive = 0.0F;
    } else if (record->path_distance <= 0.0F) {
        record->path_distance = 0.01F;
        motion->drive = 0.0F;
    }
    sample.distance = record->path_distance;
    if (path_sample(path, &sample)) {
        work->pos.x = sample.pos.x;
        work->pos.y = sample.pos.y;
        work->pos.z = sample.pos.z;
        return TRUE;
    }
    return FALSE;
}

int player_collide_wall(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    NJS_VECTOR offset;
    NJS_VECTOR normal;
    NJS_VECTOR position;
    struct polygon_ball *ball;
    struct world_object *world;
    Sint16 old_flags;
    Uint16 i;
    struct task *owner;
    Uint8 grounded;
    int result = 0;
    int ang;
    int pitch;
    int roll;
    int other_pitch;
    int other_roll;
    int pitch_distance;
    int roll_distance;
    Float dot;
    Float off;

    if ((work->collision->flags & COLLISION_PUSHED) &&
        !(work->flags & PLAYER_HURT) &&
        (work->collision->flags & COLLISION_HIT_SIDE) &&
        (work->collision->hit->shapes[work->collision->hit_shape].push &
         PUSH_ALL)) {
        NJS_VECTOR direction;

        collision_get_hit_normal(work, &normal);
        direction = motion->speed;
        if (njScalor(&direction) != 0.0F) {
            njUnitVector(&direction);
        }
        dot = njInnerProduct(&normal, &direction);
        if (dot < 0.0F) {
            motion->speed.x = 0.0F;
            motion->speed.z = 0.0F;
        }
        if (record->speed.x >= record->consts.running_speed) {
            result = PLAYER_CRASHED;
        } else {
            result = 1;
        }
    }
    if (work->flags & PLAYER_ON_TASK) {
        work->flags &= ~PLAYER_ON_GROUND;
        work->pos.x += motion->speed.x;
        work->pos.z += motion->speed.z;
        return result;
    }
    old_flags = work->flags;
    work->flags &= ~PLAYER_ON_GROUND;
    grounded = FALSE;
    world = world_objects;
    ball = record->contact;
    matrix_push_unit();
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    position.x = position.z = 0.0F;
    position.y = 1.0F;
    ball->hit.ang_x = work->ang.x;
    ball->hit.ang_z = work->ang.z;
    njCalcVector(NULL, &position, &ball->normal);
    position.x = position.z = 0.0F;
    position.y = record->consts.height * 0.5F;
    njCalcVector(NULL, &position, &offset);
    position.x = position.z = 0.0F;
    position.y = (record->consts.height * 0.5F) - (record->consts.snap * 0.5F);
    njCalcVector(NULL, &position, &normal);
    njPopMatrix(1);
    ball->radius = record->consts.height * 0.5F;
    ball->pos.x = work->pos.x + normal.x;
    ball->pos.y = work->pos.y + normal.y;
    ball->pos.z = work->pos.z + normal.z;
    ball->speed = motion->speed;
    polygon_collect_near(ball);
    for (i = 0; i < world_count; i++) {
        if (polygon_sweep(ball, world[i].object, world[i].flags)) {
            position.x = 1.0F;
            position.y = position.z = 0.0F;
            ang = vector_angle(&position, &ball->hit.normal, NULL);
            if (ang <= FACING_TURN) {
                if (record->speed.y <= 0.0F ||
                    (record->speed.y > 0.0F &&
                     !(old_flags & PLAYER_ON_GROUND))) {
                    NJS_VECTOR delta;

                    grounded = TRUE;
                    if (motion->speed.x != 0.0F || motion->speed.z != 0.0F) {
                        /* @hack redundant: owner, set here and again
                         * before it's read, which gives the frame 92 bytes
                         * (ADD #-92,R15 at 0x8c07d218); without it, the
                         * function runs 6 bytes short and 178 of 642 words
                         * differ */
                        owner = world[i].owner;
                        pitch = (Angle)(asinf(ball->hit.normal.z) *
                                        ANGLE_PER_RADIAN);
                        roll = -(Angle)(asinf(ball->hit.normal.x) *
                                        ANGLE_PER_RADIAN);
                        if (ball->hit.normal.y < 0.0F) {
                            other_pitch = (Uint16)(0x8000 - pitch);
                            other_roll = (Uint16)(0x8000 - roll);
                            pitch_distance =
                                angle_distance(work->ang.x, other_pitch);
                            roll_distance =
                                angle_distance(work->ang.z, other_roll);
                            if (pitch_distance < roll_distance) {
                                ball->hit.ang_x = other_pitch;
                                ball->hit.ang_z = roll;
                            } else {
                                ball->hit.ang_x = pitch;
                                ball->hit.ang_z = other_roll;
                            }
                        }
                        work->ang.x = angle_approach(work->ang.x,
                                                     ball->hit.ang_x, 0x1000);
                        work->ang.z = angle_approach(work->ang.z,
                                                     ball->hit.ang_z, 0x1000);
                    }
                    motion->aim.x = ball->hit.ang_x;
                    motion->aim.z = ball->hit.ang_z;
                    ball->pos.x = ball->hit.pos.x + ball->hit.speed.x;
                    ball->pos.y = ball->hit.pos.y + ball->hit.speed.y;
                    ball->pos.z = ball->hit.pos.z + ball->hit.speed.z;
                    owner = world[i].owner;
                    if (owner != NULL) {
                        record->ground_task = owner;
                        player_ride_task(work, record, ball, &offset);
                    }
                    position.x = ball->pos.x - offset.x;
                    position.y = ball->pos.y - offset.y;
                    position.z = ball->pos.z - offset.z;
                    delta.x = work->pos.x - position.x;
                    delta.y = work->pos.y - position.y;
                    delta.z = work->pos.z - position.z;
                    off = (delta.x * delta.x) + (delta.y * delta.y) +
                          (delta.z * delta.z);
                    if (off > 0.0001F) {
                        work->pos = position;
                    }
                    ball->speed.x = ball->speed.y = ball->speed.z = 0.0F;
                    work->flags |= PLAYER_ON_LAND;
                }
            } else {
                NJS_VECTOR direction;

                direction.x = -motion->speed.x;
                direction.y = -motion->speed.y;
                direction.z = -motion->speed.z;
                if (njScalor(&direction) != 0.0F) {
                    njUnitVector(&direction);
                    ang = vector_angle(&direction, &ball->hit.normal, NULL);
                    if (ang <= FACING_TURN) {
                        if (record->speed.x >= record->consts.crash_speed) {
                            result = PLAYER_CRASHED;
                        } else {
                            result = 1;
                        }
                    }
                }
                ball->pos = ball->hit.pos;
                work->pos.x = ball->hit.pos.x - offset.x;
                work->pos.y = ball->hit.pos.y - offset.y;
                work->pos.z = ball->hit.pos.z - offset.z;
                ball->speed = ball->hit.speed;
            }
        }
    }
    if (grounded != TRUE) {
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
    }
    return result;
}

void player_turn_back(struct task_work *work, NJS_VECTOR *vector)
{
    NJS_VECTOR local;
    const char *from;

    /* @hack spelling: the vector copied through a byte cursor, which the image
     * steps with FMOV.S @R4+; through the members, the function runs 6 bytes
     * long and 55 of 56 words differ */
    from = (const char *)vector;
    local.x = *(const Float *)from;
    from += sizeof(Float);
    local.y = *(const Float *)from;
    from += sizeof(Float);
    local.z = *(const Float *)from;
    matrix_push_unit();
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, -work->ang.x);
    TURN(njRotateZ, -work->ang.z);
    njCalcVector(NULL, &local, vector);
    njPopMatrix(1);
}

void player_turn_to_world(struct task_work *work, NJS_VECTOR *vector)
{
    NJS_VECTOR local;
    const char *from;

    /* @hack spelling: the vector copied through a byte cursor, as in
     * player_turn_back; through the members, 54 of 55 words differ */
    from = (const char *)vector;
    local.x = *(const Float *)from;
    from += sizeof(Float);
    local.y = *(const Float *)from;
    from += sizeof(Float);
    local.z = *(const Float *)from;
    matrix_push_unit();
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    njCalcVector(NULL, &local, vector);
    njPopMatrix(1);
}

static void ground_turn_back(NJS_VECTOR *normal, NJS_VECTOR *vector,
                             Angle ang_y)
{
    Angle ang_z;
    Angle ang_x;
    NJS_VECTOR local;
    const char *from;

    /* @hack spelling: the vector copied through a byte cursor, as in
     * player_turn_back; through the members, the function runs 6 bytes long and
     * 76 of 78 words differ */
    from = (const char *)vector;
    local.x = *(const Float *)from;
    from += sizeof(Float);
    local.y = *(const Float *)from;
    from += sizeof(Float);
    local.z = *(const Float *)from;
    ang_z = -(Angle)(atan2f(normal->x, normal->y) * ANGLE_PER_RADIAN);
    ang_x = (Angle)(asinf(normal->z) * ANGLE_PER_RADIAN);
    matrix_push_unit();
    TURN(njRotateY, ang_y);
    TURN(njRotateX, -ang_x);
    TURN(njRotateZ, -ang_z);
    njCalcVector(NULL, &local, vector);
    njPopMatrix(1);
}

static void ground_turn_to_world(NJS_VECTOR *normal, NJS_VECTOR *vector,
                                 Angle ang_y)
{
    Angle ang_z;
    Angle ang_x;
    NJS_VECTOR local;
    const char *from;

    /* @hack spelling: the vector copied through a byte cursor, as in
     * player_turn_back; through the members, 72 of 75 words differ */
    from = (const char *)vector;
    local.x = *(const Float *)from;
    from += sizeof(Float);
    local.y = *(const Float *)from;
    from += sizeof(Float);
    local.z = *(const Float *)from;
    ang_z = -(Angle)(atan2f(normal->x, normal->y) * ANGLE_PER_RADIAN);
    ang_x = (Angle)(asinf(normal->z) * ANGLE_PER_RADIAN);
    matrix_push_unit();
    TURN(njRotateZ, ang_z);
    TURN(njRotateX, ang_x);
    TURN(njRotateY, -ang_y);
    njCalcVector(NULL, &local, vector);
    njPopMatrix(1);
}

/* a speed this slow or slower counts as none */
#define SPEED_NONE 0.001F

void player_take_speed(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    NJS_VECTOR speed;

    speed = motion->speed;
    if ((work->flags & PLAYER_ON_GROUND) &&
        (record->ground_flags & WORLD_NO_SLOPE)) {
        ground_turn_back(&record->ground_normal, &speed, work->ang.y);
    } else {
        player_turn_back(work, &speed);
    }
    if (work->flags & PLAYER_ON_GROUND) {
        speed.y = 0.0F;
    }
    if (work->flags & PLAYER_ON_GROUND) {
        if (!(record->last_ground_flags &
              (WORLD_GRIP_LEAST | WORLD_GRIP_LOW))) {
            Float grip = record->consts.ground_accel * 0.9F;

            if (speed.x < 0.01F && speed.x > -0.01F) {
                speed.x = 0.0F;
            }
            if (speed.z <= grip && speed.z >= -grip) {
                speed.z = 0.0F;
            }
        }
    } else {
        if (speed.x <= SPEED_NONE && speed.x >= -SPEED_NONE) {
            speed.x = 0.0F;
        }
        if (speed.z <= SPEED_NONE) {
            if (speed.z >= -SPEED_NONE) {
                speed.z = 0.0F;
            }
        }
    }
    record->speed = speed;
    work->ang.x &= 0xffff;
    work->ang.y &= 0xffff;
    work->ang.z &= 0xffff;
    motion->aim.x &= 0xffff;
    motion->aim.y &= 0xffff;
    motion->aim.z &= 0xffff;
    if ((work->flags & PLAYER_ON_GROUND) &&
        !(record->ground_flags & WORLD_NO_SLOPE)) {
        record->level = -njInnerProduct(&world_gravity, &record->ground_normal);
    } else {
        record->level = 1.0F;
    }
}

void player_take_push_speed(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    NJS_VECTOR speed;
    struct task_work *other;

    other = work->collision->hit->owner->work;
    speed = motion->speed;
    other->pos.x += speed.x;
    other->pos.y += speed.y;
    other->pos.z += speed.z;
    player_turn_back(work, &speed);
    if (speed.x <= SPEED_NONE && speed.x >= -SPEED_NONE) {
        speed.x = 0.0F;
    }
    if (speed.z <= SPEED_NONE && speed.z >= -SPEED_NONE) {
        speed.z = 0.0F;
    }
    if (work->flags & PLAYER_ON_GROUND) {
        speed.y = 0.0F;
    }
    record->speed = speed;
}

void player_take_wall_speed(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    NJS_VECTOR speed;
    Float bound;

    speed = motion->speed;
    player_turn_back(work, &speed);
    if (speed.x <= SPEED_NONE && speed.x >= -SPEED_NONE) {
        speed.x = 0.0F;
    }
    if (speed.y <= SPEED_NONE && speed.y >= -SPEED_NONE) {
        speed.y = 0.0F;
    }
    if (speed.z <= SPEED_NONE && speed.z >= -SPEED_NONE) {
        speed.z = 0.0F;
    }
    record->speed = speed;
    if (work->flags & PLAYER_ON_LAND) {
        record->speed.y = 0.0F;
    }
    if (work->pos.y > -18400.0F) {
        if (work->pos.z < -160.0F) {
            work->pos.z = -160.0F;
        }
        if (work->pos.z > 160.0F) {
            work->pos.z = 160.0F;
        }
        if (work->pos.x < -20.0F) {
            work->pos.x = -20.0F;
        }
        if (work->pos.x > 300.0F) {
            work->pos.x = 300.0F;
        }
    } else if (work->pos.y > -18845.0F) {
        bound =
            160.0F + ((-18400.0F - work->pos.y) * (50.0F * (1.0F / 445.0F)));
        if (work->pos.z < -bound) {
            work->pos.z = -bound;
        }
        if (work->pos.z > bound) {
            work->pos.z = bound;
        }
        bound =
            -20.0F + ((-18400.0F - work->pos.y) * (140.0F * (1.0F / 445.0F)));
        if (work->pos.x < bound) {
            work->pos.x = bound;
        }
        if (work->pos.x > 300.0F) {
            work->pos.x = 300.0F;
        }
    } else if (work->pos.y > -19000.0F) {
        if (work->pos.z < -110.0F) {
            work->pos.z = -110.0F;
        }
        if (work->pos.z > 110.0F) {
            work->pos.z = 110.0F;
        }
        if (work->pos.x < 115.0F) {
            work->pos.x = 115.0F;
        }
        if (work->pos.x > 300.0F) {
            work->pos.x = 300.0F;
        }
    }
}

/* narrow an angle in place to its low 16 bits, sign-extended */
/* @hack spelling: the angle read into wide and narrowed from it, which
 * loads each address and value into R4 and R5 (0x8c07eab4 to 0x8c07eae2);
 * narrowed in place, R2 and R3 take the values and 127 of
 * player_anim_make_blend's 812 words differ */
#define NARROW_ANGLE(ang)                                                      \
    {                                                                          \
        Angle wide = (ang);                                                    \
        (ang) = (Sint16)wide;                                                  \
    }

void player_anim_step(struct player_anim *anim)
{
    Uint16 index = anim->action;
    NJS_ACTION *action;

    anim->previous = anim->current;
    anim->flags = 0;
    if (anim->current != anim->action && anim->state != PLAYER_ANIM_BLENDING &&
        anim->state != PLAYER_ANIM_NEW && anim->state != PLAYER_ANIM_FREE) {
        anim->state = PLAYER_ANIM_CHANGE;
        anim->updates = 0;
    } else {
        anim->updates++;
    }
    switch (anim->state) {
    case PLAYER_ANIM_NEW:
        anim->frame = 0.0F;
        anim->current = index;
        anim->state = PLAYER_ANIM_CHANGE;
        anim->frame = anim->start_frame = 0.0F;
        anim->blend = heap_alloc(sizeof(NJS_ACTION));
        anim->blend->object = anim->actions[index].action->object;
        anim->blend->motion = heap_alloc(sizeof(NJS_MOTION));
        anim->blend->motion->nbFrame = 2;
        anim->blend->motion->type = NJD_MTYPE_POS_0 | NJD_MTYPE_ANG_1;
        anim->blend->motion->inp_fn = 2;
        anim->blend->motion->mdata =
            heap_calloc(PLAYER_BLEND_NODES, sizeof(NJS_MDATA3));
        break;
    case PLAYER_ANIM_CHANGE:
        if (anim->actions[index].blend != 1.0F &&
            anim->actions[index].action->object ==
                anim->actions[anim->current].action->object) {
            player_anim_blend_from(anim);
            anim->frame = 0.0F;
        } else {
            anim->frame = 1.0F;
        }
        anim->current = index;
        anim->state = PLAYER_ANIM_BLENDING;
        anim->next = (Uint8)anim->actions[index].next;
        /* fall through */
    case PLAYER_ANIM_BLENDING:
        anim->frame += anim->actions[index].blend;
        if (anim->frame >= 1.0F) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            anim->state = anim->actions[index].mode;
            player_anim_free_blend(anim);
            player_anim_start(anim);
        }
        break;
    case PLAYER_ANIM_LOOP:
        action = anim->actions[index].action;
        anim->frame += anim->actions[index].speed;
        while (anim->frame >= (Float)action->motion->nbFrame) {
            anim->frame -= (Float)action->motion->nbFrame;
        }
        while (0.0F > anim->frame) {
            anim->frame += (Float)action->motion->nbFrame;
        }
        break;
    case PLAYER_ANIM_LOOP_SPEED:
        action = anim->actions[index].action;
        anim->frame += *anim->speed * anim->actions[index].speed;
        while (anim->frame >= (Float)action->motion->nbFrame) {
            anim->frame -= (Float)action->motion->nbFrame;
        }
        while (0.0F > anim->frame) {
            anim->frame += (Float)action->motion->nbFrame;
        }
        break;
    case PLAYER_ANIM_LOOP_PACE:
        action = anim->actions[index].action;
        anim->frame += *anim->pace * anim->actions[index].speed;
        while (anim->frame >= (Float)action->motion->nbFrame) {
            anim->frame -= (Float)action->motion->nbFrame;
        }
        while (0.0F > anim->frame) {
            anim->frame += (Float)action->motion->nbFrame;
        }
        break;
    case PLAYER_ANIM_ONCE_PACE:
        action = anim->actions[index].action;
        anim->frame += *anim->pace;
        if (anim->frame >= (Float)action->motion->nbFrame - 1.0F) {
            player_anim_play_next(anim, index);
        }
        break;
    case PLAYER_ANIM_ONCE:
        action = anim->actions[index].action;
        anim->frame += anim->actions[index].speed;
        if (anim->frame >= (Float)action->motion->nbFrame - 1.0F) {
            player_anim_play_next(anim, index);
        }
        break;
    case PLAYER_ANIM_BACK:
        action = anim->actions[index].action;
        anim->frame -= anim->actions[index].speed;
        if (0.0F > anim->frame) {
            player_anim_play_next(anim, index);
        }
        break;
    case PLAYER_ANIM_BLEND_NEXT:
        action = anim->actions[index].action;
        anim->frame += anim->actions[index].speed;
        if (anim->frame >= (Float)action->motion->nbFrame - 1.0F) {
            anim->updates = 0;
            if (anim->actions[index].blend != 1.0F &&
                anim->actions[index].action->object ==
                    anim->actions[anim->next].action->object) {
                index = anim->next;
                anim->action = (Uint8)index;
                player_anim_blend_from(anim);
                anim->frame = 0.0F;
            } else {
                anim->frame = 1.0F;
            }
            anim->current = index;
            anim->next = (Uint8)anim->actions[index].next;
            anim->state = PLAYER_ANIM_BLENDING;
            anim->frame += anim->actions[index].blend;
            if (anim->frame >= 1.0F) {
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                anim->state = anim->actions[index].mode;
                player_anim_free_blend(anim);
                player_anim_start(anim);
            }
        }
        break;
    case PLAYER_ANIM_HOLD:
        action = anim->actions[index].action;
        anim->frame += anim->actions[index].speed;
        if (anim->frame >= (Float)action->motion->nbFrame - 1.0F) {
            anim->frame = (Float)action->motion->nbFrame - 1.0F;
            anim->flags |= PLAYER_ACTION_DONE;
        }
        break;
    case PLAYER_ANIM_BACK_HOLD:
        action = anim->actions[index].action;
        anim->frame -= anim->actions[index].speed;
        if (0.0F > anim->frame) {
            anim->frame = 0.0F;
            anim->flags |= PLAYER_ACTION_DONE;
        }
        break;
    case PLAYER_ANIM_DRIVEN:
        action = anim->actions[index].action;
        while (anim->frame >= (Float)action->motion->nbFrame) {
            anim->frame -= (Float)action->motion->nbFrame;
        }
        while (0.0F > anim->frame) {
            anim->frame += (Float)action->motion->nbFrame;
        }
        break;
    /* @hack redundant: case PLAYER_ANIM_FREE beside the default, which runs the
     * switch's table to state 14 (MOV #15,R1 at 0x8c07df18); without it,
     * the table stops at 12, the function runs 32 bytes short and 711 of
     * 776 words differ */
    case PLAYER_ANIM_FREE:
    default:
        player_anim_free_blend(anim);
        if (anim->blend != NULL) {
            if (anim->blend->motion->mdata != NULL) {
                heap_free(anim->blend->motion->mdata);
                anim->blend->motion->mdata = NULL;
            }
            if (anim->blend->motion != NULL) {
                heap_free(anim->blend->motion);
                anim->blend->motion = NULL;
            }
            heap_free(anim->blend);
            anim->blend = NULL;
        }
        break;
    }
}

static void player_anim_play_next(struct player_anim *anim, Uint16 index)
{
    anim->action = (Uint8)anim->actions[index].next;
    anim->current = anim->action;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    anim->state = anim->actions[anim->current].mode;
    anim->next = (Uint8)anim->actions[anim->action].next;
    anim->flags |= PLAYER_ACTION_DONE;
    player_anim_start(anim);
}

static void player_anim_start(struct player_anim *anim)
{
    if (anim->state == PLAYER_ANIM_BACK ||
        anim->state == PLAYER_ANIM_BACK_HOLD) {
        anim->frame =
            (Float)anim->actions[anim->current].action->motion->nbFrame - 1.0F;
    } else if (anim->state == PLAYER_ANIM_LOOP ||
               anim->state == PLAYER_ANIM_LOOP_PACE ||
               anim->state == PLAYER_ANIM_LOOP_SPEED ||
               anim->state == PLAYER_ANIM_DRIVEN) {
        anim->frame = anim->start_frame;
        anim->start_frame = 0.0F;
    } else {
        anim->frame = 0.0F;
    }
}

static void player_anim_blend_from(struct player_anim *anim)
{
    NJS_MOTION *next;
    NJS_MOTION *prev;
    Uint16 next_n;
    Uint16 prev_n;
    Uint16 index = anim->action;
    Uint16 current = anim->current;

    next = anim->actions[index].action->motion;
    prev = anim->actions[current].action->motion;
    if (next->type & NJD_MTYPE_SCL_2) {
        next_n = 3;
    } else {
        next_n = 2;
    }
    if (prev->type & NJD_MTYPE_SCL_2) {
        prev_n = 3;
    } else {
        prev_n = 2;
    }
    /* @hack redundant: both arms make the blend, but the image still tests
     * the kinds (CMP/EQ #3 at 0x8c07e6a8 and 0x8c07e6b0); as one call, 39 of
     * 43 words differ */
    if (next_n == 3 || prev_n == 3) {
        player_anim_make_blend(anim);
    } else {
        player_anim_make_blend(anim);
    }
}

static void player_anim_make_blend(struct player_anim *anim)
{
    NJS_OBJECT *nodes[PLAYER_BLEND_NODES];
    NJS_OBJECT *object;
    NJS_OBJECT *node;
    NJS_MOTION *next;
    NJS_MOTION *prev;
    Uint16 next_n;
    Uint16 prev_n;
    NJS_MDATA2 *src;
    NJS_MDATA2 *old;
    NJS_MDATA2 *dst;
    Uint32 i;
    Uint16 index = anim->action;
    Uint16 current = anim->current;

    object = anim->actions[index].action->object;
    anim->blend->object = object;
    tree_list_nodes(object, nodes);
    next = anim->actions[index].action->motion;
    prev = anim->actions[current].action->motion;
    if (next->type & NJD_MTYPE_SCL_2) {
        next_n = 3;
    } else {
        next_n = 2;
    }
    if (prev->type & NJD_MTYPE_SCL_2) {
        prev_n = 3;
    } else {
        prev_n = 2;
    }
    src = next->mdata;
    old = prev->mdata;
    dst = anim->blend->motion->mdata;
    for (i = 0; i < anim->actions[index].nodes; i++) {
        node = nodes[i];
        if (src->p[0] != NULL || old->p[0] != NULL) {
            NJS_MKEY_F *key = heap_alloc(2 * sizeof(NJS_MKEY_F));

            dst->p[0] = key;
            key->keyframe = 0;
            if (old->p[0] != NULL) {
                NJS_MKEY_F *keys = old->p[0];
                Uint32 nb = old->nb[0];
                Uint32 start = 0;
                Uint32 end = 0;
                Uint32 span;
                Float share;

                while (anim->frame > (Float)keys[end].keyframe) {
                    if (end >= nb - 1) {
                        end = 0;
                        start = nb - 1;
                        break;
                    }
                    start = end;
                    end++;
                }
                key->key[0] = keys[start].key[0];
                key->key[1] = keys[start].key[1];
                key->key[2] = keys[start].key[2];
                if (end != start) {
                    span = keys[end].keyframe - keys[start].keyframe;
                    if (end < start) {
                        span += prev->nbFrame;
                    }
                    share = (anim->frame - (Float)keys[start].keyframe) /
                            (Float)span;
                    key->key[0] +=
                        (keys[end].key[0] - keys[start].key[0]) * share;
                    key->key[1] +=
                        (keys[end].key[1] - keys[start].key[1]) * share;
                    key->key[2] +=
                        (keys[end].key[2] - keys[start].key[2]) * share;
                }
            } else {
                key->key[0] = node->pos[0];
                key->key[1] = node->pos[1];
                key->key[2] = node->pos[2];
            }
            key[1].keyframe = 1;
            if (src->p[0] != NULL) {
                NJS_MKEY_F *keys = src->p[0];

                key[1].key[0] = keys->key[0];
                key[1].key[1] = keys->key[1];
                key[1].key[2] = keys->key[2];
            } else {
                key[1].key[0] = node->pos[0];
                key[1].key[1] = node->pos[1];
                key[1].key[2] = node->pos[2];
            }
            dst->nb[0] = 2;
        } else {
            dst->p[0] = NULL;
            dst->nb[0] = 0;
        }
        if (src->p[1] != NULL || old->p[1] != NULL) {
            NJS_MKEY_A *key = heap_alloc(2 * sizeof(NJS_MKEY_A));

            dst->p[1] = key;
            key->keyframe = 0;
            if (old->p[1] != NULL) {
                NJS_MKEY_A *keys = old->p[1];
                Uint32 nb = old->nb[1];
                Uint32 start = 0;
                Uint32 end = 0;
                Uint32 span;
                Float share;

                while (anim->frame > (Float)keys[end].keyframe) {
                    if (end >= nb - 1) {
                        end = 0;
                        start = nb - 1;
                        break;
                    }
                    start = end;
                    end++;
                }
                key->key[0] = keys[start].key[0];
                key->key[1] = keys[start].key[1];
                key->key[2] = keys[start].key[2];
                if (end != start) {
                    span = keys[end].keyframe - keys[start].keyframe;
                    if (end < start) {
                        span += prev->nbFrame;
                    }
                    share = (anim->frame - (Float)keys[start].keyframe) /
                            (Float)span;
                    key->key[0] +=
                        (Angle)((Float)angle_difference(keys[start].key[0],
                                                        keys[end].key[0]) *
                                share);
                    key->key[1] +=
                        (Angle)((Float)angle_difference(keys[start].key[1],
                                                        keys[end].key[1]) *
                                share);
                    key->key[2] +=
                        (Angle)((Float)angle_difference(keys[start].key[2],
                                                        keys[end].key[2]) *
                                share);
                }
            } else {
                key->key[0] = node->ang[0];
                key->key[1] = node->ang[1];
                key->key[2] = node->ang[2];
            }
            key[1].keyframe = 1;
            if (src->p[1] != NULL) {
                NJS_MKEY_A *keys = src->p[1];

                key[1].key[0] = keys->key[0];
                key[1].key[1] = keys->key[1];
                key[1].key[2] = keys->key[2];
            } else {
                key[1].key[0] = node->ang[0];
                key[1].key[1] = node->ang[1];
                key[1].key[2] = node->ang[2];
            }
            NARROW_ANGLE(key->key[0]);
            NARROW_ANGLE(key->key[1]);
            NARROW_ANGLE(key->key[2]);
            NARROW_ANGLE(key[1].key[0]);
            NARROW_ANGLE(key[1].key[1]);
            NARROW_ANGLE(key[1].key[2]);
            key[1].key[0] = angle_unwrap(key->key[0], key[1].key[0]);
            key[1].key[1] = angle_unwrap(key->key[1], key[1].key[1]);
            key[1].key[2] = angle_unwrap(key->key[2], key[1].key[2]);
            dst->nb[1] = 2;
        } else {
            dst->p[1] = NULL;
            dst->nb[1] = 0;
        }
        if (prev_n == 3 || next_n == 3) {
            NJS_MDATA3 *dst3;
            NJS_MDATA3 *src3;
            NJS_MDATA3 *old3;

            src3 = (NJS_MDATA3 *)src;
            dst3 = (NJS_MDATA3 *)dst;
            old3 = (NJS_MDATA3 *)old;
            dst3->nb[1] = dst->nb[1];
            dst3->nb[0] = dst->nb[0];
            if ((next_n == 3 && src3->p[2] != NULL) ||
                (prev_n == 3 && old3->p[2] != NULL)) {
                NJS_MKEY_F *key = heap_alloc(2 * sizeof(NJS_MKEY_F));

                dst3->p[2] = key;
                key->keyframe = 0;
                if (prev_n == 3 && old3->p[2] != NULL) {
                    /* @hack spelling: start declared ahead of nb, so that
                     * end's zero copies from start in R6 (MOV R6,R5 at
                     * 0x8c07ebcc); declared after nb, 1 of 812 words
                     * differ */
                    NJS_MKEY_F *keys = old3->p[2];
                    Uint32 start = 0;
                    Uint32 nb = old3->nb[2];
                    Uint32 end = 0;
                    Uint32 span;
                    Float share;

                    while (anim->frame > (Float)keys[end].keyframe) {
                        if (end >= nb - 1) {
                            end = 0;
                            start = nb - 1;
                            break;
                        }
                        start = end;
                        end++;
                    }
                    key->key[0] = keys[start].key[0];
                    key->key[1] = keys[start].key[1];
                    key->key[2] = keys[start].key[2];
                    if (end != start) {
                        span = keys[end].keyframe - keys[start].keyframe;
                        if (end < start) {
                            span += prev->nbFrame;
                        }
                        share = (anim->frame - (Float)keys[start].keyframe) /
                                (Float)span;
                        key->key[0] +=
                            (keys[end].key[0] - keys[start].key[0]) * share;
                        key->key[1] +=
                            (keys[end].key[1] - keys[start].key[1]) * share;
                        key->key[2] +=
                            (keys[end].key[2] - keys[start].key[2]) * share;
                    }
                } else {
                    key->key[0] = node->scl[0];
                    key->key[1] = node->scl[1];
                    key->key[2] = node->scl[2];
                }
                key[1].keyframe = 1;
                if (next_n == 3 && src3->p[2] != NULL) {
                    NJS_MKEY_F *keys = src3->p[2];

                    key[1].key[0] = keys->key[0];
                    key[1].key[1] = keys->key[1];
                    key[1].key[2] = keys->key[2];
                } else {
                    key[1].key[0] = node->scl[0];
                    key[1].key[1] = node->scl[1];
                    key[1].key[2] = node->scl[2];
                }
                dst3->nb[2] = 2;
            } else {
                dst3->p[2] = NULL;
                dst3->nb[2] = 0;
            }
        }
        if (next_n == 3 || prev_n == 3) {
            dst = (NJS_MDATA2 *)((NJS_MDATA3 *)dst + 1);
        } else {
            dst++;
        }
        if (next_n == 3) {
            src = (NJS_MDATA2 *)((NJS_MDATA3 *)src + 1);
        } else {
            src++;
        }
        if (prev_n == 3) {
            old = (NJS_MDATA2 *)((NJS_MDATA3 *)old + 1);
        } else {
            old++;
        }
    }
    if (next_n == 3 || prev_n == 3) {
        anim->blend->motion->type =
            NJD_MTYPE_POS_0 | NJD_MTYPE_ANG_1 | NJD_MTYPE_SCL_2;
        anim->blend->motion->inp_fn = 3;
    } else {
        anim->blend->motion->type = NJD_MTYPE_POS_0 | NJD_MTYPE_ANG_1;
        anim->blend->motion->inp_fn = 2;
    }
}

static void player_anim_free_blend(struct player_anim *anim)
{
    NJS_MOTION *motion = anim->blend->motion;
    void **keys = (void **)motion->mdata;
    Uint16 kinds = motion->inp_fn;
    int i;

    for (i = anim->actions[anim->action].nodes; i > 0; i--) {
        if (*keys != NULL) {
            heap_free(*keys);
            *keys = NULL;
            keys[kinds] = NULL;
        }
        keys++;
        if (*keys != NULL) {
            heap_free(*keys);
            *keys = NULL;
            keys[kinds] = NULL;
        }
        keys++;
        if (kinds == 3) {
            if (*keys != NULL) {
                heap_free(*keys);
                *keys = NULL;
                keys[kinds] = NULL;
            }
        }
        if (kinds == 3) {
            keys += 4;
        } else {
            keys += 2;
        }
    }
}

void player_update_welds(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    struct player_anim *anim;
    struct weld *weld;
    NJS_MODEL *model_a;
    NJS_MODEL *model_b;
    NJS_POINT3 *vertex_a;
    NJS_POINT3 *vertex_b;
    NJS_POINT3 *points_a;
    NJS_POINT3 *points_b;
    NJS_VECTOR *normals_a;
    NJS_VECTOR *normals_b;
    NJS_POINT3 point;
    NJS_POINT3 other;
    Uint16 i;
    int index_a;
    int index_b;

    anim = &record->anim;
    weld = anim->welds;
    switch (anim->weld_mode) {
    case PLAYER_WELD_MAKE:
        for (; weld->base != NULL; weld++) {
            switch (weld->kind) {
            case WELD_MARK:
            case WELD_MARK + 1:
            case WELD_MARK + 2:
            case WELD_MARK + 3:
            case WELD_MARK + 4:
            case WELD_MARK + 5:
                weld->vertices = NULL;
                weld->node_a = tree_node_index(weld->base, weld->object_a);
                weld->node_b = 0;
                break;
            default:
                weld->vertices =
                    heap_alloc(weld->count * (4 * sizeof(NJS_POINT3)));
                model_a = weld->object_a->model;
                model_b = weld->object_b->model;
                for (i = 0; i < weld->count; i++) {
                    weld->vertices[i * 4] =
                        model_a->points[weld->indices[i * 2]];
                    weld->vertices[(i * 4) + 1] =
                        model_a->normals[weld->indices[i * 2]];
                    weld->vertices[(i * 4) + 2] =
                        model_b->points[weld->indices[(i * 2) + 1]];
                    weld->vertices[(i * 4) + 3] =
                        model_b->normals[weld->indices[(i * 2) + 1]];
                }
                weld->node_a = tree_node_index(weld->base, weld->object_a);
                weld->node_b = tree_node_index(weld->base, weld->object_b);
                /* fall through */
            case WELD_OFF:
                break;
            }
        }
        anim->weld_mode = PLAYER_WELDING;
        break;
    case PLAYER_WELDING:
        if (anim->state == PLAYER_ANIM_BLENDING) {
            pose_action(&_unit_matrix_, anim->blend, anim->frame,
                        anim->actions[anim->action].nodes, NULL);
        } else {
            pose_action(&_unit_matrix_, anim->actions[anim->action].action,
                        anim->frame, anim->actions[anim->action].nodes, NULL);
        }
        while (weld->base != NULL) {
            if (weld->base == anim->actions[anim->action].action->object) {
                if (weld->node_a == 0xff) {
                    weld->node_a = tree_node_index(weld->base, weld->object_a);
                    weld++;
                    continue;
                }
                pose_get_matrix(weld->node_a, &player_weld_matrix_a);
                if (weld->object_b != NULL) {
                    points_a = weld->object_a->model->points;
                    normals_a = weld->object_a->model->normals;
                    njSetMatrix(&player_weld_inverse_a, &player_weld_matrix_a);
                    njInvertMatrix(&player_weld_inverse_a);
                    if (weld->node_b == 0xff) {
                        weld->node_b =
                            tree_node_index(weld->base, weld->object_b);
                        weld++;
                        continue;
                    }
                    pose_get_matrix(weld->node_b, &player_weld_matrix_b);
                    points_b = weld->object_b->model->points;
                    normals_b = weld->object_b->model->normals;
                    njSetMatrix(&player_weld_inverse_b, &player_weld_matrix_b);
                    njInvertMatrix(&player_weld_inverse_b);
                } else {
                    point.x = point.y = point.z = 0.0F;
                    other.x = other.y = other.z = 0.0F;
                    if (weld->count == 0) {
                        other.x = 1.0F;
                    } else if (weld->count == 1) {
                        other.y = 1.0F;
                    } else {
                        other.z = 1.0F;
                    }
                    njPushMatrix(&player_weld_matrix_a);
                }
                switch (weld->kind) {
                case WELD_BOTH:
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        vertex_a = &weld->vertices[i * 4];
                        vertex_b = &weld->vertices[(i * 4) + 2];
                        njCalcPoint(&player_weld_matrix_a, vertex_a, &point);
                        njCalcPoint(&player_weld_matrix_b, vertex_b, &other);
                        point.x = (point.x + other.x) * 0.5F;
                        point.y = (point.y + other.y) * 0.5F;
                        point.z = (point.z + other.z) * 0.5F;
                        njCalcPoint(&player_weld_inverse_a, &point,
                                    &points_a[index_a]);
                        njCalcPoint(&player_weld_inverse_b, &point,
                                    &points_b[index_b]);
                        njCalcVector(&player_weld_matrix_a,
                                     &weld->vertices[(i * 4) + 1], &point);
                        njCalcVector(&player_weld_matrix_b,
                                     &weld->vertices[(i * 4) + 3], &other);
                        point.x += other.x;
                        point.y += other.y;
                        point.z += other.z;
                        njUnitVector(&point);
                        njCalcVector(&player_weld_inverse_a, &point,
                                     &normals_a[index_a]);
                        njCalcVector(&player_weld_inverse_b, &point,
                                     &normals_b[index_b]);
                    }
                    break;
                case WELD_TO_A:
                    njPushMatrix(&player_weld_inverse_b);
                    njMultiMatrix(NULL, &player_weld_matrix_a);
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        points_a[index_a] = weld->vertices[i * 4];
                        njCalcPoint(NULL, &weld->vertices[i * 4],
                                    &points_b[index_b]);
                        normals_a[index_a] = weld->vertices[(i * 4) + 1];
                        njCalcVector(NULL, &weld->vertices[(i * 4) + 1],
                                     &normals_b[index_b]);
                    }
                    njPopMatrix(1);
                    break;
                case WELD_TO_B:
                    njPushMatrix(&player_weld_inverse_a);
                    njMultiMatrix(NULL, &player_weld_matrix_b);
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        points_b[index_b] = weld->vertices[(i * 4) + 2];
                        njCalcPoint(NULL, &weld->vertices[(i * 4) + 2],
                                    &points_a[index_a]);
                        normals_b[index_b] = weld->vertices[(i * 4) + 3];
                        njCalcVector(NULL, &weld->vertices[(i * 4) + 3],
                                     &normals_a[index_a]);
                    }
                    njPopMatrix(1);
                    break;
                case WELD_MARK:
                    njCalcPoint(NULL, &point, &record->weld_point0);
                    njCalcVector(NULL, &other, &record->weld_axis0);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 1:
                    njCalcPoint(NULL, &point, &record->weld_point1);
                    njCalcVector(NULL, &other, &record->weld_axis1);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 2:
                    njCalcPoint(NULL, &point, &record->weld_point2);
                    njCalcVector(NULL, &other, &record->weld_axis2);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 3:
                    njCalcPoint(NULL, &point, &record->weld_point3);
                    njCalcVector(NULL, &other, &record->weld_axis3);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 4:
                    njCalcPoint(NULL, &point, &record->weld_point4);
                    njCalcVector(NULL, &other, &record->weld_axis4);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 5:
                    njCalcPoint(NULL, &point, &record->weld_point5);
                    njCalcVector(NULL, &other, &record->weld_axis5);
                    njPopMatrix(1);
                    break;
                }
            }
            weld++;
        }
        break;
    case PLAYER_WELD_FREE:
    default:
        for (; weld->base != NULL; weld++) {
            if (weld->vertices != NULL) {
                model_a = weld->object_a->model;
                model_b = weld->object_b->model;
                for (i = 0; i < weld->count; i++) {
                    index_a = weld->indices[i * 2];
                    index_b = weld->indices[(i * 2) + 1];
                    model_a->points[index_a] = weld->vertices[i * 4];
                    model_a->normals[index_a] = weld->vertices[(i * 4) + 1];
                    model_b->points[index_b] = weld->vertices[(i * 4) + 2];
                    model_b->normals[index_b] = weld->vertices[(i * 4) + 3];
                }
                heap_free(weld->vertices);
                weld->vertices = NULL;
            }
        }
        break;
    }
}

void player_look_turn_head(struct player_look *look)
{
    int ang;

    ang = angle_approach(look->head_y, look->head_aim_y, 0x1000);
    /* above 0x8000, the angle turns negative. 0x8000 itself stays
     * positive */
    if (ang > 0x8000) {
        ang |= 0xffff0000;
    }
    if (ang > look->head_max_y) {
        ang = look->head_max_y;
    } else if (ang < look->head_min_y) {
        ang = look->head_min_y;
    }
    look->head_y = ang;
    look->head->ang[1] = (Uint16)look->head_y;
    ang = angle_approach(look->head_z, look->head_aim_z, 0x1000);
    if (ang > 0x8000) {
        ang |= 0xffff0000;
    }
    if (ang > look->head_max_z) {
        ang = look->head_max_z;
    } else if (ang < look->head_min_z) {
        ang = look->head_min_z;
    }
    look->head_z = ang;
    look->head->ang[2] = (Uint16)look->head_z;
}

void player_look_turn_eyes(struct player_look *look)
{
    int ang;

    ang = angle_approach(look->eye0_y, look->eyes_aim_y, 0x800);
    if (ang & 0x8000) {
        ang |= 0xffff0000;
    }
    if (ang > look->eye0_max_y) {
        ang = look->eye0_max_y;
    } else if (ang < look->eye0_min_y) {
        ang = look->eye0_min_y;
    }
    look->eye0_y = ang;
    look->eye0->ang[1] = look->eye0_y;
    ang = angle_approach(look->eye1_y, look->eyes_aim_y, 0x800);
    if (ang & 0x8000) {
        ang |= 0xffff0000;
    }
    if (ang < look->eye1_min_y) {
        ang = look->eye1_min_y;
    } else if (ang > look->eye1_max_y) {
        ang = look->eye1_max_y;
    }
    look->eye1_y = ang;
    look->eye1->ang[1] = look->eye1_y;
    ang = angle_approach(look->eye0_z, look->eyes_aim_z, 0x800);
    if (ang & 0x8000) {
        ang |= 0xffff0000;
    }
    if (ang < look->eyes_min_z) {
        ang = look->eyes_min_z;
    } else if (ang > look->eyes_max_z) {
        ang = look->eyes_max_z;
    }
    look->eye0_z = ang;
    look->eye0->ang[2] = look->eye0_z;
    look->eye1_z = ang;
    look->eye1->ang[2] = look->eye1_z;
}

/* blend the points of object, and of each object under and after it in the
 * tree, from shape mouth->from toward shape mouth->to by mouth->amount. a
 * node's shapes are its keys in mouth->shapes, and a node with none is left
 * as it is. *index is object's node number on entry, and on return the
 * number of the last node reached */
static void player_mouth_blend(NJS_OBJECT *object, struct player_mouth *mouth,
                               Sint16 *index)
{
    NJS_MDATA2 *data = (NJS_MDATA2 *)mouth->shapes->mdata;
    NJS_POINT3 *points = object->model->points;
    NJS_OBJECT *next;

    data += *index;
    if (data->p[0] != NULL) {
        NJS_POINT3 *from = ((NJS_MKEY_P *)data->p[0])[mouth->from].key;
        NJS_POINT3 *to = ((NJS_MKEY_P *)data->p[0])[mouth->to].key;

        points_blend(from, to, points, mouth->amount, object->model->nbPoint);
    }
    next = object->child;
    if (next != NULL) {
        ++*index;
        player_mouth_blend(next, mouth, index);
    }
    next = object->sibling;
    if (next == NULL) {
        return;
    }
    ++*index;
    player_mouth_blend(next, mouth, index);
}

void player_mouth_destroy(struct task *task)
{
    struct player_mouth *mouth = (struct player_mouth *)task->motion;

    if (mouth == NULL) {
        return;
    }
    mouth->face->sibling = mouth->mouth;
}

void player_mouth_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[PLAYER_VARS(work)->player];
    struct player_mouth *mouth = (struct player_mouth *)task->motion;
    struct event_mouth *talk;
    NJS_OBJECT *blend;
    Sint16 index;

    if (player == NULL) {
        task_kill(task);
        return;
    }
    talk = &player->event->mouth;
    event_mouth_step(player);
    switch (work->mode) {
    case MOUTH_INIT:
        mouth->key = 0;
        player_records[PLAYER_VARS(work)->player]->mouth = mouth;
        if (mouth->script == NULL) {
            mouth->face->sibling = mouth->mouth;
            mouth->amount = 0.0F;
            mouth->step = 0.0F;
            work->mode = MOUTH_STILL;
            break;
        }
        work->mode = MOUTH_NEXT_KEY;
        /* fall through */
    case MOUTH_NEXT_KEY:
        mouth->from = mouth->to;
        if (mouth->script == NULL || mouth->script[mouth->key].steps == 0) {
            mouth->face->sibling = mouth->mouth;
            mouth->amount = 0.0F;
            mouth->step = 0.0F;
            mouth->to = 0;
            mouth->key = 0;
            mouth->script = NULL;
            work->mode = MOUTH_STILL;
            break;
        }
        mouth->to = mouth->script[mouth->key].shape;
        mouth->amount = 0.0F;
        mouth->step = 1.0F / mouth->script[mouth->key].steps;
        work->mode = MOUTH_SCRIPT;
        /* fall through */
    case MOUTH_SCRIPT:
        if (mouth->script == NULL) {
            work->mode = MOUTH_NEXT_KEY;
            break;
        }
        mouth->amount += mouth->step;
        if (mouth->amount >= 1.0F) {
            mouth->amount = 1.0F;
        }
        blend = mouth->blend;
        index = 0;
        player_mouth_blend(blend, mouth, &index);
        if (mouth->amount >= 1.0F) {
            work->mode = MOUTH_NEXT_KEY;
            ++mouth->key;
        }
        mouth->face->sibling = mouth->blend;
        break;
    case MOUTH_STILL:
        if (talk->steps != 0) {
            work->mode = MOUTH_TALK;
        } else if (mouth->script != NULL) {
            work->mode = MOUTH_NEXT_KEY;
            break;
        } else {
            mouth->amount = 0.0F;
            mouth->step = 0.0F;
            mouth->face->sibling = mouth->mouth;
            break;
        }
        /* fall through */
    case MOUTH_TALK:
        if (talk->steps == 0) {
            work->mode = MOUTH_STILL;
            break;
        }
        mouth->from = talk->shape;
        mouth->to = talk->next_shape;
        blend = mouth->blend;
        mouth->amount = (Float)talk->timer / (Float)talk->steps;
        index = 0;
        player_mouth_blend(blend, mouth, &index);
        mouth->face->sibling = mouth->blend;
        break;
    }
}

void player_mouth_set_script(Sint8 player, int script)
{
    if (player_records[(Uint8)player] != NULL) {
        struct player_mouth *mouth = player_records[(Uint8)player]->mouth;

        if (mouth != NULL) {
            mouth->script = event_mouth_scripts[script];
        }
    }
}

void player_mouth_stop(Uint8 player)
{
    if (player_records[player] != NULL) {
        struct player_mouth *mouth = player_records[player]->mouth;

        if (mouth != NULL) {
            mouth->script = NULL;
        }
    }
}

void player_crash_burst(struct task_work *work)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 6, crash_stars_step)) != NULL) {
        PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
    }
}

void player_make_ripple(Uint8 player, NJS_POINT3 *pos, Angle *ang)
{
    struct task_work *work = player_work[player];
    struct player *record = player_records[player];
    NJS_VECTOR growth;
    Float fall;

    if (pos->y > work->pos.y + record->consts.height) {
        return;
    }
    if (njScalor(&record->speed) == 0.0F) {
        return;
    }
    if ((Float)rand() * RAND_SCALE <= 0.85F) {
        return;
    }
    fall = -record->speed.y * 0.005F;
    growth.x = fall + 0.03F;
    growth.y = 0.0F;
    growth.z = 0.85F;
    ripple_create(pos, &growth, record->consts.radius * 0.1F);
}

#define SE_SPLASH 717

void player_make_splash(struct task_work *work)
{
    NJS_VECTOR speed;
    int i;

    speed.y =
        _builtin_fabsf(player_records[PLAYER_VARS(work)->player]->speed.y) *
        0.5F;
    for (i = 0; i < 20; i++) {
        speed.x = (0.5F - ((Float)rand() * RAND_SCALE)) * 2.0F;
        speed.z = (0.5F - ((Float)rand() * RAND_SCALE)) * 2.0F;
        splash_create(&work->pos, &speed, 0.2F);
    }
    sound_play(SE_SPLASH, 0, 0, 0);
}

#define DUST_SCALE 0.15F

/* @hack redundant: register on record, read once, which keeps work in R13
 * and rand's address in R14 (0x8c07ff38, 0x8c07ff60); without it, the two
 * trade and 14 of 202 words differ */
void player_kick_dust(struct task_work *work, struct task_motion *motion,
                      register struct player *record)
{
    Float speed;
    Float height;
    Float size;
    /* @hack type: volatile on puff, which keeps the particle in a
     * stack slot across the calls that fill it (ADD #-4,R15 at 0x8c07ff30);
     * without it, puff takes a register, the function runs 32 bytes short
     * and 200 of 202 words differ */
    struct particle *volatile puff;

    if (record->ground_flags &
        (WORLD_LIQUID | 0x8000 | WORLD_GRIP_LOW | WORLD_GRIP_LEAST)) {
        return;
    }
    speed = njScalor2(&motion->speed);
    /* @hack spelling: height and size set in the test, by commas, which
     * copies speed into FR15 ahead of the products and puts rand's pool word
     * before 0.04F's (0x8c07ff4c, 0x8c07ff4e); as statements ahead of the
     * if, 8 of 202 words differ */
    if ((height = speed * 0.04F, size = speed * 0.01F, speed > 12.0F) &&
        (Float)rand() * RAND_SCALE > 0.3F) {
        if ((puff = particle_alloc(dust_pool.pool, dust_rise_step)) != NULL) {
            puff->pos.x = work->pos.x - (motion->speed.x * 0.5F);
            puff->pos.y = work->pos.y + height - (motion->speed.y * 0.5F);
            puff->pos.z = work->pos.z - (motion->speed.z * 0.5F);
            puff->scale =
                (size + ((Float)rand() * (0.02F * RAND_SCALE))) * DUST_SCALE;
            puff->time = (Float)rand() * (5.0F * RAND_SCALE);
            puff->ang =
                (Angle)((Float)rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
        }
    }
    if (speed > 2.0F && (Float)rand() * RAND_SCALE > 0.3F) {
        if ((puff = particle_alloc(dust_pool.pool, dust_rise_step)) != NULL) {
            puff->pos.x = work->pos.x;
            puff->pos.y = work->pos.y + height;
            puff->pos.z = work->pos.z;
            puff->scale =
                (size + ((Float)rand() * (0.01F * RAND_SCALE))) * DUST_SCALE;
            puff->time = (Float)rand() * (7.0F * RAND_SCALE);
            puff->ang =
                (Angle)((Float)rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
        }
    }
}

#define SE_BOARD_ICE 237
#define SE_BOARD_SAND_RUN 691

/* icecap_board_spray, in Ice Cap's module, STG08, at this offset into the
 * module area. the call runs whatever module is loaded there */
#define ICECAP_BOARD_SPRAY ((void (*)(int player))(MODULE_AREA + 0xb20))

void player_board_spray(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    Float drag;
    /* @hack spelling: 0.0F held in zero for every store, which loads it
     * with FLDI0 FR13 ahead of the test (0x8c08021e); stored as 0.0F, the
     * FLDI0 comes after it, and 2 of 284 words differ */
    Float zero;
    Float scale;
    /* @hack redundant: register on speed, which keeps it in FR15
     * (0x8c08014e); without it, FR14, and 10 of 284 words differ */
    register Float speed;
    NJS_VECTOR back;
    struct dust_params *params = NULL;
    void (*create)(struct dust_params *params) = NULL;

    speed = njScalor2(&motion->speed);
    if (speed > 3.0F && record->anim.state == PLAYER_ANIM_DRIVEN) {
        if (record->ground_flags & (WORLD_GRIP_LEAST | WORLD_GRIP_LOW)) {
            if ((Float)rand() * RAND_SCALE > 0.95F) {
                ICECAP_BOARD_SPRAY((Sint8)PLAYER_VARS(work)->player);
            }
            sound_play_for(SE_BOARD_ICE, (int)work, 1, 0, 2);
        } else if (record->ground_flags & WORLD_SAND) {
            sound_play_for(SE_BOARD_SAND_RUN, (int)work, 1, 0, 2);
        }
    }
    if (record->ground_flags & WORLD_GRIP_LOW) {
        params = &board_snow;
        create = snow_create_from;
    } else if (record->ground_flags & WORLD_SAND) {
        params = &board_sand;
        create = sand_create_from;
    }
    if (params == NULL) {
        return;
    }
    drag = 0.2F;
    zero = 0.0F;
    scale = speed * 0.002F;
    if (speed > 3.0F && (Float)rand() * RAND_SCALE <= 0.1F) {
        back.x = -record->consts.height * 0.4F;
        back.y = zero;
        back.z = zero;
        player_turn_to_world(work, &back);
        params->pos.x = work->pos.x + back.x;
        params->pos.y = work->pos.y + back.y + 1.0F;
        params->pos.z = work->pos.z + back.z;
        params->scale = scale + ((Float)rand() * (0.01F * RAND_SCALE));
        params->speed.x = motion->speed.x * drag;
        params->speed.y = motion->speed.y * drag;
        params->speed.z = motion->speed.z * drag;
        create(params);
    }
    if (speed >= 4.0F || record->anim.state == PLAYER_ANIM_DRIVEN) {
        if ((Float)rand() * RAND_SCALE <= 0.25F) {
            back.x = zero;
            back.y = zero;
            back.z = -motion->drive;
            player_turn_to_world(work, &back);
            params->pos.x = work->pos.x + back.x + motion->speed.x;
            params->pos.y = work->pos.y + back.y + motion->speed.y + 1.0F;
            params->pos.z = work->pos.z + back.z + motion->speed.z;
            params->scale = scale + ((Float)rand() * (0.01F * RAND_SCALE));
            params->speed.x = back.x + (motion->speed.x * drag);
            params->speed.y = back.y + (motion->speed.y * drag);
            params->speed.z = back.z + (motion->speed.z * drag);
            create(params);
        }
    }
}

struct mode_sound {
    Sint32 se;
    Uint8 player;
    Sint16 mode;
};

#define MODE_SOUND(task) ((struct mode_sound *)(task)->extra)

/* while the player stays in the mode, the task plays the sound at the
 * player. once the player leaves the mode, or its slot empties, the task
 * ends */
/* @hack redundant: register on task, player and work, which keeps task in
 * R13 and the sound in R12 (0x8c0803de, 0x8c0803e4); without them, the two
 * trade and 8 of 46 words differ */
static void player_mode_sound_step(register struct task *task)
{
    struct mode_sound *sound = task->extra;
    register Uint32 player = sound->player;
    register struct task_work *work = player_work[player];

    if (work != NULL && work->mode == sound->mode) {
        sound_play_at_for(sound->se, (int)task, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 2);
    } else {
        task_kill(task);
    }
}

void player_sound_in_mode(int se, Uint8 player, Sint16 mode)
{
    struct task *task = task_create(TASK_EXTRA, 6, player_mode_sound_step);

    if (task != NULL) {
        MODE_SOUND(task)->se = se;
        MODE_SOUND(task)->player = player;
        MODE_SOUND(task)->mode = mode;
    }
}

static void player_fall_destroy(struct task *task)
{
}

struct fall_timer {
    Uint32 steps;
};

#define FALL_TIMER(task) ((struct fall_timer *)(task)->extra)

/* a fall's task ends the act once it has counted more than this many
 * steps: on its 122nd step */
#define FALL_STEPS 120

/* after FALL_STEPS, end the act in a miss. the death zone task that made
 * the fall, if one did, is marked hurt and stops checking. while the set
 * editor is open, the task ends without ending the act */
static void player_fall_step(struct task *task)
{
    if (FALL_TIMER(task)->steps++ > FALL_STEPS) {
        stage_end(STAGE_END_MISS);
        if (task->parent != NULL) {
            task->parent->work->flags |= PLAYER_HURT;
        }
        task_kill(task);
    } else if (edit_mode == 1) {
        task_kill(task);
    }
}

/* the voices the characters cry out with as they fall out of the stage */
#define VOICE_FALL_SONIC 0x587
#define VOICE_FALL_TAILS 0x55d
#define VOICE_FALL_KNUCKLES 0x54c
#define VOICE_FALL_AMY 0x50a
#define VOICE_FALL_BIG 0x51e
#define VOICE_FALL_E102 0x534
/* the voices the characters cry out with as their air runs out */
#define VOICE_DROWN_SONIC 0x58a
#define VOICE_DROWN_TAILS 0x560
#define VOICE_DROWN_KNUCKLES 0x548
#define VOICE_DROWN_AMY 0x50b
#define VOICE_DROWN_BIG 0x523

/* the drown task's steps when the countdown starts, the player drowns, and
 * the act ends */
#define DROWN_COUNT_STEPS 420
#define DROWN_AIR_OUT_STEPS 780
#define DROWN_END_STEPS 1020

void death_zone_step(struct task *task)
{
    struct task_work *work = player_work[0];
    struct death_zone *zone;
    struct death_zone **acts;
    Uint16 stage;
    Uint16 act;
    int character;
    Float drop;
    struct height_query query;

    task_step_children(task);
    if (work == NULL || task->child != NULL ||
        (task->work->flags & PLAYER_HURT)) {
        return;
    }
    stage = stage_get_stage_act();
    act = (Uint8)stage;
    acts = death_zones[(Uint16)STAGE_OF(stage)];
    if (!acts) {
        return;
    }
    zone = acts[act];
    if (zone == NULL || zone->object == NULL) {
        return;
    }
    character = 1 << get_character();
    query.pos = work->pos;
    while (zone->object != NULL) {
        if (debug_hidden >= 0) {
            njSimpleDrawObject(zone->object);
        }
        if ((zone->characters & character) &&
            polygon_find_model_heights(&query, zone->object)) {
            /* @hack spelling: an empty arm for below.found, whose deleted jump
             * loads above.found into R2 (0x8c080584); as one test, R3, and 2
             * of 110 words differ */
            if (query.below.found) {
            } else if (query.above.found) {
                drop = _builtin_fabsf(query.pos.y - query.above.level);
                /* @hack spelling: an empty arm for a drop over 30, whose
                 * deleted jump loads the next zone's object into R2
                 * (0x8c0805a4); as one test, R3, and 2 of 110 words differ */
                if (drop > 30.0F) {
                } else {
                    player_fall_in_zone(task, 0);
                }
            }
        }
        zone++;
    }
}

void player_down(int player)
{
    struct task_work *work = player_work[(Uint8)player];

    if (PLAYER_VARS(work)->player != 1) {
        if (edit_mode == 0) {
            work->flags |= PLAYER_REQUEST;
            work->submode = PLAYER_MODE_DOWN;
            player_records[(Uint8)player]->effects |= PLAYER_FALLING;
            task_create(TASK_EXTRA, 0, player_fall_step);
        }
    }
}

/* the camera looks at the player's eyes and the player cries out. for
 * Knuckles it also asks player 0, whoever falls, for PLAYER_MODE_RESET */
static void player_fall_cry(Uint8 player)
{
    struct task_work *work = player_work[player];

    camera_override(camera_step_eyes, 0, 0);
    switch (PLAYER_VARS(work)->character) {
    case CHARACTER_SONIC:
        sound_play(VOICE_FALL_SONIC, 0, 0, 0);
        break;
    case CHARACTER_TAILS:
        sound_play(VOICE_FALL_TAILS, 0, 0, 0);
        break;
    case CHARACTER_KNUCKLES:
        sound_play(VOICE_FALL_KNUCKLES, 0, 0, 0);
        player_set_mode(0, PLAYER_MODE_RESET);
        break;
    case CHARACTER_AMY:
        sound_play(VOICE_FALL_AMY, 0, 0, 0);
        break;
    case CHARACTER_BIG:
        sound_play(VOICE_FALL_BIG, 0, 0, 0);
        break;
    case CHARACTER_E102:
        sound_play(VOICE_FALL_E102, 0, 0, 0);
        break;
    }
}

void player_fall(Uint8 player)
{
    struct task *task;

    task = task_create(TASK_EXTRA, 0, player_fall_step);
    task->destructor = player_fall_destroy;
    player_fall_cry(player);
}

static void player_fall_in_zone(struct task *parent, Uint8 player)
{
    struct task *task;

    task = task_create_child(TASK_EXTRA, player_fall_step, parent);
    task->destructor = player_fall_destroy;
    player_fall_cry(player);
}

/* the seconds left, over the player as its air runs out */
static void player_drown_draw(struct task *task)
{
    struct drown *drown = DROWN(task);
    int steps = drown->steps;
    int player = drown->player;
    Float part;
    struct task_work *work;
    int seconds;
    NJS_POINT3 pos;
    NJS_POINT2 screen;

    /* @hack spelling: work set by a comma after part, which loads player into
     * R6 and the 60 into R7 (0x8c0807c8, 0x8c0807ce), and work into R5 from
     * a copy of player (0x8c0807e2 to 0x8c0807ec); as a statement of its
     * own, 81 of 98 words differ */
    steps = steps - DROWN_COUNT_STEPS;
    part = (Float)(steps % 60) * (1.0F / 60.0F), work = player_work[player];
    seconds = steps / 60;
    if (seconds >= 5 || seconds < 0) {
        return;
    }
    air_number.value = 5 - seconds;
    if (part >= 0.3F) {
        part = 0.3F;
    }
    air_number.scale = (part + 1.0F) * 2.0F;
    pos = work->pos;
    njProjectScreen(NULL, &pos, &screen);
    screen.y -= 96.0F;
    air_number.pos.x = screen.x;
    air_number.pos.y = screen.y;
    njFogDisable();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    hud_draw_number(&air_number);
    njFogEnable();
}

static void player_drown_destroy(struct task *task)
{
    stream_restart_music();
}

static void player_drown_step(struct task *task)
{
    struct drown *drown = DROWN(task);
    int player = drown->player;
    Uint32 steps = drown->steps++;
    int voice;

    if (steps < DROWN_AIR_OUT_STEPS &&
        player_records[player]->underwater_steps < 60) {
        task_kill(task);
        return;
    }
    if (steps == DROWN_AIR_OUT_STEPS) {
        if (player != 0) {
            player_work[player]->flags |= PLAYER_HURT;
        } else {
            player_set_mode(player, PLAYER_MODE_DROWN);
            switch (PLAYER_VARS(player_tasks[player]->work)->character) {
            case CHARACTER_SONIC:
                voice = VOICE_DROWN_SONIC;
                break;
            case CHARACTER_TAILS:
                voice = VOICE_DROWN_TAILS;
                break;
            case CHARACTER_KNUCKLES:
                voice = VOICE_DROWN_KNUCKLES;
                break;
            case CHARACTER_AMY:
                voice = VOICE_DROWN_AMY;
                break;
            case CHARACTER_BIG:
                voice = VOICE_DROWN_BIG;
                break;
            }
            /* @bug uninitialized: no case sets voice for E-102, Eggman or
             * Tikal. R4 then still holds the work player_set_mode loaded
             * into it (0x8c081140), and sound_play takes that address as
             * the voice (0x8c0809b4) */
            sound_play(voice, 0, 0, 0);
        }
        stream_stop_music();
        return;
    } else if (steps == DROWN_END_STEPS) {
        stage_end(STAGE_END_MISS);
        task_kill(task);
        return;
    } else if (edit_mode == 1) {
        task_kill(task);
        player_records[player]->underwater_steps = 0;
        return;
    }
    if (steps <= 1) {
        task->destructor = player_drown_destroy;
        task->draw = player_drown_draw;
        stream_request_interlude(MUSIC_TIMER);
    }
    task->draw(task);
}

void player_run_path(Uint8 player, struct path *path)
{
    struct task_work *work = player_work[player];

    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_PATH;
    PLAYER_VARS(work)->arg.path = path;
    player_records[player]->speed.x += 5.0F;
}

void player_spin_path(Uint8 player, struct path *path)
{
    struct task_work *work = player_work[player];

    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_PATH_SPIN;
    PLAYER_VARS(work)->arg.path = path;
}

void player_join_path(Uint8 player, struct path *path)
{
    struct task_work *work = player_work[player];

    if (work == NULL) {
        return;
    }
    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_JOIN_PATH;
    PLAYER_VARS(work)->arg.path = path;
    path_find_nearest(path, &work->pos, NULL,
                      &player_records[player]->path_distance);
}

void player_hang_path(Uint8 player, struct path *path, int point)
{
    struct task_work *work = player_work[player];
    struct player *record = player_records[player];

    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_HANG;
    PLAYER_VARS(work)->arg.path = path;
    record->path_point = point;
    path_get_distance(path, point, &record->path_distance);
}

void player_ledge_path(Uint8 player, struct path *path, int point,
                       struct angles *ang)
{
    struct task_work *work = player_work[player];
    struct player *record = player_records[player];

    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_LEDGE;
    PLAYER_VARS(work)->arg.path = path;
    record->path_point = point;
    path_get_distance(path, point, &record->path_distance);
    player_set_angles(player, ang->x, ang->y, ang->z);
}

int player_on_path(struct path *path)
{
    int players = 0;
    int i;
    struct task_work *work;

    for (i = 0; i < 2; i++) {
        if ((work = player_work[i]) != NULL) {
            if ((work->flags & PLAYER_ON_PATH) != 0) {
                if (PLAYER_VARS(work)->arg.path == path) {
                    players += i + 1;
                }
            }
        }
    }
    return players;
}

void player_event_run(Uint8 player, Float x, Float z)
{
    struct event_work *event;

    if (player_work[player] == NULL) {
        return;
    }
    event = player_work[player]->event;
    event->action = EVENT_ACTION_RUN;
    event->move.pos.x = x;
    event->move.pos.z = z;
    event->speed = 0.0F;
    event->accel = 0.0F;
    player_work[player]->flags |= PLAYER_EVENT_DRIVEN;
}

void player_event_move(Uint8 player, Float x, Float z)
{
    struct event_work *event;

    if (player_work[player] == NULL) {
        return;
    }
    event = player_work[player]->event;
    event->action = EVENT_ACTION_MOVE;
    event->move.pos.x = x;
    event->move.pos.z = z;
    player_work[player]->flags |= PLAYER_EVENT_DRIVEN;
}

void player_event_push(Uint8 player, Float x, Float z)
{
    struct event_work *event;

    if (player_work[player] == NULL) {
        return;
    }
    event = player_work[player]->event;
    event->action = EVENT_ACTION_PUSH;
    event->move.pos.x = x;
    event->move.pos.z = z;
    player_work[player]->flags |= PLAYER_EVENT_DRIVEN;
}

void player_event_stop(Uint8 player)
{
    struct event_work *event;

    if (player_work[player] == NULL) {
        return;
    }
    event = player_work[player]->event;
    event->action = EVENT_ACTION_DONE;
    player_work[player]->flags |= PLAYER_EVENT_DRIVEN;
}

void player_set_heading(Sint8 player, Angle ang)
{
    player_set_angles(player, player_work[(Uint8)player]->ang.x, ang,
                      player_work[(Uint8)player]->ang.z);
    player_work[(Uint8)player]->event->action = 0;
}

void player_event_end(Uint8 player)
{
    player_work[player]->event->action = 0;
}

void player_set_look_task(Uint8 player, struct task *task, NJS_POINT3 *offset)
{
    struct event_work *event;

    if (player_work[player] == NULL) {
        return;
    }
    event = player_work[player]->event;
    event->look = EVENT_LOOK_TASK;
    event->look_task = task;
    if (offset != NULL) {
        event->look_pos.x = offset->x;
        event->look_pos.y = offset->y;
        event->look_pos.z = offset->z;
    }
}

void player_set_look_point(Uint8 player, NJS_POINT3 *pos)
{
    struct event_work *event;

    if (player_work[player] == NULL) {
        return;
    }
    event = player_work[player]->event;
    event->look = EVENT_LOOK_POINT;
    if (pos != NULL) {
        event->look_pos.x = pos->x;
        event->look_pos.y = pos->y;
        event->look_pos.z = pos->z;
    }
}

void player_set_look_angles(Uint8 player, struct angles *ang)
{
    struct event_work *event;

    if (player_work[player] == NULL) {
        return;
    }
    event = player_work[player]->event;
    event->look = EVENT_LOOK_ANGLES;
    if (ang != NULL) {
        event->look_ang.x = ang->x;
        event->look_ang.y = ang->y;
        event->look_ang.z = ang->z;
    }
}

void player_clear_look(Uint8 player)
{
    struct event_work *event;

    if (player_work[player] == NULL) {
        return;
    }
    event = player_work[player]->event;
    event->look = 0;
}

struct look_timer {
    Sint8 player;
    Uint32 steps;
    Uint32 length;
};

#define LOOK_TIMER(task) ((struct look_timer *)(task)->extra)

static void player_look_timer_destroy(struct task *task)
{
    player_clear_look(LOOK_TIMER(task)->player);
}

static void player_look_timer_step(struct task *task)
{
    struct look_timer *timer = LOOK_TIMER(task);

    if (player_work[(Uint8)timer->player] == NULL ||
        timer->steps++ > timer->length) {
        task_kill(task);
    }
}

void player_look_at_task(Uint8 player, struct task *task, int steps)
{
    struct task *timer;
    NJS_POINT3 offset;

    if (player_work[player] == NULL || task == NULL) {
        return;
    }
    if ((timer = task_create(TASK_EXTRA, 5, player_look_timer_step)) == NULL) {
        return;
    }
    offset.x = offset.y = offset.z = 0.0F;
    timer->destructor = player_look_timer_destroy;
    LOOK_TIMER(timer)->player = player;
    LOOK_TIMER(timer)->length = steps;
    player_set_look_task(player, task, &offset);
}

void player_look_at_point(Uint8 player, NJS_POINT3 *pos, Uint32 steps)
{
    struct task *timer;

    if (player_work[player] == NULL || pos == NULL) {
        return;
    }
    if ((timer = task_create(TASK_EXTRA, 5, player_look_timer_step)) == NULL) {
        return;
    }
    timer->destructor = player_look_timer_destroy;
    LOOK_TIMER(timer)->player = player;
    LOOK_TIMER(timer)->length = steps;
    player_set_look_point(player, pos);
}

void player_look_at_angles(Uint8 player, struct angles *ang, Uint32 steps)
{
    struct task *timer;

    if (player_work[player] == NULL || ang == NULL) {
        return;
    }
    if ((timer = task_create(TASK_EXTRA, 5, player_look_timer_step)) == NULL) {
        return;
    }
    timer->destructor = player_look_timer_destroy;
    LOOK_TIMER(timer)->player = player;
    LOOK_TIMER(timer)->length = steps;
    player_set_look_angles(player, ang);
}

void player_circle(Uint8 player, struct task *task)
{
    struct task_work *work = player_work[player];

    if (!(work->flags & PLAYER_HOLDING)) {
        work->flags |= PLAYER_REQUEST;
        work->submode = PLAYER_MODE_CIRCLE;
        player_records[player]->held = task;
    }
}

void player_dangle(Uint8 player, struct task *task)
{
    struct task_work *work = player_work[player];

    if (!(work->flags & PLAYER_HOLDING)) {
        work->flags |= PLAYER_REQUEST;
        work->submode = PLAYER_MODE_DANGLE;
        player_records[player]->held = task;
    }
}

void player_knock_back(Uint8 player, Float x, Float y, Float z)
{
    struct task_work *work = player_work[player];

    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_KNOCK_BACK;
    player_face_along(player, -x, 0.0F, -z);
    player_set_speed(player, x, y, z);
}

void player_knock_apart(Uint8 one, Uint8 other, Float speed)
{
    struct task_work *work_one = player_work[one];
    struct task_work *work_other = player_work[other];
    NJS_VECTOR away;

    away.x = work_one->pos.x - work_other->pos.x;
    away.y = work_one->pos.y - work_other->pos.y;
    away.z = work_one->pos.z - work_other->pos.z;
    njUnitVector(&away);
    away.x *= speed;
    away.y = 0.0F;
    away.z *= speed;
    player_knock_back(one, away.x, away.y, away.z);
    player_knock_back(other, -away.x, -away.y, -away.z);
}

void player_set_mode(Uint8 player, Uint8 mode)
{
    struct task_work *work = player_work[player];

    work->flags |= PLAYER_REQUEST;
    work->submode = mode;
}

void player_set_speed(Uint8 player, Float x, Float y, Float z)
{
    struct task_work *work = player_work[player];
    struct task_motion *motion = player_motion[player];
    struct player *record = player_records[player];
    NJS_VECTOR speed;

    motion->speed.x = x;
    motion->speed.y = y;
    motion->speed.z = z;
    speed.x = x;
    speed.y = y;
    speed.z = z;
    player_turn_back(work, &speed);
    if (speed.x <= SPEED_NONE && speed.x >= -SPEED_NONE) {
        speed.x = 0.0F;
    }
    if (speed.z <= SPEED_NONE && speed.z >= -SPEED_NONE) {
        speed.z = 0.0F;
    }
    record->speed = speed;
}

void player_bounce(Uint8 player, Float x, Float y, Float z)
{
    struct task_work *work = player_work[player];

    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_SPRING;
    player_set_speed(player, x, y, z);
}

void player_bounce_higher(Uint8 player, Uint8 bounce, Float x, Float y, Float z)
{
    struct task_work *work = player_work[player];

    work->flags |= PLAYER_REQUEST;
    if (bounce == 3) {
        work->submode = PLAYER_MODE_BOUNCE_3;
    } else if (bounce == 2) {
        work->submode = PLAYER_MODE_BOUNCE_2;
    } else {
        work->submode = PLAYER_MODE_BOUNCE_1;
    }
    player_set_speed(player, x, y, z);
}

void player_launch(Uint8 player, const Angle *ang, Float speed, Sint16 steps)
{
    struct task_work *work = player_work[player];
    struct task_motion *motion = player_motion[player];
    struct player *record = player_records[player];
    NJS_VECTOR up;

    record->launch_steps = steps;
    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_LAUNCH;
    up.x = 0.0F;
    up.y = speed;
    up.z = 0.0F;
    record->speed = up;
    work->ang.x = ang[0];
    work->ang.y = 0x4000 - ang[1];
    motion->aim.y = work->ang.y;
    work->ang.z = ang[2];
    player_turn_to_world(work, &up);
    motion->speed = up;
}

void player_fling(Uint8 player, Float speed, NJS_VECTOR *line)
{
    struct task_work *work = player_work[player];
    struct task_motion *motion = player_motion[player];
    struct player *record = player_records[player];
    NJS_VECTOR along;

    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_FLING;
    player_set_angles(player, 0,
                      (int)(atan2f(line->x, line->z) * ANGLE_PER_RADIAN), 0);
    along = *line;
    along.x *= speed;
    along.y *= speed;
    along.z *= speed;
    motion->speed = along;
    player_turn_back(work, &along);
    record->speed = along;
}

void player_send_flying(Uint8 player, NJS_VECTOR *speed, const Angle *ang,
                        Sint16 steps)
{
    struct task_work *work = player_work[player];
    struct task_motion *motion = player_motion[player];
    struct player *record = player_records[player];
    NJS_VECTOR world;

    record->launch_steps = steps;
    record->speed = *speed;
    work->ang.x = ang[0];
    work->ang.y = 0x4000 - ang[1];
    motion->aim.y = work->ang.y;
    work->ang.z = ang[2];
    world = *speed;
    player_turn_to_world(work, &world);
    motion->speed = world;
}

void player_launch_speed(Uint8 player, NJS_VECTOR *speed, const Angle *ang,
                         Sint16 steps)
{
    struct task_work *work = player_work[player];
    struct task_motion *motion = player_motion[player];
    struct player *record = player_records[player];
    NJS_VECTOR world;

    record->launch_steps = steps;
    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_LAUNCH_SPEED;
    record->speed = *speed;
    work->ang.x = ang[0];
    work->ang.y = 0x4000 - ang[1];
    motion->aim.y = work->ang.y;
    work->ang.z = ang[2];
    world = *speed;
    player_turn_to_world(work, &world);
    motion->speed = world;
}

void player_set_angles(Uint8 player, Angle x, Angle y, Angle z)
{
    struct task_work *work = player_work[player];
    struct task_motion *motion = player_motion[player];

    work->ang.x = x;
    work->ang.y = 0x4000 - y;
    motion->aim.y = work->ang.y;
    work->ang.z = z;
}

void player_face_along(Uint8 player, Float x, Float y, Float z)
{
    struct task_work *work = player_work[player];
    struct task_motion *motion = player_motion[player];
    Float length = njSqrt((x * x) + (y * y) + (z * z));
    int ang_y = (int)(atan2f(z, x) * ANGLE_PER_RADIAN);
    Float sine = njSin(ang_y);
    int ang_x =
        (int)(acosf(-z / _builtin_fabsf(length * sine)) * ANGLE_PER_RADIAN);
    Float across = njSqrt((x * x) + (y * y));
    int ang_z =
        (int)(asinf(y / across) * ANGLE_PER_RADIAN) -
        (int)(asinf(_builtin_fabsf(length * sine) * njSin(ang_x) / across) *
              ANGLE_PER_RADIAN);

    work->ang.x = ang_x;
    work->ang.y = ang_y;
    motion->aim.y = work->ang.y;
    work->ang.z = ang_z;
}

void player_updraft(Sint8 player, Float x, Float y, Float z)
{
    struct task_work *work = player_work[(Uint8)player];

    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_UPDRAFT;
    player_set_accel(player, x, y, z);
}

void player_push(Sint8 player, Float x, Float y, Float z)
{
    struct player *record = player_records[(Uint8)player];
    Float gravity = record->consts.gravity;

    x /= gravity;
    y /= gravity;
    z /= gravity;
    player_set_accel(player, x, y, z);
}

void player_set_accel(Sint8 player, Float x, Float y, Float z)
{
    struct task_motion *motion = player_motion[(Uint8)player];

    motion->accel.x = x;
    motion->accel.y = y;
    motion->accel.z = z;
}

void player_set_position(Uint8 player, Float x, Float y, Float z)
{
    Uint8 index = player_history_index[player];

    if (player_work[player] == NULL) {
        return;
    }
    player_work[player]->pos.x = x;
    player_work[player]->pos.y = y;
    player_work[player]->pos.z = z;
    player_history_pos[index][player].x = x;
    player_history_pos[index][player].y = y;
    player_history_pos[index][player].z = z;
}

void player_squash(Uint8 player, Uint8 bounce)
{
    struct task_work *work = player_work[player];

    work->flags |= PLAYER_REQUEST;
    if (bounce == 3) {
        work->submode = PLAYER_MODE_SQUASH_3;
    } else if (bounce == 2) {
        work->submode = PLAYER_MODE_SQUASH_2;
    } else {
        work->submode = PLAYER_MODE_STILL;
    }
}

void player_ride_off(Uint8 player, Float x, Float y, Float z)
{
    struct task_work *work = player_work[player];

    work->flags |= PLAYER_REQUEST;
    work->submode = PLAYER_MODE_RIDE_OFF;
    player_set_speed(player, x, y, z);
}

void player_push_out_of(struct task_work *work)
{
    /* @hack spelling: player_work read through works, a local, which loads its
     * address into R4 and player 0 through it (0x8c0817ee, 0x8c0817f4);
     * through player_work, R14 takes both and 7 of 92 words differ */
    struct task_work **works = player_work;
    struct task_work *player = works[0];
    struct collision_shape *shape = player->collision->shapes;
    NJS_POINT3 away;
    Float gap;

    away.x = shape->center.x - work->pos.x;
    away.y = shape->center.y - work->pos.y;
    away.z = shape->center.z - work->pos.z;
    gap = njScalor(&away);
    gap -= shape->radius;
    gap -= work->collision->shapes->radius;
    if (gap < 0.0F) {
        if (njScalor(&away) == 0.0F) {
            away.x = 1.0F;
            away.y = away.z = 0.0F;
        } else {
            njUnitVector(&away);
        }
        gap = -0.2F - gap;
        player->pos.x += gap * away.x;
        player->pos.y += gap * away.y;
        player->pos.z += gap * away.z;
    }
}

int player_in_range(const NJS_POINT3 *pos, Float radius)
{
    int i;
    const unsigned char *point;
    Float x;
    Float y;
    Float z;

    /* @hack spelling: pos read through a byte cursor, which the image steps
     * with FMOV.S @R4+ (0x8c0818b6); through the members, 55 of 67 words
     * differ */
    point = (const unsigned char *)pos;
    x = *(const Float *)point;
    point += sizeof(Float);
    y = *(const Float *)point;
    point += sizeof(Float);
    z = *(const Float *)point;
    for (i = 0; i < 2; i++) {
        struct task_work *work;
        struct collision_work *collision;
        const unsigned char *center;
        NJS_POINT3 to;
        Float gap;

        if ((work = player_work[i]) == NULL) {
            continue;
        }
        collision = work->collision;
        if (collision == NULL) {
            continue;
        }
        /* @hack spelling: the center read through a byte cursor too (FMOV.S
         * @R4+,FR3 at 0x8c0818d2); through the members, 42 of 67 words
         * differ */
        center = (const unsigned char *)&collision->shapes->center;
        to.x = *(const Float *)center - x;
        center += sizeof(Float);
        to.y = *(const Float *)center - y;
        center += sizeof(Float);
        to.z = *(const Float *)center - z;
        gap = njScalor(&to);
        gap -= radius;
        if (gap < 0.0F) {
            return i + 1;
        }
    }
    return 0;
}

int player_in_cylinder(NJS_POINT3 *pos, Float radius, Float height)
{
    int i;
    NJS_POINT3 to;

    for (i = 0; i < 1; i++) {
        struct task_work *work = player_work[i];
        Float off;

        to.x = work->pos.x - pos->x;
        to.y = work->pos.y - pos->y;
        to.z = work->pos.z - pos->z;
        off = (to.x * to.x) + (to.z * to.z) - (radius * radius);
        if (off > 0.0F) {
            continue;
        }
        off = to.y;
        if (off < 0.0F) {
            off = -off;
        }
        if (off > height) {
            continue;
        }
        return i + 1;
    }
    return 0;
}

int player_get_character(Uint8 player)
{
    struct task_work *work = player_work[player];

    if (work == NULL) {
        return get_character();
    }
    return PLAYER_VARS(work)->character;
}

Bool player_ground_speed(Uint8 player, Float *speed)
{
    struct task_work *work = player_work[player];
    struct player *record = player_records[player];

    if (work != NULL && (work->flags & PLAYER_ON_GROUND)) {
        *speed = record->speed.x;
    } else {
        return FALSE;
    }
    return TRUE;
}

Bool player_get_history(Uint8 player, int steps, NJS_POINT3 *pos,
                        struct angles *ang)
{
    Uint8 index = player_history_index[player] - steps;

    if (player_work[player] == NULL) {
        return FALSE;
    }
    if (pos != NULL) {
        *pos = player_history_pos[index][player];
    }
    if (ang != NULL) {
        Angle *then = player_history_ang[index][player];

        ang->x = then[0];
        ang->y = then[1];
        ang->z = then[2];
    }
    return TRUE;
}

Bool player_set_history(Uint8 player, int steps, NJS_POINT3 *pos,
                        struct angles *ang)
{
    Uint8 index = player_history_index[player] - steps;

    if (player_work[player] == NULL) {
        return FALSE;
    }
    if (pos != NULL) {
        player_history_pos[index][player] = *pos;
    }
    if (ang != NULL) {
        player_history_ang[index][player][0] = ang->x;
        player_history_ang[index][player][1] = ang->y;
        player_history_ang[index][player][2] = ang->z;
    }
    return TRUE;
}

struct task *player_get_task(Uint8 player)
{
    return player_tasks[player];
}

int player_task_character(struct task *task)
{
    int i;
    int character = -1;

    if (task == NULL) {
        return -1;
    }
    for (i = 0; i < PLAYER_COUNT; i++) {
        if (player_tasks[i] == task) {
            character = PLAYER_VARS(task->work)->character;
            break;
        }
    }
    return character;
}

struct player *player_get(Uint8 player)
{
    return player_records[player];
}

int player_count(void)
{
    int i;
    int count = 0;

    for (i = 0; i < PLAYER_COUNT; i++) {
        if (player_tasks[i] != NULL) {
            count++;
        }
    }
    return count;
}

int player_nearest(NJS_POINT3 *pos)
{
    int i;
    int nearest = 0;
    Float best = 10000000.0F;
    NJS_POINT3 to;

    for (i = 0; i < PLAYER_COUNT; ++i) {
        if (player_work[i] != NULL) {
            Float off;

            to = *pos;
            njSubVector(&to, &player_work[i]->pos);
            off = njScalor(&to);
            if (off < best) {
                best = off;
                nearest = i;
            }
        }
    }
    return nearest;
}

int player_find_rival(Uint8 player)
{
    int i;
    /* @hack spelling: player_work read through works, a local, which loads its
     * address into R5 after the player's EXTU.B (0x8c081cee, 0x8c081cf0);
     * through player_work, the two trade places and 2 of 33 words differ */
    struct task_work **works = player_work;
    int id = works[player]->id == RIVAL_ID ? PLAYER_ID : RIVAL_ID;

    for (i = 0; i < PLAYER_COUNT; ++i) {
        if (works[i] != NULL) {
            if (works[i]->id == id) {
                return i;
            }
        }
    }
    return -1;
}

Bool player_stands_on(Uint8 player, struct task *task)
{
    struct task_work *work = player_work[player];
    struct player *record = player_records[player];

    if (record != NULL && work != NULL && (work->flags & PLAYER_ON_LAND) &&
        record->ground_task == task) {
        return TRUE;
    }
    return FALSE;
}

void player_suspend(Uint8 player)
{
    struct task *task = player_tasks[player];

    if (task != NULL) {
        player_saved_steps[player] = task->step;
        player_saved_draws[player] = task->draw;
        task->step = NULL;
        task->draw = NULL;
    }
}

void player_resume(Uint8 player)
{
    struct task *task = player_tasks[player];

    if (task != NULL) {
        task->step = player_saved_steps[player];
        task->draw = player_saved_draws[player];
    }
}

#define SPEED_UP_STEPS 600

struct speed_up {
    Uint8 player;
    Uint16 steps;
    /* the record's constants as they were before the speed up doubled
     * them */
    Float ground_accel;
    Float top_speed;
    Float still_grip;
};

#define SPEED_UP(task) ((struct speed_up *)(task)->extra)

static void player_speed_up_step(struct task *task)
{
    struct speed_up *speed_up;
    struct player *record;

    /* @hack spelling: speed_up assigned inside the index, which keeps it in R6
     * (MOV.L @(44,R4),R6 at 0x8c081dd4); assigned ahead, R5, and 8 of 21
     * words differ */
    record = player_records[(speed_up = SPEED_UP(task))->player];
    if (record != NULL) {
        if (speed_up->steps++ < SPEED_UP_STEPS) {
            return;
        }
    }
    task_kill(task);
}

static void player_speed_up_destroy(struct task *task)
{
    struct player *record = player_records[SPEED_UP(task)->player];

    if (record != NULL) {
        record->consts.ground_accel = SPEED_UP(task)->ground_accel;
        record->consts.top_speed = SPEED_UP(task)->top_speed;
        record->consts.still_grip = SPEED_UP(task)->still_grip;
    }
}

static void player_speed_up_start(struct task *task)
{
    struct player *record = player_records[SPEED_UP(task)->player];

    if (record == NULL) {
        task_kill(task);
        return;
    }
    SPEED_UP(task)->ground_accel = record->consts.ground_accel;
    SPEED_UP(task)->top_speed = record->consts.top_speed;
    SPEED_UP(task)->still_grip = record->consts.still_grip;
    record->consts.ground_accel = record->consts.ground_accel * 2.0F;
    record->consts.top_speed = record->consts.top_speed * 2.0F;
    record->consts.still_grip *= 2.0F;
    if (stage_get_stage_act() != STAGE_ACT(STAGE_HIGHWAY, 1)) {
        stream_request_interlude(MUSIC_SPEEDUP);
    }
    task->step = player_speed_up_step;
    task->destructor = player_speed_up_destroy;
}

void player_give_speed_up(Uint8 player)
{
    struct task *task;

    /* @bug memory: for a player past the last slot, the fallback to player 0
     * reads player_work past its end (MOV.L @(R0,R2),R2 at 0x8c081f14). it
     * tests player > PLAYER_COUNT - 1 (CMP/GT at 0x8c081f0a), so it takes only
     * such a player. a player 1 to 7 keeps its number even when its slot is
     * empty, and a speed up for it goes to nobody */
    /* @hack redundant: player >= 1 is the image's own test (CMP/GE at
     * 0x8c081f04), which the test after it makes needless; without it,
     * 27 of 30 words differ */
    /* NOLINTNEXTLINE(misc-redundant-expression) */
    if (player >= 1 && player > PLAYER_COUNT - 1 &&
        player_work[player] == NULL) {
        player = 0;
    }
    task = task_create(TASK_EXTRA, 2, player_speed_up_start);
    if (task != NULL) {
        SPEED_UP(task)->player = player;
    }
}

void player_give_magnetic_barrier(Uint8 player)
{
    struct player *record;
    struct task *task;

    /* @bug memory: for a player past the last slot, the fallback to player 0
     * reads player_work past its end (MOV.L @(R0,R2),R2 at 0x8c081f9c). it
     * tests player > PLAYER_COUNT - 1 (CMP/GT at 0x8c081f92), so it takes only
     * such a player. a player 1 to 7 keeps its number even when its slot is
     * empty, and a magnetic barrier for it goes to nobody */
    /* @hack redundant: player >= 1 is the image's own test (CMP/GE at
     * 0x8c081f8c), which the test after it makes needless; without it,
     * 43 of 49 words differ */
    /* NOLINTNEXTLINE(misc-redundant-expression) */
    if (player >= 1 && player > PLAYER_COUNT - 1 &&
        player_work[player] == NULL) {
        player = 0;
    }
    record = player_records[player];
    if (record != NULL) {
        if ((record->effects & PLAYER_MAGNETIC_BARRIER) == 0) {
            task = task_create(TASK_WORK, 2, magnetic_barrier_start);
            if (task != NULL) {
                PLAYER_VARS(task->work)->player = player;
                record->effects &= ~(PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER);
                record->effects |= PLAYER_MAGNETIC_BARRIER;
            }
        }
    }
}

void player_give_barrier(Uint8 player)
{
    struct player *record;
    struct task *task;

    /* @bug memory: for a player past the last slot, the fallback to player 0
     * reads player_work past its end (MOV.L @(R0,R1),R3 at 0x8c081ffe). it
     * tests player > PLAYER_COUNT - 1 (CMP/GT at 0x8c081ff4), so it takes only
     * such a player. a player 1 to 7 keeps its number even when its slot is
     * empty, and a barrier for it goes to nobody */
    /* @hack redundant: player >= 1 is the image's own test (CMP/GE at
     * 0x8c081fee), which the test after it makes needless; without it,
     * 43 of 49 words differ */
    /* NOLINTNEXTLINE(misc-redundant-expression) */
    if (player >= 1 && player > PLAYER_COUNT - 1 &&
        player_work[player] == NULL) {
        player = 0;
    }
    record = player_records[player];
    if (record != NULL) {
        if ((record->effects & PLAYER_BARRIER) == 0) {
            task = task_create(TASK_WORK, 2, barrier_start);
            if (task != NULL) {
                PLAYER_VARS(task->work)->player = player;
                record->effects &= ~(PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER);
                record->effects |= PLAYER_BARRIER;
            }
        }
    }
}

void player_give_invincibility(Uint8 player)
{
    struct player *record;
    struct task *task;

    /* @bug memory: for a player past the last slot, the fallback to player 0
     * reads player_work past its end (MOV.L @(R0,R2),R2 at 0x8c082060). it
     * tests player > PLAYER_COUNT - 1 (CMP/GT at 0x8c082056), so it takes only
     * such a player. a player 1 to 7 keeps its number even when its slot is
     * empty, and invincibility for it goes to nobody */
    /* @hack redundant: player >= 1 is the image's own test (CMP/GE at
     * 0x8c082050), which the test after it makes needless; without it,
     * 49 of 53 words differ */
    /* NOLINTNEXTLINE(misc-redundant-expression) */
    if (player >= 1 && player > PLAYER_COUNT - 1 &&
        player_work[player] == NULL) {
        player = 0;
    }
    record = player_records[player];
    if (record != NULL && !(record->effects & PLAYER_INVINCIBLE)) {
        task = task_create(TASK_WORK, 2, invincibility_start);
        if (task != NULL) {
            PLAYER_VARS(task->work)->player = player;
            record->effects |= PLAYER_INVINCIBLE;
            stream_request_interlude(MUSIC_INVNCIBL);
        }
    }
}
