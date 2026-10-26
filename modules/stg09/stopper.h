/* StoperC, StoperL and StoperR stoppers on the pinball tables */
#ifndef STG09_STOPPER_H
#define STG09_STOPPER_H

#include "system/task.h"

/* the steps the object list names for StoperC, StoperL and StoperR. each clears
 * its stopper's casino_flags bit.
 *
 * StoperC's top rises while CASINO_STOPPER_C_UP is set, and sinks while it is
 * clear. the top collides from the first step that finds the bit set. it
 * collides until a step finds the bit clear and the top all the way down.
 *
 * StoperL's and StoperR's arms swing up while CASINO_STOPPER_L_UP and
 * CASINO_STOPPER_R_UP are set. a touch of an arm while its bit is set sends
 * player 0 flying. the touch also gives 8 rings and clears the bit.
 *
 * each arm has a switch 40 units down world z from it. a touch of a switch
 * toggles CASINO_UNK_100. a toggle that clears the bit eases the tables' camera
 * to view 0 */
void stopper_c_step(struct task *task);
void stopper_l_step(struct task *task);
void stopper_r_step(struct task *task);

#endif
