/* O HAND claw, which grabs player 0 and shakes its rings out into the gold
 * heap */
#ifndef STG09_HAND_H
#define STG09_HAND_H

#include "system/task.h"

/* the object list's step. the hand waits 100 steps after it starts and after
 * each grab. it also waits while the gold heap holds more than 9999 rings.
 * then, with switch 1 on, it grabs if ring_count is above 0. with switch 1 on
 * and no rings, it turns switch 1 off.
 *
 * a grab adds 10 to player 0's launch_steps and stops player 0. it also stops
 * the player nearest the hand, whose position and angles become the restart
 * point. the jaws take player 0 if the hand's collision record touches another
 * record as they close. scl then gets player 0's position. jaws that take
 * nothing turn switch 1 off and go back.
 *
 * the jaws shake player 0's rings out into the heap. as they open, they put
 * the rest in the heap and let player 0 go. counters 1 and 2 then show the
 * heap's rings. if the action ends first, player 0 keeps the rest */
void hand_step(struct task *task);

#endif
