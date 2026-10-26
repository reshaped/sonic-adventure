/* CC3, the pinball tables' camera, which eases between nine views */
#ifndef STG09_PINBALL_CAMERA_H
#define STG09_PINBALL_CAMERA_H

#include "system/task.h"

/* CC3's step in the object list. it runs the camera in
 * CAMERA_MODE_RUIN_WAKA1 at CAMERA_LEVEL_EVENT, from the object's scl looking
 * at its pos. then it eases the camera to view 0 */
void pinball_camera_step(struct task *task);

#endif
