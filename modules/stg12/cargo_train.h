/* the cargo ride's train, which runs back and forth along z between two
 * stops */
#ifndef STG12_CARGO_TRAIN_H
#define STG12_CARGO_TRAIN_H

#include <sg_xpt.h>

#include "system/task.h"

/* the train's speed along z, by which its parts move each step */
extern Float cargo_train_speed;

/* the first step of a task with a work. it makes the train's parts as its
 * children, then runs the train until cargo_ride_on is FALSE */
void cargo_train_start(struct task *task);

#endif
