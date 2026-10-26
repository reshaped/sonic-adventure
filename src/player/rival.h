/* rivals: the character a battle sets against player 0, as player 1. the
 * task that drives the rival writes player 1's pad each step. each function
 * here that takes a task takes that one */
#ifndef PLAYER_RIVAL_H
#define PLAYER_RIVAL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

struct player;

/* the battles, named for the rival, then the character it fights */
enum battle_type {
    BATTLE_SONIC_KNUCKLES,
    BATTLE_SONIC_E102,
    BATTLE_KNUCKLES_SONIC,
    BATTLE_KNUCKLES_TAILS,
    BATTLE_E102_SONIC,
    BATTLE_E102_TAILS
};

/* the hits on the rival that win a battle */
#define RIVAL_HITS 3

/* set in a rival's flags while it attacks, but for the rival Knuckles'
 * attacks with one hit taken. an aim the rival E-102 hasn't locked on to
 * dies once the flag is clear */
#define RIVAL_ATTACKING 0x1

/* the stroke with which a rival that stands turns toward player 0 */
#define RIVAL_NUDGE 0.005F

struct battle {
    enum battle_type kind;
    struct task *player; /* player 0's task */
    struct task *rival; /* player 1's task, which rival_create makes */
    /* rival_begin sets it to 0. the rival's start function then copies it to
     * the rival's unk_10 */
    Sint32 unk_0c;
    /* called by rival_end with the task that drives the rival. it may be
     * NULL */
    void (*finish)(struct task *task);
    Uint8 unk_14[28];
};

/* the extra of the task that drives a rival */
struct rival {
    Sint8 mode; /* an enum rival_mode, which each rival's file defines */
    /* the rival Knuckles' punch_phase in RIVAL_PUNCH. the others only clear
     * it */
    Sint8 phase;
    Sint8 kind; /* the battle's kind, an enum battle_type */
    Sint8 hits; /* how many hits the rival has taken */
    Sint16 timer;
    /* how many attacks the rival Knuckles has picked with one hit taken. its
     * low bit picks the next: RIVAL_PUNCH_AT when set, else RIVAL_ROLL_AT */
    Sint16 turn;
    Sint8 *attacks; /* the mode it attacks in, by the hits it has taken */
    Sint32 flags; /* RIVAL_ATTACKING */
    Sint32 unk_10; /* a copy of the battle's unk_0c */
    void (*finish)(struct task *task);
    NJS_POINT3 target; /* where it runs or jumps to */
    Angle ang;
    /* the squared distance the rival Sonic's RIVAL_HOP sets and tests
     * against range[2] squared */
    Float distance2;
    Angle heading;
    Float stroke;
    Uint8 unk_34[8];
    Uint32 on;
    Uint32 press;
    struct task *rival_task;
    struct task_work *rival_work;
    struct task_motion *rival_motion;
    struct player *rival_record;
    struct task *player_task;
    struct task_work *player_work;
    struct task_motion *player_motion;
    struct player *player_record;
    /* the rival Sonic's aim: a task that follows him, for player 0's E-102
     * to lock on to */
    struct task *aim;
    struct task *rings;
    struct task *bound; /* the task of an invisible bound round the arena */
    /* distances across from player 0 within which its attacks start. each
     * mode that reads one names it */
    Float range[3];
    Uint8 unk_7c[4];
    Sint16 wait; /* RIVAL_WAIT lasts until timer passes it */
    Sint16 face_steps; /* RIVAL_FACE lasts at least until timer passes it */
    /* the steps it holds action as it attacks: the rival Sonic charges, and
     * the rival E-102 aims */
    Sint16 charge;
    Sint16 chase; /* RIVAL_ROLL lasts at most until timer passes it */
    /* the timer bits that switch the stick on and off as it rolls. the rival
     * E-102, which never rolls, counts its volleys here */
    Sint16 blink;
    Uint8 unk_8a[6];
    /* the rival's start in the arena. rival_catch_falls takes its y as the
     * floor's height */
    NJS_POINT3 start;
};

/* end the battle as the task that drives the rival is destroyed. it kills
 * the rings and the bound, turns rival_battling FALSE and calls the
 * battle's finish */
void rival_end(struct task *task);
/* show the hud's rings and lives. on a frame's first step, it also draws
 * the hits the rival can still take */
void rival_draw(struct task *task);
/* FALSE once player 0 is in PLAYER_MODE_DOWN, else TRUE. it reads player
 * 0's task as the start function kept it, so it can't tell whether that
 * task still lives */
Bool rival_player_fights(struct task *task);
/* have the rival's player task kill itself on its next step, undrawn */
void rival_kill(struct task *task);
/* begin the battle: start the task that drives battle's rival, by
 * battle's kind, and the battle's rings. battle's player and rival must be
 * live player tasks. it returns the task, or NULL for an unknown kind or
 * when the task can't be made. it clears battle->unk_0c and turns
 * rival_battling TRUE, even on NULL. with a task, rival_place has placed the
 * rival, and the outcome is BATTLE_RUNNING. the battle camera has started
 * too. for the rival Sonic or E-102, it also stops E-102's
 * countdown, even on NULL */
struct task *rival_begin(struct battle *battle);
/* TRUE from rival_begin until rival_end, as rival_set_battling sets it */
Bool rival_battling(void);
void rival_set_battling(Bool on);
/* put the rival at its arena's start, and keep its place in start.
 * outside the three arenas, the rival stays where it is */
void rival_place(struct task *task);
/* make the rival's player task, as player 1, by battle's kind, and keep it
 * in battle->rival. it returns the task, or NULL for a NULL battle or an
 * unknown kind. an unknown kind also sets battle->rival to NULL */
struct task *rival_create(struct battle *battle);
/* have battle's rival kill itself on its next step, undrawn, and set
 * battle->rival to NULL. with battle NULL, player 1's task does instead */
void rival_remove(struct battle *battle);
/* knock the two players apart, each at speed across and up at just under
 * 2, with a sound. without both players' work, only the sound plays */
void rival_push_apart(Uint8 player, Uint8 other, Float speed);
/* catch either player that falls more than 5 below start. its y goes back
 * to start's, and its speed and accel to 0. player_set_history makes start
 * its latest place. it returns 0, though its callers test the result for
 * these bits */
#define RIVAL_FELL 0x1
#define RIVAL_PLAYER_FELL 0x2
int rival_catch_falls(struct rival *rival);

#endif
