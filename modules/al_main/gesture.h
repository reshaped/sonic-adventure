/* a Chao that makes gestures, again and again */
#ifndef AL_MAIN_GESTURE_H
#define AL_MAIN_GESTURE_H

#include "al_main/chao.h"
#include "system/task.h"

/* the behaviors of modes 11 and 12, which their declarations below follow in
 * order. each plays a count of gestures, each to its motion's end, and then
 * goes back to idle. each starts only while bit 14 or 10 of the record's
 * minimal is set, which status_require_minimal tests. where the bit is clear,
 * status_require_minimal sends the Chao back to idle and raises its tedious */

/* 8 to 27 gestures, each motion 32, 33, 34 or 35 at random with voice
 * 0x346. the eyes are 2 for a curio above 0, and 0 otherwise */
void fun_0c914380(struct task_work *work, struct alife_chao *chao);
/* 8 to 17 gestures. at 7 in 10, each is motion 92 or 94 with a chatter, and
 * otherwise motion 96 or 97 with voice 0x353. the eyes are 2 for a curio
 * above 0, and 0 otherwise, for the first 2001 steps */
void fun_0c9145a0(struct task_work *work, struct alife_chao *chao);

#endif
