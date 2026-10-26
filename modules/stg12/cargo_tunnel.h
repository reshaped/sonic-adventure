/* the cargo ride's tunnel walls, drawn scrolling past the train */
#ifndef STG12_CARGO_TUNNEL_H
#define STG12_CARGO_TUNNEL_H

#include "system/task.h"

/* the first step of a task with a work, which cargo_ride.c makes as the ride
 * starts. the task draws the walls until cargo_ride_on is FALSE */
void cargo_tunnel_start(struct task *task);

#endif
