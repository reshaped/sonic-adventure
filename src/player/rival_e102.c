/* the rival E-102: E-102 as player 1, on the pad its task writes */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/battle.h"
#include "collision/collision.h"
#include "enemy/enemy.h"
#include "event/play.h"
#include "object/common.h"
#include "player/e102.h"
#include "player/e102_effects.h"
#include "player/e102_mode.h"
#include "player/e102_shot.h"
#include "player/lockon.h"
#include "player/npc.h"
#include "player/player.h"
#include "player/rival.h"
#include "player/rival_e102.h"
#include "sound/stream.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the rival E-102's modes. the stick is released unless a mode moves it */
enum rival_mode {
    RIVAL_START,
    RIVAL_MUSIC,
    RIVAL_WAIT, /* until timer passes wait */
    RIVAL_READY, /* setting player 1's mode to PLAYER_MODE_READY, then facing */
    /* facing player 0, until timer passes face_steps with the rival on the
     * ground. while the rival stands, the stick turns it toward player 0 at
     * RIVAL_NUDGE. here the battle is lost once rival_player_fights is
     * FALSE. a rival that has taken RIVAL_HITS hits goes to RIVAL_BEATEN */
    RIVAL_FACE,
    RIVAL_RECOIL, /* recoiling, running from player 0 unless beaten */
    RIVAL_BEATEN,
    RIVAL_IDLE,
    RIVAL_AIM_AT, /* running at player 0, to aim within range[0] */
    /* aiming, the stick toward player 0 at a stroke of 0.1, until timer
     * passes charge. it puts a new aim ahead of player 0 every AIM_STEPS
     * steps */
    RIVAL_AIM,
    RIVAL_LEAP, /* pressing jump */
    RIVAL_AIM_AGAIN, /* in the air at player 0, aiming again at a new aim */
    RIVAL_RISE, /* in the air at player 0, jump held, to aim as it falls */
    /* in the air at player 0, jump and action held, until it locks on or
     * lands */
    RIVAL_AIM_AIR,
    RIVAL_SKATE, /* starting to skate */
    /* skating at player 0, to aim within SKATE_RANGE of it */
    RIVAL_SKATE_AT,
    /* aiming, until it locks on or timer passes LOCKON_STEPS */
    RIVAL_SKATE_AIM,
    RIVAL_VOLLEY_AT, /* running at player 0, to aim within range[0] */
    /* aiming until it locks on, then again from RIVAL_VOLLEY_AT, VOLLEYS
     * times in all, as blink counts them. once timer passes LOCKON_STEPS,
     * it goes back to RIVAL_VOLLEY_AT */
    RIVAL_VOLLEY,
    RIVAL_HOVER_AT, /* running at player 0, to hover within range[0] */
    /* hovering at player 0 and aiming, until it lands. it descends once it
     * holds HOVER_LOCKONS lock-ons, or once timer passes HOVER_STEPS. it
     * also descends once timer passes HOVER_AIM_STEPS while it isn't
     * aiming */
    RIVAL_HOVER,
    RIVAL_DESCEND /* jump held, until it lands */
};

/* where an aim goes: AIM_ABOVE above where LEAD puts player 0, or on
 * player 0, which it then follows */
enum aim_place { AIM_AHEAD, AIM_ON };
#define AIM_ABOVE 5.0F

enum aim_mode {
    AIM_START,
    AIM_WAIT, /* waiting for a lock-on, while the rival attacks */
    AIM_LOCKED /* locked on, until the shot hits it or the lock-on goes */
};

/* what the rival E-102 says as it recovers from a hit with one hit left */
#define VOICE_LAST 0xab

struct aim_vars {
    Uint8 unk_00;
    Uint8 place; /* an aim_place */
    Uint8 unk_02[6];
    struct task *rival; /* the task that drives the rival */
};

#define AIM(work) ((struct aim_vars *)&(work)->vars)

