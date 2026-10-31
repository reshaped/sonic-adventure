/* rivals: what every battle shares, and the rival Sonic */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "adv02/mystic_ruins.h"
#include "camera/battle.h"
#include "collision/collision.h"
#include "collision/invisible.h"
#include "draw/screen.h"
#include "enemy/enemy.h"
#include "event/play.h"
#include "hud/hud.h"
#include "object/common.h"
#include "object/ring.h"
#include "player/e102.h"
#include "player/e102_shot.h"
#include "player/knuckles.h"
#include "player/lockon.h"
#include "player/npc.h"
#include "player/player.h"
#include "player/rival.h"
#include "player/rival_e102.h"
#include "player/rival_knuckles.h"
#include "player/sonic.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define ARENA_PRACTICE STAGE_ACT(STAGE_PRACTICE, 0)
#define ARENA_MYSTIC_RUINS STAGE_ACT(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)
#define ARENA_EGG_CARRIER STAGE_ACT(STAGE_EGG_CARRIER, 2)

/* the rival Sonic's modes. the stick is released unless a mode moves it */
enum rival_mode {
    RIVAL_START,
    RIVAL_MUSIC,
    RIVAL_WAIT, /* until timer passes wait */
    /* facing player 0, until timer passes face_steps. while the rival
     * stands, the stick turns it toward player 0 at RIVAL_NUDGE. here the
     * battle is lost once rival_player_fights is FALSE. a rival that has
     * taken RIVAL_HITS hits goes to RIVAL_BEATEN */
    RIVAL_FACE,
    /* pausing until timer passes PAUSE_STEPS, then taking up the mode
     * attacks[hits] names */
    RIVAL_PAUSE,
    RIVAL_RECOIL, /* recoiling, running from player 0 unless beaten */
    RIVAL_BEATEN,
    RIVAL_IDLE,
    /* running at player 0, to jump from the ground within range[0] */
    RIVAL_JUMP_AT,
    /* in the air to target, until it lands. it holds jump while it's more
     * than JUMP_EASE across from target. nearer, it eases the stick */
    RIVAL_JUMP,
    RIVAL_ACT_AT, /* running at player 0, to press action within range[1] */
    RIVAL_ACT_WAIT, /* pressing action, until it curls into a ball */
    /* holding action, the stick at player 0, until timer reaches charge */
    RIVAL_ACT,
    /* rolling at player 0, the stick pushed while timer & blink is 0 and
     * released otherwise. it ends once timer passes chase, or once player
     * 1's flags lose PLAYER_ATTACKING */
    RIVAL_ROLL,
    RIVAL_LEAP, /* in the air at player 0, jump held, until it lands */
    /* running at player 0, to jump from the ground within range[2] */
    RIVAL_HOP_AT,
    /* in the air at player 0, until it lands. it holds jump until it's
     * HOP_HEIGHT above player 0, then presses jump again to strike */
    RIVAL_HOP,
    /* keeping player 0 its target, until player 1's flags lose
     * PLAYER_ATTACKING */
    RIVAL_STRIKE,
    RIVAL_ACT_TARGET, /* taking player 0's place as target, to press action */
    /* rolling to target, until player 1's flags lose PLAYER_ATTACKING. near
     * target, it jumps if player 0 is in the air, or else presses action
     * and brakes */
    RIVAL_RUSH
};

enum rings_mode { RINGS_WAIT, RINGS_STEP };

/* the rival Sonic's voices. as it recovers from a hit with one hit left,
 * it says the VOICE_LAST for the character it fights */
#define VOICE_LAST_KNUCKLES 0xa9
#define VOICE_LAST_E102 0xac
#define VOICE_HIT 0xa7 /* as it takes a hit */

#define SE_PUSH_APART 598

struct aim_vars {
    Uint8 unk_00[8];
    struct task *rival_task; /* the rival it follows */
};

#define AIM(work) ((struct aim_vars *)&(work)->vars)

