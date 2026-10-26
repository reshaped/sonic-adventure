/* ShoogaiL and ShoogaiR, obstacles on the first table, which sink when
 * touched and rise again 1500 steps later */
#ifndef STG09_OBSTACLE_H
#define STG09_OBSTACLE_H

#include "system/task.h"

/* the object list's steps. each new touch of an obstacle gives a ring. each
 * obstacle has a bit in casino_flags: CASINO_SHOOGAI_L_DOWN for ShoogaiL, and
 * CASINO_SHOOGAI_R_DOWN for ShoogaiR. a touch while the bit is clear sets it
 * for 1500 steps. the obstacle sinks while its bit is set, and rises while
 * it's clear */
void obstacle_left_step(struct task *task);
void obstacle_right_step(struct task *task);

#endif