/* the steps from one aim RIVAL_AIM or RIVAL_HOVER puts out to the next, a
 * power of 2 */
#define AIM_STEPS 16

/* how near player 0 an aim must be for the shot that hits it to reach
 * player 0. it is squared, as point_distance_squared measures */
#define SHOT_RANGE (10.0F * 10.0F)

#define SKATE_RANGE 100.0F
/* the steps RIVAL_SKATE_AIM and RIVAL_VOLLEY wait for a lock-on */
#define LOCKON_STEPS 300
#define VOLLEYS 4
#define HOVER_LOCKONS 3
#define HOVER_STEPS 600
#define HOVER_AIM_STEPS 60

#define LEAD_STEPS 20.0F /* how far ahead LEAD looks, in steps */
/* where player 0 will be LEAD_STEPS steps on, if it keeps its motion's way
 * and its forward speed */
#define LEAD(out, rival)                                                       \
    do {                                                                       \
        vector_unit(&(rival)->player_motion->speed, (out));                    \
        (out)->x = (out)->x * (rival)->player_record->speed.x * LEAD_STEPS;    \
        (out)->y = (out)->y * (rival)->player_record->speed.x * LEAD_STEPS;    \
        (out)->z = (out)->z * (rival)->player_record->speed.x * LEAD_STEPS;    \
        VECTOR_SUM((out), &(rival)->player_work->pos, (out));                  \
    } while (0)

static void rival_e102_step(struct task *task);
static struct task *rival_e102_aim(struct task *task, enum aim_place place,
                                   NJS_POINT3 *pos);
static void rival_e102_aim_step(struct task *task);
static void rival_e102_aim_destroy(struct task *task);

/* the attack the rival makes, by the hits it has taken. three hits end
 * the battle, so it never makes the last two */
static Sint8 rival_e102_attacks[5] = {RIVAL_AIM_AT, RIVAL_LEAP, RIVAL_SKATE,
                                      RIVAL_VOLLEY_AT, RIVAL_HOVER_AT};

/* the shape of an aim */
/* clang-format off */
static struct collision_shape rival_e102_aim_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE, 0, PUSH_FIRM(PUSH_PLAYERS),
    {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F, {0}, 0, NULL};
/* clang-format on */

struct task *rival_e102_start(struct battle *battle)
{
    struct task *rival_task;
    struct task *task;
    struct rival *rival;

    e102_time_runs = FALSE;
    rival_task = battle->rival;
    task = task_create(TASK_WORK, 0, rival_e102_step);
    if (task == NULL) {
        return NULL;
    }
    rival = heap_calloc(1, sizeof(struct rival));
    task->extra = rival;
    rival->rival_task = rival_task;
    rival->rival_work = rival_task->work;
    rival->rival_motion = rival_task->motion;
    rival->rival_record = rival->rival_motion->record;
    rival->player_task = battle->player;
    rival->player_work = rival->player_task->work;
    rival->player_motion = rival->player_task->motion;
    rival->player_record = rival->player_motion->record;
    rival->kind = battle->kind;
    rival->unk_10 = battle->unk_0c;
    rival->finish = battle->finish;
    rival->attacks = rival_e102_attacks;
    rival->range[0] = 100.0F;
    rival->range[1] = 100.0F;
    rival->wait = 0;
    rival->face_steps = 60;
    rival->charge = 60;
    rival_place(task);
    task->draw = rival_draw;
    task->destructor = rival_end;
    battle_set_outcome(BATTLE_RUNNING);
    camera_start_battle(task);
    return task;
}

/* write the rival E-102's pad and stick for the step, by its mode, and
 * keep E-102's countdown stopped */
