/* the tables' Tuuka, a row of three lanes with lamps, and RightLane, one
 * lane */
#ifndef STG09_LANES_H
#define STG09_LANES_H

#include "system/task.h"

/* the object list's steps. a touch on one of Tuuka's three lanes lights
 * that lane's lamp and gives a ring. once all three are lit, Tuuka gives
 * rings and flashes its lamps, and then unlights them. the triggers and A
 * turn the lit lamps round the lanes. RightLane gives rings at each touch,
 * and more at every fifth. while some of the tables' countdowns run, the
 * lanes give more rings, and Tuuka fires HoudaiL and HoudaiR */
void lanes_step(struct task *task);
void right_lane_step(struct task *task);

#endif
