/* 0 LIGHT CAMERA camera that turns to watch the nearest player, with a lamp
 * on its lens */
#ifndef STG10_LIGHT_CAMERA_H
#define STG10_LIGHT_CAMERA_H

#include "system/task.h"

/* the object list's step */
void light_camera_step(struct task *task);

#endif
