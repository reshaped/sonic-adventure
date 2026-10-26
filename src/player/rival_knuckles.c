/* the rival Knuckles: Knuckles as player 1, on the pad its task writes */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/battle.h"
#include "collision/collision.h"
#include "event/play.h"
#include "object/common.h"
#include "player/knuckles.h"
#include "player/npc.h"
#include "player/player.h"
#include "player/rival.h"
#include "player/rival_knuckles.h"
#include "sound/stream.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "vmu/vibration.h"

/* the rival Knuckles' modes. the stick is released unless a mode moves
 * it */
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
    RIVAL_PUNCH_AT, /* running at player 0, to punch within range[0] */
    RIVAL_PUNCH, /* punching at player 0, by phase */
    RIVAL_ROLL_AT, /* running at player 0, to press action within range[1] */
    /* rolling at player 0, the stick released while timer & blink is 0 and
     * pushed otherwise. it ends once timer passes chase, or once player 1's
     * flags lose PLAYER_ATTACKING */
    RIVAL_ROLL,
    /* turning toward player 0 at RIVAL_NUDGE, to jump from the ground */
    RIVAL_TAKE_OFF,
    /* in the air at player 0, jump held as it rises, then pressed again to
     * glide */
    RIVAL_JUMP,
    RIVAL_GLIDE, /* gliding at player 0, until it lands or climbs a wall */
    RIVAL_CLIMB /* the stick pushed up, until it has climbed over the top */
};

/* the phases of RIVAL_PUNCH. each of the first three presses action */
enum punch_phase {
    PUNCH_FIRST,
    /* once timer passes PUNCH_GAP, ten steps after the first */
    PUNCH_SECOND,
    PUNCH_THIRD, /* once timer passes PUNCH_GAP again */
    /* until the punching ends. the rival then halves its speed and steers
     * off */
    PUNCH_FINISH,
    /* the stick released, until its speed falls to 0.02 or timer passes
     * SLOW_STEPS */
    PUNCH_SLOW
};
#define PUNCH_GAP 8
#define SLOW_STEPS 120

/* the rival Knuckles' voices. as it recovers from a hit with one hit left,
 * it says the VOICE_LAST for the character it fights */
#define VOICE_LAST_SONIC 0xa8
#define VOICE_LAST_TAILS 0xaa
#define VOICE_HIT 0xa6 /* as it takes a hit */

/* the y1 RIVAL_CLIMB writes to player 1's pad: its stick pushed up */
#define STICK_UP (-127)

static void rival_knuckles_step(struct task *task);

/* the attack the rival Knuckles makes, by the hits it has taken. with one
 * hit taken, it takes turns at RIVAL_ROLL_AT and RIVAL_PUNCH_AT instead */
static Sint8 rival_knuckles_attacks[RIVAL_HITS] = {
    RIVAL_PUNCH_AT, RIVAL_ROLL_AT, RIVAL_TAKE_OFF};

struct task *rival_knuckles_start(struct battle *battle)
{
    struct task *rival_task = battle->rival;
    struct task *task;
    struct rival *rival;

    task = task_create(TASK_WORK, 0, rival_knuckles_step);
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
    rival->attacks = rival_knuckles_attacks;
    rival->range[0] = 50.0F;
    rival->range[1] = 50.0F;
    rival->range[2] = 50.0F;
    rival->wait = 120;
    rival->face_steps = 60;
    rival->chase = 120;
    rival->blink = 1;
    rival_place(task);
    task->draw = rival_draw;
    task->destructor = rival_end;
    battle_set_outcome(BATTLE_RUNNING);
    camera_start_battle(task);
    return task;
}

