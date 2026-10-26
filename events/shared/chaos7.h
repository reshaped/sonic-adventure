/* an event's hold on the boss Chaos 7, which places and turns him. release,
 * move, set_pos and set_ang act on the last hold created, and all but
 * release need one. the mode calls reach the boss through b_chaos7, and
 * need no hold */
#ifndef EVENT_CHAOS7_H
#define EVENT_CHAOS7_H
#include <sg_xpt.h>

/* create a hold, which puts the boss at x, y and z, turned by the angles.
 * it keeps where he stood, to put him back. turn is how far
 * event_chaos7_move turns him each step before he moves. a failed
 * task_create or heap_alloc is unchecked */
void event_chaos7_hold(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                       Angle ang_z, Angle turn);
/* put the boss back where the hold found him, destroy the hold and clear
 * its task pointer. when fight is TRUE, chaos_end_event then hands the boss
 * to his fight. with no hold, it does nothing */
void event_chaos7_release(Bool fight);
/* unless the boss is in that mode already, ask him for BOSS_READY, BOSS_MODE_2,
 * BOSS_MODE_13, BOSS_MODE_14 or BOSS_EVENT_RAY, through chaos7_event_mode_1 to
 * chaos7_event_ray. when it asks, event_chaos7_ray also sets the ray's
 * length to length */
void event_chaos7_mode_1(void);
void event_chaos7_mode_2(void);
void event_chaos7_mode_13(void);
void event_chaos7_mode_14(void);
void event_chaos7_ray(Float length);
/* turn the boss toward x and z by the hold's turn each step. he then moves to
 * x, y and z. the move lasts steps + 1 steps, each 1 / steps of the way. it
 * ends a step past the point. toward his z or a higher one, the turn takes the
 * short way. that holds from any y angle from 0 to 0x10000. toward a lower z,
 * its aim is below 0. his y angle turns up to such an aim only from at most
 * 0x8000 below it. from anywhere else his y angle turns down and wraps. the
 * turn then ends only if a step lands it above 0, within turn of the aim */
void event_chaos7_move(Float x, Float y, Float z, int steps);
/* move the hold and the boss to x, y and z. event_chaos7_set_ang turns both by
 * the angles */
void event_chaos7_set_pos(Float x, Float y, Float z);
void event_chaos7_set_ang(Angle x, Angle y, Angle z);

#endif
