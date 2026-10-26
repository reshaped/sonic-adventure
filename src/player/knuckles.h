/* Knuckles as a player: his modes, and the hunt for the pieces of the master
 * emerald */
#ifndef PLAYER_KNUCKLES_H
#define PLAYER_KNUCKLES_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* what Knuckles is doing, in work->mode. knuckles_mode_names in knuckles.c
 * gives the image's name for each, in this order */
enum knuckles_mode {
    KNUCKLES_INIT,
    KNUCKLES_STAND,
    KNUCKLES_WALK,
    KNUCKLES_PATH, /* running along a path */
    KNUCKLES_ROLL, /* curled up, rolling on the ground */
    KNUCKLES_PATH_SPIN, /* curled up, along a path */
    KNUCKLES_JUMP,
    KNUCKLES_SPRING,
    KNUCKLES_LAUNCH,
    KNUCKLES_FALL,
    KNUCKLES_STOP, /* braking */
    KNUCKLES_GLIDE,
    /* turning back after he glides past the edges of the act's death
     * zones. once back over them, he glides on straight for 30 steps */
    KNUCKLES_GLIDE_TURN,
    KNUCKLES_PUSH,
    KNUCKLES_HURT,
    KNUCKLES_BACK,
    KNUCKLES_POSE, /* still, in the action a request set */
    /* knocked back, off a wall he ran into or by PLAYER_MODE_KNOCK_BACK */
    KNUCKLES_CRASH,
    /* flying at the nearest target: the attack his fighting gloves charge */
    KNUCKLES_HOMING,
    KNUCKLES_DIG_DOWN,
    KNUCKLES_DIG_UP,
    KNUCKLES_DIG_OUT, /* up out of the hole, and falling */
    KNUCKLES_DIG_FAIL, /* trying to dig ground he can't dig */
    KNUCKLES_DIG_FAIL_BACK, /* KNUCKLES_DIG_FAIL's frames, played back */
    KNUCKLES_CLIMB,
    KNUCKLES_CLIMB_OUT, /* up over the top of the wall */
    KNUCKLES_PUNCH,
    KNUCKLES_LIFT,
    KNUCKLES_PUT_DOWN,
    KNUCKLES_HOLD_STAND,
    KNUCKLES_GRIP, /* holding on to a task, to push, pull or shake it */
    KNUCKLES_HOLD_WALK,
    KNUCKLES_THROW,
    KNUCKLES_HOLD_STOP,
    KNUCKLES_HOLD_FALL,
    KNUCKLES_HOLD_JUMP,
    KNUCKLES_HOLD_SPRING,
    KNUCKLES_HOLD_LAUNCH,
    KNUCKLES_PULL,
    KNUCKLES_AIR_THROW,
    KNUCKLES_SHAKE, /* shaking the task he grips */
    KNUCKLES_HOLD_SHAKE, /* shaking the CARRY_KIND_SMALL task he holds */
    KNUCKLES_HANG, /* hanging from a path */
    KNUCKLES_HANG_GO, /* moving along the path he hangs from */
    KNUCKLES_LEDGE, /* hanging from a ledge's path */
    KNUCKLES_LEDGE_GO,
    KNUCKLES_PANEL_JUMP, /* jumping from a panel to the next */
    KNUCKLES_PANEL_WAIT, /* standing on a panel */
    KNUCKLES_ICED, /* in a block of ice, until he struggles free */
    KNUCKLES_EVENT, /* moved along an event's paths, until it has none */
    /* drowning: an action, then up to the water's surface above him */
    KNUCKLES_RISE,
    KNUCKLES_DOWN,
    /* riding, still in a stage module's action, until PLAYER_MODE_RIDE_OFF */
    KNUCKLES_MODULE_POSE,
    KNUCKLES_EDIT /* in the set editor, where the stick moves him freely */
};

/* what Knuckles is doing, as knuckles_get_activity reports it */
enum knuckles_activity {
    KNUCKLES_DIGGING_DOWN = 1,
    KNUCKLES_DIGGING_UP,
    KNUCKLES_DIGGING_OUT, /* up out of the hole, and falling */
    KNUCKLES_PUNCHING,
    KNUCKLES_CLIMBING,
    KNUCKLES_CLIMBING_OUT, /* up over the top of the wall */
    KNUCKLES_GLIDING /* in KNUCKLES_GLIDE or KNUCKLES_GLIDE_TURN */
};

/* the id of a place a piece of the master emerald can lie. it is the
 * place's group, under the mask PIECE_GROUP, ORed with its number among the
 * group's places from 0. a group holds the places of one piece: the first,
 * the second buried, the second not buried, or the third */
#define PIECE_ID_FIRST 16
#define PIECE_ID_LAST 79
#define PIECE_GROUP 0x70
#define PIECE_FIRST 0x10
#define PIECE_BURIED 0x20
#define PIECE_SECOND 0x30
#define PIECE_THIRD 0x40

/* the step of Knuckles' player task */
void knuckles_step(struct task *task);
/* what player's Knuckles is doing, as an enum knuckles_activity. 0 when
 * player has no task or isn't Knuckles, or while he does anything else */
int knuckles_get_activity(int player);
/* start the hunt: pick the act's three pieces at random, each among its
 * group's places. the second may be a buried one only once he has the
 * shovel claws. a demo of Red Mountain's third act always picks the same
 * three. TRUE once they're picked. FALSE, with the hunt off, for a
 * character other than Knuckles or an act without pieces */
Bool knuckles_pick_pieces(void);
/* the piece nearest to from of those found and not taken. TRUE with its
 * place in *pos and its distance in *distance. FALSE, leaving both as they
 * were, when no such piece lies within 99999 of from. FALSE too while
 * player 0 has no task or isn't Knuckles, while the hunt is off, and once
 * all three are taken */
Bool knuckles_nearest_piece(NJS_POINT3 *from, NJS_POINT3 *pos, Float *distance);
/* the hunt's calls for a piece's place. each reads only the low byte of
 * id. each returns FALSE when the hunt is off, or id is the place of none
 * of the three pieces */

/* find piece id at pos, for the radar and knuckles_nearest_piece. TRUE
 * once found. FALSE also for a piece already taken */
Bool knuckles_find_piece(int id, NJS_POINT3 *pos);
/* take piece id, which flies to Knuckles from pos, or for a buried one from
 * him. the act's restart point moves to the piece's mark, or else to the
 * place knuckles_find_piece last gave it, not to pos. TRUE once taken.
 * FALSE also for a piece already taken */
Bool knuckles_take_piece(int id, NJS_POINT3 *pos);
/* mark pos as where Knuckles restarts once he takes piece id. TRUE once
 * marked. FALSE also for a piece already marked */
Bool knuckles_mark_piece(int id, NJS_POINT3 *pos);

/* the sounds knuckles_force_dig takes: that of a dig that finds nothing,
 * and that of one that finds an item */
#define KNUCKLES_DIG_SOUND_NOTHING 1
#define KNUCKLES_DIG_SOUND_ITEM 2
/* until Knuckles' step next ends, a dig turns up nothing. one away from a
 * buried piece plays sound at once, or no sound for any other value */
void knuckles_force_dig(int sound);

#endif
