/* an event's light: Tikal's light at a point */
#ifndef EVENT_LIGHT_H
#define EVENT_LIGHT_H
#include <sg_xpt.h>

#include "system/task.h"

/* make Tikal's light at x, y and z, under a task with an event record to
 * run paths. returns the task, or null when task_create fails */
struct task *event_light_create(Float x, Float y, Float z);

#endif