/* write the rival Knuckles' pad and stick for the step, by its mode */
static void rival_knuckles_step(struct task *task)
{
    struct rival *rival;
    Uint32 on;
    Uint32 press;
    Angle ang;
    Float stroke;
    Float across;
    int caught;
    int flags;

    rival = task->extra;
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
    if (PLAYER_VARS(rival->player_work)->character == CHARACTER_SONIC) {
        rival->rival_work->collision->flags |= COLLISION_TARGET;
    }
    pad_disable_player(1);
    ang = 0;
    stroke = 0.0F;
    on = pad_records[1].on;
    press = pad_records[1].press;
    caught = rival_catch_falls(rival);
    if (caught & RIVAL_FELL) {
        rival->mode = RIVAL_BEATEN;
        rival->rival_work->pos = rival->start;
    } else if (caught & RIVAL_PLAYER_FELL) {
        /* @hack redundant: an empty arm, whose test keeps the image's TST R0,R4
         * at 0x8c089222. the switch's CMP/HS at 0x8c089232 sets T again
         * before any branch. without it, rival_knuckles.c's code runs 32
         * bytes short and 711 of 774 words differ */
    }
    switch (rival->mode) {
    case RIVAL_START:
        rival->rival_work->id = RIVAL_ID;
        rival->timer = 0;
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
            if (rival->hits == 1) {
                if (rival->turn & 1) {
                    rival->mode = RIVAL_PUNCH_AT;
                } else {
                    rival->mode = RIVAL_ROLL_AT;
                }
                rival->turn++;
            } else {
                rival->mode = rival->attacks[rival->hits];
                rival->flags |= RIVAL_ATTACKING;
            }
        }
        break;
    case RIVAL_RECOIL:
        if (rival->hits < RIVAL_HITS) {
            ang = npc_angle_to(&rival->rival_work->pos,
                               &rival->player_work->pos, NULL, &across);
            stroke = 1.0F;
            ang += NJM_DEG_ANG(180);
        }
        if (!(rival->rival_work->flags & PLAYER_RECOILING)) {
            rival->mode = RIVAL_READY;
            if (rival->hits == RIVAL_HITS - 1) {
                if (rival->kind == BATTLE_KNUCKLES_SONIC) {
                    stream_request_voice(VOICE_LAST_SONIC);
                } else if (rival->kind == BATTLE_KNUCKLES_TAILS) {
                    stream_request_voice(VOICE_LAST_TAILS);
                }
            }
        }
        break;
    case RIVAL_BEATEN:
        battle_set_outcome(BATTLE_WON);
        task_kill(task);
        return;
    case RIVAL_IDLE:
        break;
    case RIVAL_PUNCH_AT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        if (across < rival->range[0]) {
            rival->phase = PUNCH_FIRST;
            rival->mode = RIVAL_PUNCH;
        }
        break;
    case RIVAL_PUNCH:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        if (rival->rival_record->speed.x > 2.0F) {
            rival->rival_record->speed.x = 1.9F;
        }
        switch (rival->phase) {
        case PUNCH_FIRST:
            if (rival->rival_record->speed.x < 0.5F) {
                rival->rival_record->speed.x = 0.5F;
            }
            press |= pad_action_buttons;
            rival->phase = PUNCH_SECOND;
            break;
        case PUNCH_SECOND:
            if (rival->timer++ > PUNCH_GAP) {
                press |= pad_action_buttons;
                rival->timer = 0;
                rival->phase = PUNCH_THIRD;
            }
            break;
        case PUNCH_THIRD:
            if (rival->timer++ > PUNCH_GAP) {
                /* @hack redundant: an empty test of player 0's
                 * PLAYER_ON_GROUND, which keeps the image's TST R12,R0 at
                 * 0x8c08955e; without it, 12 of 774 words differ */
                if (rival->player_work->flags & PLAYER_ON_GROUND) {
                }
                press |= pad_action_buttons;
                rival->timer = 0;
                rival->phase = PUNCH_FINISH;
            }
            break;
        case PUNCH_FINISH:
            stroke = 1.0F;
            if (knuckles_get_activity(1) != KNUCKLES_PUNCHING) {
                rival->timer = 0;
                rival->rival_record->speed.x *= 0.5F;
                ang = angle_arc_tan2(rival->rival_motion->speed.z,
                                     rival->rival_motion->speed.x) +
                      NJM_DEG_ANG(11.25);
                stroke = 1.0F;
                rival->phase = PUNCH_SLOW;
            }
            break;
        case PUNCH_SLOW:
            stroke = 0.0F;
            if (rival->rival_record->speed.x <= 0.02F ||
                rival->timer++ > SLOW_STEPS) {
                rival->timer = 0;
                rival->phase = PUNCH_FIRST;
                rival->mode = RIVAL_READY;
            }
            break;
        }
        break;
    case RIVAL_ROLL_AT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        if (across < rival->range[1]) {
            press = pad_action_buttons;
            if (rival->rival_record->speed.x <= 2.0F) {
                rival->rival_record->speed.x = 2.01F;
            }
            rival->phase = 0;
            rival->mode = RIVAL_ROLL;
        }
        break;
    case RIVAL_ROLL:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        if ((rival->timer & rival->blink) == 0) {
            stroke = 0.0F;
        } else {
            stroke = 1.0F;
        }
        rival->timer++;
        if (rival->timer > rival->chase) {
            press |= pad_action_buttons;
            npc_stick_against(&rival->rival_motion->speed, &stroke, &ang);
            stroke = 1.0F;
            rival->timer = 0;
            rival->mode = RIVAL_READY;
        } else if ((rival->rival_work->flags & PLAYER_ATTACKING) !=
                   PLAYER_ATTACKING) {
            rival->timer = 0;
            rival->mode = RIVAL_READY;
        }
        break;
    case RIVAL_TAKE_OFF:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, NULL);
        stroke = RIVAL_NUDGE;
        if (rival->rival_work->flags & PLAYER_ON_GROUND) {
            press |= pad_jump_buttons;
            rival->mode = RIVAL_JUMP;
        }
        break;
    case RIVAL_JUMP:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, NULL);
        stroke = 1.0F;
        if (rival->rival_motion->speed.y > 0.0F) {
            on |= pad_jump_buttons;
        } else {
            press |= pad_jump_buttons;
            rival->timer = 0;
            rival->mode = RIVAL_GLIDE;
        }
        break;
    case RIVAL_GLIDE:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, NULL);
        rival->timer++;
        stroke = 0.5F;
        if (rival->rival_work->flags & PLAYER_ON_GROUND) {
            rival->timer = 0;
            if (knuckles_get_activity(1) == KNUCKLES_CLIMBING) {
                rival->mode = RIVAL_CLIMB;
            } else {
                rival->mode = RIVAL_READY;
            }
        } else {
            on |= pad_jump_buttons;
        }
        break;
    case RIVAL_CLIMB:
        stroke = 1.0F;
        pad_records[1].y1 = STICK_UP;
        if (knuckles_get_activity(1) != KNUCKLES_CLIMBING &&
            knuckles_get_activity(1) != KNUCKLES_CLIMBING_OUT) {
            rival->mode = RIVAL_READY;
        }
        break;
    }
    if (npc_players_clash(rival->rival_task, rival->player_task)) {
        rival->mode = RIVAL_WAIT;
        rival->phase = 0;
        rival->timer = 0;
        rival->rival_task->work->flags &= ~PLAYER_HURT;
        rival->player_task->work->flags &= ~PLAYER_HURT;
        rival_push_apart(0, 1, 1.0F);
        vibration_start(0, 3, 50, 2);
    } else {
        flags = rival->rival_work->flags;
        if (!(flags & PLAYER_RECOILING) && (flags & PLAYER_HURT)) {
            rival->hits++;
            rival->timer = 0;
            rival->phase = 0;
            rival->mode = RIVAL_RECOIL;
            stream_request_voice(VOICE_HIT);
        }
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
