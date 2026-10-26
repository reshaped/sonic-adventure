/* Emerald Coast's path cameras, which run the camera along a curve through a
 * path's points */
#ifndef STG01_PATH_CAMERA_H
#define STG01_PATH_CAMERA_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the steps of the tasks path_create_tasks makes for the stage's paths. each
 * runs its task with one of three settings. beach_act0_paths holds the paths
 * with steps 0 and 1, and beach_act1_paths those with steps 2 to 5. two
 * paths take step 7, and neither list holds them. no path takes step 6 */
void beach_path_camera_0(struct task *task);
void beach_path_camera_1(struct task *task);
void beach_path_camera_2(struct task *task);
void beach_path_camera_3(struct task *task);
void beach_path_camera_4(struct task *task);
void beach_path_camera_5(struct task *task);
void beach_path_camera_6(struct task *task);
void beach_path_camera_7(struct task *task);
/* the squared distance from one to other in x and z alone. nothing in this
 * image calls it */
Float beach_path_camera_flat_distance_squared(NJS_POINT3 *one,
                                              NJS_POINT3 *other);

#endif
