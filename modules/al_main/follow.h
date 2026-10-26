/* a Chao that follows player 0 */
#ifndef AL_MAIN_FOLLOW_H
#define AL_MAIN_FOLLOW_H

#include "al_main/chao.h"
#include "system/task.h"

/* the behaviors of modes 29, 36 and 37, which their declarations below follow
 * in order. each starts only while bit 6, 3 or 13 of the record's minimal is
 * set, which status_require_minimal tests. where the bit is clear,
 * status_require_minimal sends the Chao back to idle and raises its tedious.
 *
 * each step turns the Chao toward player 0. once a behavior has started, it
 * also pushes the Chao ahead while their squared distance passes 3. it goes
 * back to idle once its timer passes its limit as the motion reaches its end.
 * it also goes back once the timer passes 120 with player 0 too far */

/* motion 142 and chatter, with alife_face_look's z at 0x1000 and mouth 2.
 * its limit is 180 to 379, and too far is more than 10 away */
void fun_0c913280(struct task_work *work, struct alife_chao *chao);
/* motion 127 and a quicker chatter. its limit is 180 to 379, and too far is
 * more than 10 away */
void fun_0c9134c0(struct task_work *work, struct alife_chao *chao);
/* motion 132, without a voice. it starts only with a like of 50 or more for
 * player 0's character. below that, the Chao only turns, and the behavior tests
 * again the next step. its limit is 280 to 479, and too far is more than 8
 * away */
void fun_0c9136e0(struct task_work *work, struct alife_chao *chao);

#endif