static void rival_e102_step(struct task *task)
{
    struct rival *rival;
    struct e102 *e102;
    Uint32 on;
    Uint32 press;
    Angle ang;
    Float stroke;
    Float across;
    Float distance;
    NJS_POINT3 lead;
    int caught;
    int flags;

    rival = task->extra;
    e102_time_runs = FALSE;
    if (event_in_control() == TRUE) {
        return;
    }
    camera_keep_battle();
    rival->rival_record = rival->rival_motion->record;
    rival->player_record = rival->player_motion->record;
    if (rival->rival_record == NULL) {
        return;
    }
    if (rival->player_record == NULL) {
        return;
    }
    e102 = rival->rival_record->own.e102.record[0];
    if (PLAYER_VARS(rival->player_work)->character == CHARACTER_SONIC) {
        rival->rival_work->collision->flags |= COLLISION_TARGET;
    }
    pad_disable_player(1);
    ang = 0;
    stroke = 0.0F;
    on = pad_records[1].on;
    press = pad_records[1].press;
    rival->rival_record->upgrades |= PLAYER_E102_BOOSTER;
    caught = rival_catch_falls(rival);
    if (caught & RIVAL_FELL) {
        rival->mode = RIVAL_BEATEN;
        rival->rival_work->pos = rival->start;
    } else if (caught & RIVAL_PLAYER_FELL) {
        /* @hack redundant: an empty arm, whose test keeps the image's TST R0,R4
         * at 0x8c089b22. the switch's CMP/HS at 0x8c089b38 sets T again
         * before any branch. without it, rival_e102.c's code runs 32 bytes
         * short and 1007 of 1091 words differ */
    }
    switch (rival->mode) {
    case RIVAL_START:
        rival->rival_work->id = RIVAL_ID;
        rival->timer = 0;
        rival->phase = 0;
        rival->mode = RIVAL_MUSIC;
        stream_reset();
        break;
    case RIVAL_MUSIC:
        stream_request_music(MUSIC_BOSSEVNT);
        rival->mode = RIVAL_WAIT;
        break;
    case RIVAL_WAIT:
        rival->timer++;
        if (rival->timer > rival->wait) {
            rival->timer = 0;
            rival->mode = RIVAL_READY;
        }
        break;
    case RIVAL_READY:
        player_set_mode(1, PLAYER_MODE_READY);
        rival->mode = RIVAL_FACE;
        break;
    case RIVAL_FACE:
        if (!rival_player_fights(task)) {
            battle_set_outcome(BATTLE_LOST);
            task_kill(task);
            return;
        }
        if (rival->hits >= RIVAL_HITS) {
            rival->mode = RIVAL_BEATEN;
            break;
        }
        rival->flags &= ~RIVAL_ATTACKING;
        if (rival->rival_record->speed.x == 0.0F) {
            ang = npc_angle_to(&rival->rival_work->pos,
                               &rival->player_work->pos, NULL, NULL);
            stroke = RIVAL_NUDGE;
        }
        rival->timer++;
        if (rival->timer > rival->face_steps &&
            (rival->rival_work->flags & PLAYER_ON_GROUND)) {
            rival->timer = 0;
            rival->mode = rival->attacks[rival->hits];
            rival->flags |= RIVAL_ATTACKING;
        }
        break;
    case RIVAL_RECOIL:
        if (rival->hits < RIVAL_HITS) {
            ang = npc_angle_to(&rival->rival_work->pos,
                               &rival->player_work->pos, NULL, &distance);
            stroke = 1.0F;
            ang += NJM_DEG_ANG(180);
        }
        if (!(rival->rival_work->flags & PLAYER_RECOILING)) {
            rival->mode = RIVAL_READY;
            if (rival->hits == RIVAL_HITS - 1) {
                stream_request_voice(VOICE_LAST);
            }
        }
        break;
    case RIVAL_BEATEN:
        battle_set_outcome(BATTLE_WON);
        task_kill(task);
        return;
    case RIVAL_IDLE:
        break;
    case RIVAL_AIM_AT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        if (rival->rival_record->speed.x > 1.0F) {
            rival->rival_record->speed.x = 1.0F;
        }
        if (across < rival->range[0]) {
            rival->heading = ang;
            npc_stick_against(&rival->rival_motion->speed, &stroke, &ang);
            press |= pad_action_buttons;
            rival->mode = RIVAL_AIM;
            rival->flags |= RIVAL_ATTACKING;
        }
        break;
    case RIVAL_AIM:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 0.1F;
        on |= pad_action_buttons;
        rival->timer++;
        if (!(rival->timer & (AIM_STEPS - 1))) {
            LEAD(&lead, rival);
            rival_e102_aim(task, AIM_AHEAD, &lead);
        }
        if (rival->timer > rival->charge) {
            rival->timer = 0;
            rival->mode = RIVAL_READY;
            rival->flags &= ~RIVAL_ATTACKING;
        }
        break;
    case RIVAL_LEAP:
        press |= pad_jump_buttons;
        rival->mode = RIVAL_RISE;
        break;
    case RIVAL_RISE:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, NULL);
        stroke = 1.0F;
        on |= pad_jump_buttons;
        if (rival->rival_work->flags & PLAYER_ON_GROUND) {
            rival->mode = RIVAL_READY;
        }
        if (rival->rival_motion->speed.y < 0.0F) {
            press |= pad_action_buttons;
            rival->flags |= RIVAL_ATTACKING;
            rival->mode = RIVAL_AIM_AIR;
            rival_e102_aim(task, AIM_AHEAD, &rival->player_work->pos);
        }
        break;
    case RIVAL_AIM_AIR:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, NULL);
        stroke = 1.0F;
        on |= pad_jump_buttons | pad_action_buttons;
        if (e102->lockon_count > 0) {
            on &= ~pad_action_buttons;
            rival->flags &= ~RIVAL_ATTACKING;
            rival->mode = RIVAL_AIM_AGAIN;
        }
        if (rival->rival_work->flags & PLAYER_ON_GROUND) {
            rival->timer = 0;
            on &= ~pad_action_buttons;
            rival->flags &= ~RIVAL_ATTACKING;
            rival->mode = RIVAL_READY;
        }
        break;
    case RIVAL_AIM_AGAIN:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, NULL);
        stroke = 1.0F;
        on |= pad_jump_buttons;
        /* @hack redundant: stroke stored over itself, which makes no code
         * but has the action mask take R3 and press R1 (0x8c08a020,
         * 0x8c08a01c); without it, they take R1 and R0 and 9 of 1091 words
         * differ */
        rival->stroke = rival->stroke;
        press |= pad_action_buttons;
        rival->mode = RIVAL_AIM_AIR;
        rival->flags |= RIVAL_ATTACKING;
        rival_e102_aim(task, AIM_AHEAD, &rival->player_work->pos);
        break;
    case RIVAL_SKATE:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 0.1F;
        e102_skate_start(rival->rival_task);
        rival->mode = RIVAL_SKATE_AT;
        rival->timer = 0;
        break;
    case RIVAL_SKATE_AT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           &distance, &across);
        stroke = 1.0F;
        if (distance < SKATE_RANGE) {
            press |= pad_action_buttons;
            rival->flags |= RIVAL_ATTACKING;
            rival_e102_aim(task, AIM_AHEAD, NULL);
            rival->mode = RIVAL_SKATE_AIM;
        }
        break;
    case RIVAL_SKATE_AIM:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        if (rival->rival_record->speed.x > 2.0F) {
            rival->rival_record->speed.x = 2.0F;
        }
        on |= pad_action_buttons;
        rival->timer++;
        if (e102->lockon_count > 0 || rival->timer > LOCKON_STEPS) {
            on &= ~pad_action_buttons;
            rival->flags &= ~RIVAL_ATTACKING;
            rival->mode = RIVAL_READY;
            rival->timer = 0;
        }
        break;
    case RIVAL_VOLLEY_AT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        if (rival->rival_record->speed.x > 1.0F) {
            rival->rival_record->speed.x = 1.0F;
        }
        if (across < rival->range[0]) {
            rival->heading = ang;
            npc_stick_against(&rival->rival_motion->speed, &stroke, &ang);
            press |= pad_action_buttons;
            rival->mode = RIVAL_VOLLEY;
            rival->flags |= RIVAL_ATTACKING;
            LEAD(&lead, rival);
            rival_e102_aim(task, AIM_AHEAD, &lead);
        }
        break;
    case RIVAL_VOLLEY:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 0.1F;
        on |= pad_action_buttons;
        rival->timer++;
        if (e102->lockon_count > 0) {
            on &= ~pad_action_buttons;
            rival->blink++;
            if (rival->blink > VOLLEYS - 1) {
                rival->mode = RIVAL_READY;
            } else {
                rival->mode = RIVAL_VOLLEY_AT;
            }
            rival->flags &= ~RIVAL_ATTACKING;
        }
        if (rival->timer > LOCKON_STEPS) {
            rival->mode = RIVAL_VOLLEY_AT;
            rival->flags &= ~RIVAL_ATTACKING;
        }
        break;
    case RIVAL_HOVER_AT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        if (across < rival->range[0]) {
            rival->heading = ang;
            press |= pad_jump_buttons | pad_action_buttons;
            rival->mode = RIVAL_HOVER;
            rival->flags |= RIVAL_ATTACKING;
        }
        break;
    case RIVAL_HOVER:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, NULL);
        stroke = 1.0F;
        on |= pad_jump_buttons | pad_action_buttons;
        rival->timer++;
        /* @bug uninitialized: no path through RIVAL_HOVER writes across, so
         * the test reads whatever its frame slot holds (FMOV.S
         * @(R0,R15),FR2 with R0 8 at 0x8c08a3d4), against range[0] at
         * 0x8c08a3d8. no battle runs it: only attacks[4] leads to
         * RIVAL_HOVER_AT, and three hits end the battle first */
        /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
        if (!(rival->timer & (AIM_STEPS - 1)) && across < rival->range[0]) {
            rival_e102_aim(task, AIM_AHEAD, &rival->player_work->pos);
        }
        if (rival->rival_work->flags & PLAYER_ON_GROUND) {
            rival->timer = 0;
            rival->mode = RIVAL_READY;
            on &= ~pad_action_buttons;
            rival->flags &= ~RIVAL_ATTACKING;
        } else if (e102->lockon_count >= HOVER_LOCKONS ||
                   rival->timer > HOVER_STEPS ||
                   (rival->timer > HOVER_AIM_STEPS &&
                    !e102_is_aiming(rival->rival_task))) {
            rival->timer = 0;
            rival->mode = RIVAL_DESCEND;
            on &= ~pad_action_buttons;
            rival->flags &= ~RIVAL_ATTACKING;
        }
        break;
    case RIVAL_DESCEND:
        /* @hack redundant: stroke stored over itself, which makes no code
         * but has this case load pad_jump_buttons itself (MOV.L at
         * 0x8c08a480); without it, SHC loads its value once ahead of the
         * switch, rival_e102.c's code runs 64 bytes short and 978 of 1091
         * words differ */
        rival->stroke = rival->stroke;
        on |= pad_jump_buttons;
        if (rival->rival_work->flags & PLAYER_ON_GROUND) {
            rival->mode = RIVAL_READY;
        }
        break;
    }
    flags = rival->rival_work->flags;
    if (!(flags & PLAYER_RECOILING) && (flags & PLAYER_HURT)) {
        rival->hits++;
        rival->timer = 0;
        rival->phase = 0;
        rival->mode = RIVAL_RECOIL;
        rival->flags &= ~RIVAL_ATTACKING;
    }
    pad_records[1].on = on;
    pad_records[1].press = press;
    pad_stick_b.ang = ang;
    pad_stick_b.stroke = stroke;
    rival->ang = ang;
    rival->heading = ang;
    rival->stroke = stroke;
    rival->on = on;
    rival->press = press;
    rival_draw(task);
    task_step_children(task);
}