/* @hack type: pad_action_buttons cast to a Sint32, for the modes that OR
 * it into both press and on. SHC then copies the mask before the second OR
 * (MOV R3,R1 at 0x8c087fbc, in RIVAL_ACT_AT); uncast, 107 of 869 words
 * differ */
#define ACTION_BUTTONS ((Sint32)pad_action_buttons)
/* @hack type: pad_jump_buttons cast to a Sint32, for the same copy of the
 * jump mask (MOV R2,R1 at 0x8c088218, in RIVAL_HOP_AT); uncast, 42 of 869
 * words differ */
#define JUMP_BUTTONS ((Sint32)pad_jump_buttons)

/* the rings round the practice arena's middle, and round each post in the
 * other two arenas */
#define PRACTICE_RINGS 16
#define POST_RINGS 4

#define PAUSE_STEPS 20
#define JUMP_EASE 30.0F
#define HOP_HEIGHT 20.0F

static struct task *rival_sonic_start(struct battle *battle);
static void rival_sonic_destroy(struct task *task);
static void rival_sonic_step(struct task *self);
static void rival_aim_step(struct task *task);
static void rival_rings_place(struct task *task, struct battle *battle);
static void rival_rings_step(struct task *task);
static void rival_draw_hits(int hits, NJS_POINT3 *pos);

/* the attack the rival Sonic makes, by the hits it has taken, against
 * Knuckles and against E-102 */
static Sint8 sonic_knuckles_attacks[RIVAL_HITS] = {RIVAL_JUMP_AT, RIVAL_ACT_AT,
                                                   RIVAL_HOP_AT};
static Sint8 sonic_e102_attacks[RIVAL_HITS] = {RIVAL_JUMP_AT, RIVAL_ACT_TARGET,
                                               RIVAL_HOP_AT};

/* the shape of the rival Sonic's aim */
/* clang-format off */
static struct collision_shape rival_aim_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE, 0, PUSH_FIRM(PUSH_PLAYERS | PUSH_OBJECTS),
    {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F, {0}, 0, NULL};
/* clang-format on */

static NJS_POINT3 rival_hits_pos = {560.0F, 32.0F, 10.0F};

/* where the rival starts, by arena: practice, the Mystic Ruins and the Egg
 * Carrier */
static NJS_POINT3 rival_starts[3] = {
    {72.0F, -56.0F, -144.0F},
    {73.0F, 72.0F, 158.0F},
    {0.0F, 1525.696F, 3345.0F},
};

static Bool rival_running;

/* start the task that drives the rival Sonic, and stop E-102's countdown.
 * against E-102, it also starts the rival's aim */
static struct task *rival_sonic_start(struct battle *battle)
{
    struct task *rival_task;
    struct task *task;
    struct rival *rival;

    e102_time_runs = FALSE;
    rival_task = battle->rival;
    task = task_create(TASK_WORK, 0, rival_sonic_step);
    if (task == NULL) {
        return NULL;
    }
    rival = heap_calloc(1, sizeof(struct rival));
    task->extra = rival;
    rival->rival_task = rival_task;
    rival->rival_work = rival->rival_task->work;
    rival->rival_motion = rival->rival_task->motion;
    rival->rival_record = rival->rival_motion->record;
    rival->player_task = battle->player;
    rival->player_work = rival->player_task->work;
    rival->player_motion = rival->player_task->motion;
    rival->player_record = rival->player_motion->record;
    rival->kind = battle->kind;
    rival->unk_10 = battle->unk_0c;
    rival->finish = battle->finish;
    rival->range[0] = 70.0F;
    rival->range[1] = 50.0F;
    rival->range[2] = 100.0F;
    rival->wait = 0;
    rival->face_steps = 60;
    rival->chase = 180;
    rival->blink = 1;
    switch (rival->kind) {
    case BATTLE_SONIC_KNUCKLES:
        rival->charge = 0;
        rival->attacks = sonic_knuckles_attacks;
        break;
    case BATTLE_SONIC_E102:
        rival->charge = 30;
        rival->face_steps = 30;
        rival->attacks = sonic_e102_attacks;
        break;
    }
    rival_place(task);
    if (rival->kind == BATTLE_SONIC_E102) {
        rival->aim = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 3,
                                 rival_aim_step);
        enemy_alloc_record(rival->aim, rival->aim->work);
        collision_init(rival->aim, &rival_aim_shape, COLLISION_ITEM, 1, 1);
        AIM(rival->aim->work)->rival_task = rival->rival_task;
        e102_time_runs = FALSE;
    }
    task->draw = rival_draw;
    task->destructor = rival_sonic_destroy;
    battle_set_outcome(BATTLE_RUNNING);
    camera_start_battle(task);
    return task;
}

