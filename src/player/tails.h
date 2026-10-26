/* Tails as a player: his modes, flight, tail attack and board */
#ifndef PLAYER_TAILS_H
#define PLAYER_TAILS_H
#include <sg_xpt.h>

struct task;

/* Tails' bytes in work->vars */
struct tails_vars {
    Uint8 player; /* as in struct player_vars */
    Uint8 character;
    Uint16 struggle; /* what's left of his struggle to break free */
    /* @hack type: four bytes as an array, whose address SHC forms to reach
     * one (ADD #12,R5 at 0x8c0c56fe). as four Sint8 fields, which it loads
     * at a displacement, 26 of 21307 words differ, all in tails_fly, summed
     * over the 21 functions that use them: 12 in tails.c, kusa_step, two in
     * b_chaos4's column.c and six events' scripts */
    Sint8 own[4];
};

#define TAILS_VARS(work) ((struct tails_vars *)&(work)->vars)
/* TAILS_PHASE is his flight's phase, which goes back to 0 once it reaches
 * 64. TAILS_CLIMB is 1 as he climbs, -1 as he dives, and 0 otherwise.
 * TAILS_STATE and TAILS_FLAGS hold the bits below */
#define TAILS_PHASE(work) (TAILS_VARS(work)->own[0])
#define TAILS_CLIMB(work) (TAILS_VARS(work)->own[1])
#define TAILS_STATE(work) (TAILS_VARS(work)->own[2])
#define TAILS_FLAGS(work) (TAILS_VARS(work)->own[3])

/* TAILS_STATE bits */
#define TAILS_FLYING 0x1
#define TAILS_ATTACKING 0x2 /* with his tails */

/* TAILS_FLAGS bits */
#define TAILS_STOP_FLYING 0x1 /* set by others: he stops flying and falls */
/* set as he hands the camera to camera_step_follow at a flight's start.
 * nothing clears it, so from then on camera_restore runs as he lands from
 * TAILS_FLY, TAILS_FALL or TAILS_TIRED, and as a request ends his flight */
#define TAILS_FLIGHT_CAMERA 0x2
#define TAILS_TAIL_SPIN 0x4 /* his tails spin as in flight */
#define TAILS_UNTOUCHABLE 0x8 /* he touches nothing */
#define TAILS_TAIL_STILL 0x10 /* his flying tails stay still */
#define TAILS_FLYING_HEAD 0x20 /* his mouth moves on the flying head */

/* Tails' task, as a player */
void tails_step(struct task *task);
/* start his clear: the camera's path round him with the clear's sound and
 * music, or, in a race he lost, his voice. in a race, a task then shows the
 * result */
void tails_clear(void);
/* the first step of his board's task */
void tails_board_start(struct task *task);
/* set TAILS_TAIL_SPIN in player's TAILS_FLAGS, so his tails spin as in
 * flight whatever he does. the player must be Tails: it sets the bit in
 * any character's work vars. it returns the player's TAILS_FLAGS with the
 * bit set, or 0 with no such player. tails_set_tail_spin_copy, declared
 * below it, does the same */
Sint8 tails_set_tail_spin(int player);
Sint8 tails_set_tail_spin_copy(int player);

#endif
