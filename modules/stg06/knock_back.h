/* the knock-back a cannon's shell gives player 0 as it hits the target player 0
 * stands on */
#ifndef STG06_KNOCK_BACK_H
#define STG06_KNOCK_BACK_H

#include "system/task.h"

/* send player 0 flying 3.5 ahead and 2.5 up, its stick ignored for 60
 * steps. ahead is the way from work to the nearest player. in act 0, it is
 * +z while that player's z is below 2000. the sound is one for Sonic, and
 * another for the other characters. the nearest player rises 1, though it
 * need not be player 0 */
void sky_deck_knock_back(struct task_work *work);

#endif
