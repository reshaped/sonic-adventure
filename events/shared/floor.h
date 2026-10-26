/* an event's floor: a solid triangle in the collision world under a task */
#ifndef EVENT_FLOOR_H
#define EVENT_FLOOR_H
#include <sg_xpt.h>

#include "system/task.h"

/* create a floor at height y, which each step moves under target's x and z.
 * target must outlive it. failures of task_create and world_object_alloc are
 * unchecked */
struct task *event_floor_create(struct task *target, Float y);

#endif
