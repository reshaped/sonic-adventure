/* EV0087's hold on the Chaos 2 boss: a task that places, turns and moves
 * him through boss_work. calls into B_CHAOS2.PRS set his modes */
#ifndef EV0087_CHAOS2_HOLD_H
#define EV0087_CHAOS2_HOLD_H
#include <sg_xpt.h>

/* save boss_work's position and angles. then put the boss at x, y, z with
 * the given angles. turn is how far a move turns his y angle each step
 * before he moves. don't hold again before the release. a second hold
 * replaces the task in the slot, and the first runs on. the task, and
 * the heap block that saves the boss's place, are allocated unchecked */
void ev0087_chaos2_hold(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                        Angle ang_z, Angle turn);
/* give boss_work back the position and angles the hold saved. then destroy
 * the task, clear its slot and set chaos_handed_over through
 * chaos_end_event. without a hold it does nothing */
void ev0087_chaos2_release(void);
/* ask the boss for BOSS_MELT, for BOSS_FORM, or to play ACTION_EVENT at
 * speed. the calls go through B_CHAOS2.PRS, which must be in the stage
 * slot */
void ev0087_chaos2_melt(void);
void ev0087_chaos2_form(void);
void ev0087_chaos2_action(Float speed);
/* set the ribbon's extra rate through ribbon_set_extra_rate: the most beads
 * it starts on top of its own rate */
void ev0087_chaos2_ribbon_rate(Float rate);
/* turn the boss's heading, his y angle, by the hold's turn each step toward
 * x, z. then move him a 1/steps share of the way to x, y, z each step. the
 * move runs steps + 1 steps, so he stops one share past x, y, z. steps must
 * be positive. a target at a lower z than his lies at a negative heading.
 * if his heading starts outside the half turn below that, the turn only
 * lowers it. it wraps to 0x10000 at 0. unless the target's heading is
 * within the hold's turn of 0, he then turns forever and never moves */
void ev0087_chaos2_move(Float x, Float y, Float z, int steps);
/* set the boss's position or angles, in boss_work and the hold's task. these
 * and the move read the task without testing it, so call them only during a
 * hold */
void ev0087_chaos2_set_pos(Float x, Float y, Float z);
void ev0087_chaos2_set_ang(Angle x, Angle y, Angle z);

#endif