/* make an aim for the rival to lock on to, as a child of task. place says
 * where it goes. it returns the aim, or NULL when it can't be made. pos
 * goes unread: AIM_AHEAD takes LEAD again */
static struct task *rival_e102_aim(struct task *task, enum aim_place place,
                                   NJS_POINT3 *pos)
{
    struct rival *rival = task->extra;
    struct task *child;
    struct task_work *work;

    child = task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE,
                              rival_e102_aim_step, task);
    if (child == NULL) {
        return NULL;
    }
    enemy_alloc_record(child, child->work);
    collision_init(child, &rival_e102_aim_shape, COLLISION_ITEM, 1, 1);
    work = child->work;
    AIM(work)->rival = task;
    AIM(work)->place = place;
    switch (place) {
    case AIM_AHEAD:
        LEAD(&work->pos, rival);
        work->pos.y += AIM_ABOVE;
        break;
    case AIM_ON:
        break;
    }
    return child;
}

/* the step of an aim. an aim the rival hasn't locked on to dies once
 * RIVAL_ATTACKING is clear. a locked aim dies when its lock-on goes, or
 * when the shot hits it. within SHOT_RANGE of player 0, that shot hurts
 * player 0. if player 0 guards, the shot bounces back at the rival
 * instead */
static void rival_e102_aim_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *target = LOCKON_TARGET(task);
    struct rival *rival = AIM(work)->rival->extra;
    struct task *player_task = rival->player_task;
    struct task_work *player_work = player_task->work;
    struct player *record = rival->player_record;
    struct task *shot;
    struct task *lockon;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a point. the image's ADD #-24,R15 reserves them below the
     * saved registers (0x8c08a650); without them, 2 of 230 words differ */
    NJS_POINT3 unused;
    NJS_VECTOR away;

    if (player_task == NULL) {
        return;
    }
    if (work->collision != NULL) {
        work->collision->flags &= ~COLLISION_TARGET;
    }
    switch (AIM(work)->place) {
    case AIM_AHEAD:
        break;
    case AIM_ON:
        work->pos = player_work->pos;
        if (record != NULL) {
            work->pos.y += 0.5F * record->consts.height;
        }
    }
    switch (work->mode) {
    case AIM_START:
        task->destructor = rival_e102_aim_destroy;
        work->collision->flags &= ~COLLISION_TARGET;
        work->mode = AIM_WAIT;
        /* fall through */
    case AIM_WAIT:
        if (target->lockon != NULL) {
            work->mode = AIM_LOCKED;
            break;
        }
        if (rival->flags & RIVAL_ATTACKING) {
            break;
        }
        task_kill(task);
        return;
    case AIM_LOCKED:
        if (target->lockon == NULL) {
            task_kill(task);
            return;
        }
        if (!(work->collision->flags & COLLISION_HIT)) {
            break;
        }
        if (work->collision->hit->kind != COLLISION_ATTACK) {
            break;
        }
        shot = work->collision->hit->owner;
        if ((lockon = lockon_of_shot(shot)) == NULL ||
            target->lockon != lockon) {
            break;
        }
        if (point_distance_squared(&work->pos, &player_work->pos) <
            SHOT_RANGE) {
            if (player_work->flags & PLAYER_GUARD) {
                VECTOR_DIFF(&away, &rival->rival_work->pos,
                            &rival->player_work->pos);
                vector_unit(&away, &away);
                VECTOR_SCALE(&away, &away, 5.0F);
                e102_release_shot(shot, &away);
                lockon_release_hit(task);
            } else {
                player_work->flags |= PLAYER_HURT;
                lockon_release(task);
            }
            task_kill(task);
            return;
        } else {
            lockon_release(task);
            task_kill(task);
            return;
        }
    }
    collision_register(work);
}

/* an aim's destructor. an aim holds nothing of its own to free */
static void rival_e102_aim_destroy(struct task *task)
{
}