void rival_end(struct task *task)
{
    struct rival *rival = task->extra;

    if (rival->rings != NULL) {
        task_kill(rival->rings);
        rival->rings = NULL;
    }
    if (rival->bound != NULL) {
        task_kill(rival->bound);
        rival->bound = NULL;
    }
    rival_set_battling(FALSE);
    if (rival->finish != NULL) {
        rival->finish(task);
    }
}

void rival_draw(struct task *task)
{
    struct rival *rival = task->extra;
    int left;

    rings_shown = 1;
    lives_shown = 1;
    if (frame_step == 0 && rival != NULL) {
        left = RIVAL_HITS - rival->hits;
        if (left < 0) {
            left = 0;
        }
        rival_draw_hits(left, &rival_hits_pos);
    }
}

static void rival_sonic_destroy(struct task *task)
{
    struct rival *rival = task->extra;

    if (rival->aim != NULL) {
        task_destroy(rival->aim);
    }
    rival_end(task);
}

/* write the rival Sonic's pad and stick for the step, by its mode */
static void rival_sonic_step(struct task *self)
{
    /* @hack spelling: task, a copy of self, which SHC keeps in R8 (MOV
     * R4,R8 at 0x8c087bd8); through self, it keeps the task in a frame slot
     * and 455 of 869 words differ */
    struct task *task = self;
    struct rival *rival;
    Uint32 on;
    /* @hack type: press a Sint32, where on is a Uint32. SHC copies a mask
     * it ORs into both, as RIVAL_ACT_WAIT's action mask (MOV R2,R0 at
     * 0x8c08801a); as a Uint32, it ORs the mask itself into both, and 149
     * of 869 words differ */
    Sint32 press;
    Angle ang;
    Float stroke;
    Float across;
    Float speed;
    int caught;
    int flags;
    Float dx;
    Float dy;
    Float dz;

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
    rival->player_work->collision->flags |= COLLISION_TARGET;
    pad_disable_player(1);
    ang = 0;
    stroke = 0.0F;
    on = pad_records[1].on;
    press = pad_records[1].press;
    rival->rival_record->upgrades |= PLAYER_SONIC_LIGHT_SHOES;
    caught = rival_catch_falls(rival);
    if (caught & RIVAL_FELL) {
        rival->mode = RIVAL_BEATEN;
        rival->rival_work->pos = rival->start;
    } else if (caught & RIVAL_PLAYER_FELL) {
        /* @hack redundant: an empty arm, whose test keeps the image's TST R0,R5
         * at 0x8c087ca2. the switch's CMP/HS at 0x8c087cae sets T again
         * before any branch. without it, rival.c's code runs 64 bytes short
         * and 804 of 869 words differ */
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
            rival->mode = RIVAL_FACE;
        }
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
        if (rival->rival_record->speed.x == 0.0F) {
            ang = npc_angle_to(&rival->rival_work->pos,
                               &rival->player_work->pos, NULL, NULL);
            stroke = RIVAL_NUDGE;
        }
        rival->timer++;
        if (rival->timer > rival->face_steps) {
            rival->timer = 0;
            rival->mode = RIVAL_PAUSE;
        }
        rival->flags &= ~RIVAL_ATTACKING;
        break;
    case RIVAL_PAUSE:
        if (rival->timer++ > PAUSE_STEPS) {
            rival->timer = 0;
            rival->mode = rival->attacks[rival->hits];
            rival->flags |= RIVAL_ATTACKING;
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
            rival->mode = RIVAL_FACE;
            if (rival->hits == RIVAL_HITS - 1) {
                if (rival->kind == BATTLE_SONIC_KNUCKLES) {
                    stream_request_voice(VOICE_LAST_KNUCKLES);
                } else if (rival->kind == BATTLE_SONIC_E102) {
                    stream_request_voice(VOICE_LAST_E102);
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
    case RIVAL_JUMP_AT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        if (across < rival->range[0] &&
            (rival->rival_work->flags & PLAYER_ON_GROUND)) {
            rival->target = rival->player_work->pos;
            speed = across * (1.0F / 60.0F);
            if (speed > 1.0F) {
                speed = 1.0F;
            } else if (speed < 0.5F) {
                speed = 0.5F;
            }
            rival->rival_record->speed.x = speed;
            press |= JUMP_BUTTONS;
            rival->mode = RIVAL_JUMP;
        }
        break;
    case RIVAL_JUMP:
        if (rival->rival_work->flags & PLAYER_ON_GROUND) {
            rival->timer = 0;
            rival->mode = RIVAL_FACE;
            break;
        }
        ang = npc_angle_to(&rival->rival_work->pos, &rival->target, NULL,
                           &across);
        if (across > JUMP_EASE) {
            on |= JUMP_BUTTONS;
            stroke = 1.0F;
        } else if (across > 0.1F) {
            stroke = across * (1.0F / JUMP_EASE);
        } else {
            stroke = 0.0F;
        }
        break;
    case RIVAL_ACT_AT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        if (across < rival->range[1]) {
            stroke = 0.0F;
            press |= ACTION_BUTTONS;
            on |= ACTION_BUTTONS;
            rival->timer = 0;
            rival->mode = RIVAL_ACT;
        }
        break;
    case RIVAL_ACT_WAIT:
        if (rival->rival_work->flags & PLAYER_BALL) {
            rival->timer = 0;
            rival->mode = RIVAL_ACT;
        } else {
            stroke = 0.0F;
            press |= ACTION_BUTTONS;
            on |= ACTION_BUTTONS;
        }
        break;
    case RIVAL_ACT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        stroke = 1.0F;
        rival->timer++;
        if (rival->timer < rival->charge) {
            on |= ACTION_BUTTONS;
        } else {
            rival->timer = 0;
            switch (rival->kind) {
            case BATTLE_SONIC_KNUCKLES:
                rival->mode = RIVAL_ROLL;
                break;
            case BATTLE_SONIC_E102:
                rival->mode = RIVAL_RUSH;
                rival->target = rival->player_work->pos;
                break;
            }
        }
        break;
    case RIVAL_ROLL:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        if ((rival->timer & rival->blink) == 0) {
            stroke = 1.0F;
        } else {
            stroke = 0.0F;
        }
        rival->timer++;
        if (rival->timer > rival->chase) {
            rival->timer = 0;
            if (!(rival->player_work->flags & PLAYER_ON_GROUND) &&
                across < rival->range[0]) {
                press |= JUMP_BUTTONS;
                rival->mode = RIVAL_LEAP;
            } else {
                ang = -angle_arc_tan2(rival->rival_motion->speed.z,
                                      rival->rival_motion->speed.x);
                stroke = 1.0F;
                press |= ACTION_BUTTONS;
                rival->mode = RIVAL_FACE;
            }
        } else if ((rival->rival_work->flags & PLAYER_ATTACKING) !=
                   PLAYER_ATTACKING) {
            rival->timer = 0;
            rival->mode = RIVAL_FACE;
        }
        break;
    case RIVAL_LEAP:
        if (rival->rival_work->flags & PLAYER_ON_GROUND) {
            rival->timer = 0;
            rival->mode = RIVAL_FACE;
        } else {
            on |= JUMP_BUTTONS;
            ang = npc_angle_to(&rival->rival_work->pos,
                               &rival->player_work->pos, NULL, &across);
            stroke = 1.0F;
        }
        break;
    case RIVAL_HOP_AT:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           NULL, &across);
        if (across < rival->range[2]) {
            if (rival->rival_work->flags & PLAYER_ON_GROUND) {
                press |= JUMP_BUTTONS;
                on |= JUMP_BUTTONS;
                rival->timer = 0;
                rival->mode = RIVAL_HOP;
            }
        } else {
            stroke = 1.0F;
        }
        break;
    case RIVAL_HOP:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->player_work->pos,
                           &across, NULL);
        stroke = 1.0F;
        if (!(rival->rival_work->flags & PLAYER_ON_GROUND)) {
            if (rival->rival_work->pos.y - rival->player_work->pos.y <
                HOP_HEIGHT) {
                on |= JUMP_BUTTONS;
            } else {
                /* @bug uninitialized: distance2 sums the squares of dx, dy and
                 * dz, which no path sets. SHC squares FR15, the 1.0F of FLDI1
                 * at 0x8c087caa, three times (0x8c0882a2 to 0x8c0882aa), so
                 * distance2 is 3.0F */
#ifdef GCC
                /* keeps the shipped strike: 3.0F is under range[2] squared,
                 * so the airborne rival strikes once it is HOP_HEIGHT above
                 * the player */
                rival->player_work->collision->flags |= COLLISION_TARGET;
                press |= JUMP_BUTTONS;
                rival->mode = RIVAL_STRIKE;
#else
                /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
                rival->distance2 = (dx * dx) + (dy * dy) + (dz * dz);
                if (rival->distance2 < rival->range[2] * rival->range[2]) {
                    rival->player_work->collision->flags |= COLLISION_TARGET;
                    press |= JUMP_BUTTONS;
                    rival->mode = RIVAL_STRIKE;
                }
#endif
            }
        } else {
            rival->mode = RIVAL_FACE;
        }
        break;
    case RIVAL_STRIKE:
        if ((rival->rival_work->flags & PLAYER_ATTACKING) == PLAYER_ATTACKING) {
            rival->player_work->collision->flags |= COLLISION_TARGET;
        } else {
            rival->player_work->collision->flags &= ~COLLISION_TARGET;
            rival->mode = RIVAL_FACE;
        }
        break;
    case RIVAL_ACT_TARGET:
        rival->target = rival->player_work->pos;
        press |= ACTION_BUTTONS;
        on |= ACTION_BUTTONS;
        rival->timer = 0;
        rival->mode = RIVAL_ACT;
        break;
    case RIVAL_RUSH:
        ang = npc_angle_to(&rival->rival_work->pos, &rival->target, NULL,
                           &across);
        if (across < 0.1F) {
            stroke = 0.0F;
        } else {
            stroke = 1.0F;
        }
        if (across < 10.0F) {
            rival->timer = 0;
            if (rival->player_work->flags & PLAYER_ON_GROUND) {
                press |= ACTION_BUTTONS;
                npc_stick_against(&rival->rival_motion->speed, &stroke, &ang);
                rival->mode = RIVAL_FACE;
            } else {
                press |= JUMP_BUTTONS;
                rival->mode = RIVAL_LEAP;
            }
        } else if ((rival->rival_work->flags & PLAYER_ATTACKING) !=
                   PLAYER_ATTACKING) {
            rival->timer = 0;
            rival->mode = RIVAL_FACE;
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
    pad_sticks[1].ang = ang;
    pad_sticks[1].stroke = stroke;
    rival->ang = ang;
    rival->heading = ang;
    rival->stroke = stroke;
    rival->on = on;
    rival->press = press;
    rival_draw(task);
    task_step_children(task);
}

/* the step of the rival Sonic's aim, which keeps it halfway up his height.
 * a shot that locked on to the aim hurts him, unless he's recoiling. while
 * he guards, the shot bounces back at player 0, and he's knocked back */
static void rival_aim_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *target = LOCKON_TARGET(task);
    struct task *rival_task = AIM(work)->rival_task;
    struct task_work *rival_work = rival_task->work;
    struct player *record = player_get(1);
    struct task *shot;
    struct task_work *other;
    struct task *lockon;
    NJS_VECTOR away;

    if (rival_task == NULL) {
        return;
    }
    if (record == NULL) {
        return;
    }
    if (work->collision != NULL) {
        work->collision->flags &= ~COLLISION_TARGET;
    }
    if (work->collision->flags & COLLISION_HIT) {
        if (work->collision->hit->kind == COLLISION_ATTACK) {
            shot = work->collision->hit->owner;
            if ((lockon = lockon_of_shot(shot)) != NULL &&
                target->lockon == lockon) {
                if (rival_work->flags & PLAYER_GUARD) {
                    other = player_get_task(0)->work;
                    VECTOR_DIFF(&away, &other->pos, &rival_work->pos);
                    if (vector_magnitude(&away) > 0.0001F) {
                        vector_unit(&away, &away);
                    }
                    player_knock_back(1, -away.x, -away.y, -away.z);
                    VECTOR_SCALE(&away, &away, 5.0F);
                    e102_release_shot(shot, &away);
                    lockon_release_hit(task);
                } else {
                    if (!(rival_work->flags & PLAYER_RECOILING)) {
                        rival_work->flags |= PLAYER_HURT;
                    }
                    lockon_release(task);
                }
            }
        }
    }
    work->pos = rival_work->pos;
    work->pos.y += 0.5F * record->consts.height;
    collision_register(work);
}

Bool rival_player_fights(struct task *task)
{
    struct rival *rival = task->extra;
    struct task *player_task = rival->player_task;

    if (player_task == NULL) {
        return FALSE;
    }
    if (player_task->work->submode == PLAYER_MODE_DOWN) {
        return FALSE;
    }
    return TRUE;
}

void rival_kill(struct task *task)
{
    struct rival *rival = task->extra;
    struct task *rival_task = rival->rival_task;

    if (rival_task != NULL) {
        rival_task->step = task_kill;
        rival_task->draw = NULL;
    }
}

struct task *rival_begin(struct battle *battle)
{
    struct task *task;

    rival_set_battling(TRUE);
    battle->unk_0c = 0;
    switch (battle->kind) {
    case BATTLE_SONIC_KNUCKLES:
    case BATTLE_SONIC_E102:
        task = rival_sonic_start(battle);
        break;
    case BATTLE_KNUCKLES_SONIC:
    case BATTLE_KNUCKLES_TAILS:
        task = rival_knuckles_start(battle);
        break;
    case BATTLE_E102_SONIC:
    case BATTLE_E102_TAILS:
        task = rival_e102_start(battle);
        break;
    default:
        task = NULL;
        break;
    }
    if (task != NULL) {
        rival_rings_place(task, battle);
    }
    return task;
}

Bool rival_battling(void)
{
    return rival_running;
}

void rival_set_battling(Bool on)
{
    rival_running = on;
}

/* place the battle's rings, as children of a task of their own in
 * rival->rings. the practice arena has a circle of them round its middle.
 * the other two arenas have them round each of three posts. they also get
 * an invisible bound round the arena, in rival->bound */
static void rival_rings_place(struct task *task, struct battle *battle)
{
    NJS_POINT3 mystic_ruins_posts[3] = {
        {105.0F, 81.0F, 330.0F},
        {13.0F, 81.0F, 281.0F},
        {104.0F, 81.0F, 194.0F},
    };
    NJS_POINT3 egg_carrier_posts[3] = {
        {-88.0F, 1533.0F, 3335.0F},
        {104.0F, 1533.0F, 3342.0F},
        {-2.0F, 1533.0F, 3249.0F},
    };
    NJS_POINT3 practice_center = {72.0F, -56.0F, -144.0F};
    struct rival *rival = task->extra;
    struct task *child;
    NJS_POINT3 *post;
    Float sin;
    Float cos;
    Float radius;
    Angle ang;
    int stage;
    int i;

    if (rival == NULL) {
        return;
    }
    rival->rings = task_create(TASK_WORK, 3, rival_rings_step);
    if (rival->rings == NULL) {
        return;
    }
    /* @hack redundant: a read of rival->rings that changes nothing, which
     * keeps rival in R14 (MOV.L @(44,R4),R14 at 0x8c08883c); without it,
     * rival takes R13, task_create_child's address R14, and 19 of 429 words
     * differ */
    (void)rival->rings;
    stage = stage_get_stage_act();
    switch (stage) {
    case ARENA_PRACTICE:
        /* @hack spelling: post set before radius, which loads the 5.0F added to
         * post->y into FR14 (FMOV.S @R0,FR14 at 0x8c0888e8) ahead of
         * post's address into R11 (MOV R15,R11 at 0x8c0888ea); with radius
         * first, as the other cases set it, the two trade and 2 of 429 words
         * differ */
        post = &practice_center;
        radius = 50.0F;
        for (ang = 0; ang < 0x10000; ang += 0x10000 / PRACTICE_RINGS) {
            _builtin_fsca(ang, &sin, &cos);
            child = task_create_child(TASK_WORK | TASK_EXTRA, ring_step,
                                      rival->rings);
            child->work->pos.x = post->x + (sin * radius);
            child->work->pos.y = post->y + 5.0F;
            child->work->pos.z = post->z + (cos * radius);
        }
        break;
    case ARENA_MYSTIC_RUINS:
        radius = 10.0F;
        post = mystic_ruins_posts;
        for (i = 0; i < 3; i++) {
            for (ang = 0; ang < 0x10000; ang += 0x10000 / POST_RINGS) {
                _builtin_fsca(ang, &sin, &cos);
                child = task_create_child(TASK_WORK | TASK_EXTRA, ring_step,
                                          rival->rings);
                child->work->pos.x = post->x + (sin * radius);
                child->work->pos.y = post->y;
                child->work->pos.z = post->z + (cos * radius);
            }
            post++;
        }
        child = rival->bound =
            task_create(TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 2,
                        invisible_bound_step);
        child->work->pos.x = 80.0F;
        child->work->pos.y = 79.0F;
        child->work->pos.z = 254.0F;
        child->work->scl.x = 100.0F;
        break;
    case ARENA_EGG_CARRIER:
        radius = 10.0F;
        post = egg_carrier_posts;
        /* @hack spelling: the posts counted down in a do-while, whose test
         * comes after the body. it keeps the ring step in R11 and post in R10
         * (0x8c088878, 0x8c088aa4); as a for loop like the Mystic Ruins', the
         * two swap and 23 of 429 words differ */
        i = 3;
        do {
            for (ang = 0; ang < 0x10000; ang += 0x10000 / POST_RINGS) {
                _builtin_fsca(ang, &sin, &cos);
                child = task_create_child(TASK_WORK | TASK_EXTRA, ring_step,
                                          rival->rings);
                child->work->pos.x = post->x + (sin * radius);
                child->work->pos.y = post->y;
                child->work->pos.z = post->z + (cos * radius);
            }
            post++;
        } while (--i != 0);
        child = rival->bound =
            task_create(TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 2,
                        invisible_bound_step);
        child->work->pos.x = -13.0F;
        child->work->pos.y = 1525.0F;
        child->work->pos.z = 3338.0F;
        child->work->scl.x = 200.0F;
        break;
    }
}

/* the rings' task. it waits six steps, then steps the rings while both
 * players have work */
static void rival_rings_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player0;
    struct task_work *player1;

    switch (work->mode) {
    case RINGS_WAIT:
        work->tick++;
        if (work->tick > 5) {
            work->tick = 0;
            work->mode = RINGS_STEP;
        }
        break;
    case RINGS_STEP:
        player0 = player_work[0];
        player1 = player_work[1];
        if (player0 != NULL && player1 != NULL) {
            task_step_children(task);
        }
        break;
    }
}

