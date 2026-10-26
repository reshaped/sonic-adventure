/* CAM COLLI, the gardens' camera keep-outs */
#ifndef AL_MAIN_CAM_COLLI_H
#define AL_MAIN_CAM_COLLI_H

#include "system/task.h"

/* the step the Station Square and Mystic Ruins gardens' object lists name
 * for CAM COLLI. each step it pushes garden_camera_pos out of a sphere
 * whose radius is 15 past its work's scl.x, the harder the deeper the
 * camera is. while the set editor is open it draws the sphere too */
void cam_colli_step(struct task *task);

#endif
