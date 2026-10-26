/* Chaos 2's fight camera */
#ifndef B_CHAOS2_CAMERA_H
#define B_CHAOS2_CAMERA_H

#include "camera/camera.h"

/* the camera_override step the boss gives the camera as the fight begins.
 * it sets camera_state's target to the boss's position while player 0 is
 * below y 30, and to player 0's from there up, plus
 * camera_boss_target_offset. it then runs the boss camera's steps, and
 * keeps the camera at y 35 or lower. view is unread */
void chaos2_camera_step(struct camera_view *view);

#endif
