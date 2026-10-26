/* the ruin's path cameras, which run the camera along a curve through a
 * path's points */
#ifndef STG07_PATH_CAMERA_H
#define STG07_PATH_CAMERA_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the steps of the tasks path_create_tasks makes for the stage's paths. both
 * take one set of settings, and act 0's paths name both */
void ruin_path_camera_0(struct task *task);
void ruin_path_camera_1(struct task *task);
/* the squared distance from one to other in x and z alone. nothing in this
 * image calls it */
Float ruin_path_camera_flat_distance_squared(NJS_POINT3 *one,
                                             NJS_POINT3 *other);

#endif
