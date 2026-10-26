/* a task that moves to the event camera's position each step, for another task
 * to look at */
#ifndef EV0061_LOOK_CAMERA_H
#define EV0061_LOOK_CAMERA_H

#include "system/task.h"

/* make task look at a new task that moves to the event camera's position. if
 * that task can't be created, nothing changes. nothing in EV0061.PRS calls
 * this or ev0061_look_at_camera_stop, and EV0061.PRS holds neither function's
 * address */
void ev0061_look_at_camera(struct task *task);
/* clear task's look, unless task is NULL. kill the newest task that
 * ev0061_look_at_camera created, unless a call to this killed it already.
 * older ones keep running */
void ev0061_look_at_camera_stop(struct task *task);

#endif
