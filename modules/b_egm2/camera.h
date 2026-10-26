/* the Egg Walker fight's camera, its shake, and the sky drawn about it */
#ifndef B_EGM2_CAMERA_H
#define B_EGM2_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* take the camera with camera_override for the fight's step. the step
 * moves the camera with an anchor task, which starts at player 0. player 0
 * must exist. pos and ang are the boss's work position and angles. the step
 * reads them from then on, so they must outlive it */
void egg_walker_camera_start(NJS_POINT3 *pos, struct angles *ang);
/* for the camera's next step only. the step checks again which side of the
 * boss player 0 is on. near the boss, the camera's goal turns about the body
 * by at most a few degrees a step. it also sinks toward the street. away
 * from the boss, the camera acts as egg_walker_camera_face_boss makes it */
void egg_walker_camera_look_past(void);
/* for the camera's next step only. while player 0 is away from the boss,
 * the camera looks at the boss's body. its goal lies a short way past
 * player 0, on the side away from the body. nothing in B_EGM2 calls it */
void egg_walker_camera_face_boss(void);
/* draw the sky's model at the camera's position. the boss's draw calls it */
void egg_walker_sky_draw(void);
/* raise the camera and its target by half of power. the offset then
 * settles back to 0 as a damped spring, over the steps after */
void egg_walker_camera_shake(Uint8 power);
/* the camera's position at its last step, after the shake and the street's
 * bounds. nothing in B_EGM2 calls it */
NJS_POINT3 *egg_walker_camera_pos(void);
/* make a task that stands 40 above pos and does nothing, for the camera to
 * swing round. the boss's entrance passes its work to camera_start_swing.
 * returns the task's work, or NULL when no task is free */
struct task_work *egg_walker_camera_target_create(NJS_POINT3 *pos);
/* TRUE if player 0 was at a greater z than the boss's body at the last side
 * check. the camera's step checks as player 0 comes near the boss, and
 * after egg_walker_camera_look_past */
Bool egg_walker_camera_beyond_boss(void);

#endif
