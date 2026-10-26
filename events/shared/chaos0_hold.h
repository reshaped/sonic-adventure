/* an event's hold on the boss Chaos 0, not on chaos0.c's model of him. the hold
 * places and turns the boss. release, move, set_pos and set_ang act on the last
 * hold created, and all but release need one. the other calls reach the boss
 * through b_chaos0 and his ribbon, and need no hold */
#ifndef EVENT_CHAOS0_HOLD_H
#define EVENT_CHAOS0_HOLD_H
#include <sg_xpt.h>

/* create a hold, which puts the boss at x, y and z, turned by the angles.
 * it keeps where he stood, to put him back. turn is how far
 * event_chaos0_hold_move turns him each step before he moves. a failed
 * task_create or heap_alloc is unchecked */
void event_chaos0_hold(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                       Angle ang_z, Angle turn);
/* put the boss back where the hold found him, and destroy the hold.
 * chaos_end_event then hands the boss to his fight. the hold's task pointer
 * stays set, so release each hold once. with no hold, it does nothing */
void event_chaos0_hold_release(void);
/* take the boss to BOSS_MELT through chaos0_event_melt.
 * event_chaos0_hold_form takes him to BOSS_FORM through chaos0_event_form */
void event_chaos0_hold_melt(void);
void event_chaos0_hold_form(void);
/* play his ACTION_EVENT at speed, through chaos0_event_action */
void event_chaos0_hold_play_event_action(Float speed);
/* set his ribbon's extra rate, through ribbon_set_extra_rate */
void event_chaos0_hold_ribbon_rate(Float rate);
/* turn the boss toward x and z by the hold's turn each step. he then moves to
 * x, y and z. the move lasts steps + 1 steps, each 1 / steps of the way. it
 * ends a step past the point. toward his z or a higher one, the turn takes the
 * short way. that holds from any y angle from 0 to 0x10000. toward a lower z,
 * its aim is below 0. his y angle turns up to such an aim only from at most
 * 0x8000 below it. from anywhere else his y angle turns down and wraps. the
 * turn then ends only if a step lands it above 0, within turn of the aim */
void event_chaos0_hold_move(Float x, Float y, Float z, int steps);
/* move the hold and the boss to x, y and z. event_chaos0_hold_set_ang turns
 * both by the angles */
void event_chaos0_hold_set_pos(Float x, Float y, Float z);
void event_chaos0_hold_set_ang(Angle x, Angle y, Angle z);

#endif