void rival_place(struct task *task)
{
    struct rival *rival = task->extra;
    int stage;

    stage = stage_get_stage_act();
    switch (stage) {
    case ARENA_PRACTICE:
        rival->rival_task->work->pos = rival_starts[0];
        break;
    case ARENA_MYSTIC_RUINS:
        rival->rival_task->work->pos = rival_starts[1];
        break;
    case ARENA_EGG_CARRIER:
        rival->rival_task->work->pos = rival_starts[2];
        break;
    }
    rival->start = rival->rival_task->work->pos;
}

struct task *rival_create(struct battle *battle)
{
    struct task *task = NULL;

    if (battle == NULL) {
        return NULL;
    }
    switch (battle->kind) {
    case BATTLE_SONIC_KNUCKLES:
    case BATTLE_SONIC_E102:
        task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1, sonic_step);
        PLAYER_VARS(task->work)->character = CHARACTER_SONIC;
        PLAYER_VARS(task->work)->player = 1;
        break;
    case BATTLE_KNUCKLES_SONIC:
    case BATTLE_KNUCKLES_TAILS:
        task =
            task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1, knuckles_step);
        PLAYER_VARS(task->work)->character = CHARACTER_KNUCKLES;
        PLAYER_VARS(task->work)->player = 1;
        break;
    case BATTLE_E102_SONIC:
    case BATTLE_E102_TAILS:
        task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1, e102_step);
        PLAYER_VARS(task->work)->character = CHARACTER_E102;
        PLAYER_VARS(task->work)->player = 1;
        break;
    }
    battle->rival = task;
    return task;
}

