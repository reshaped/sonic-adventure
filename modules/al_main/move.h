/* how a Chao makes for its target, and the behaviors that send it */
#ifndef AL_MAIN_MOVE_H
#define AL_MAIN_MOVE_H
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "system/task.h"

/* where the Chao's pos is more than 1 below water_level, send it in MODE_GO to
 * the point 40 along its facing, and return TRUE. last_mode keeps the mode it
 * leaves. its entry's held clears, and so does the held entry's held where it
 * points back. return FALSE, with nothing changed, for a Chao higher up, and
 * for one already in the mode of a behavior below but alife_join_court */
Bool fun_0c915b00(struct task_work *work, struct alife_chao *chao);

/* the behaviors below run the Chao's move. each step, a behavior changes
 * the move as the Chao's terrain and target call for, and steps it. a
 * behavior ends by going back to idle. mode_timer counts its steps toward
 * a time limit, and restarts at each change of move. whatever the behavior,
 * MOVE_STOP sends the Chao back to idle as its motion ends, and MOVE_SWIM
 * can send it to a place in MODE_GO */

/* MODE_GO's: to the Chao's target, until within 10 of it or once
 * mode_timer passes 2000 */
void alife_go(struct task_work *work, struct alife_chao *chao);
/* MODE_GO_2's, the same as MODE_GO's */
void fun_0c9170c0(struct task_work *work, struct alife_chao *chao);
/* MODE_GO_TO_PLAYER's: to player 0, with eyes 2, until within 13 or once
 * mode_timer passes 1200 */
void alife_go_to_player(struct task_work *work, struct alife_chao *chao);
/* MODE_JOIN_COURT's: as MODE_GO's. the Chao pairs with one in MODE_COURT
 * that it touches, whose entry's held is null. the two entries take each
 * other as held, and the Chao goes to MODE_PARTNER with a sound. of several
 * it touches in a step, each takes its entry as held, and its own entry
 * keeps the last */
void alife_join_court(struct task_work *work, struct alife_chao *chao);
/* mode 10's: to player 0's pos less twice the Chao's own, until within 15 of it
 * or once mode_timer passes 1200. the behavior shares its work's submode with
 * the moves, each of which starts at submode 0. at submode 0, the behavior
 * calls out and sets submode 1 before the move steps, so the move skips its
 * start. that happens on the first step, and again each time a move's own step
 * changes the move, with another call each time */
void fun_0c917140(struct task_work *work, struct alife_chao *chao);
/* mode 31's: to player 0's pos less twice the Chao's own. it has no end of
 * its own */
void fun_0c917240(struct task_work *work, struct alife_chao *chao);

#endif