void rival_remove(struct battle *battle)
{
    struct task *task;

    if (battle != NULL) {
        if (battle->rival != NULL) {
            battle->rival->step = task_kill;
            battle->rival->draw = NULL;
            battle->rival = NULL;
        }
    } else {
        task = player_get_task(1);
        if (task != NULL) {
            task->step = task_kill;
            task->draw = NULL;
        }
    }
}

void rival_push_apart(Uint8 player, Uint8 other, Float speed)
{
    struct task_work *work;
    struct task_work *other_work;
    NJS_VECTOR away;

    sound_play(SE_PUSH_APART, 0, 0, 0);
    work = player_work[player];
    other_work = player_work[other];
    if (work != NULL && other_work != NULL) {
        away.x = work->pos.x - other_work->pos.x;
        away.y = work->pos.y - other_work->pos.y;
        away.z = work->pos.z - other_work->pos.z;
        vector_unit_across(&away, &away);
        away.x *= speed;
        away.y = 1.9999999F;
        away.z *= speed;
        player_knock_back(player, away.x, away.y, away.z);
        player_knock_back(other, -away.x, away.y, -away.z);
    }
}

int rival_catch_falls(struct rival *rival)
{
    if (rival->start.y - rival->rival_work->pos.y > 5.0F) {
        rival->rival_work->pos.y = rival->start.y;
        player_set_history(1, 0, &rival->start, NULL);
        player_set_speed(1, 0.0F, 0.0F, 0.0F);
        player_set_accel(1, 0.0F, 0.0F, 0.0F);
    }
    if (rival->start.y - rival->player_work->pos.y > 5.0F) {
        rival->player_work->pos.y = rival->start.y;
        player_set_history(0, 0, &rival->start, NULL);
        player_set_speed(0, 0.0F, 0.0F, 0.0F);
        player_set_accel(0, 0.0F, 0.0F, 0.0F);
    }
    return 0;
}

/* draw the number hits as the hud's texture for it, at pos on the
 * screen */
static void rival_draw_hits(int hits, NJS_POINT3 *pos)
{
    screen_quad_init();
    njSetTexture(&hud_texlist);
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
#ifdef GCC
    /* the port's 16:9 squeezes the count about the screen's right edge */
    port_wide_anchor = WIDE_RIGHT;
#endif
    screen_quad_draw(hits, pos->x, pos->y, pos->z);
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
}
