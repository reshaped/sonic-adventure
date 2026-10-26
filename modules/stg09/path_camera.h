/* Casinopolis's path cameras, which run the camera along a curve through a
 * path's points */
#ifndef STG09_PATH_CAMERA_H
#define STG09_PATH_CAMERA_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the steps of the tasks path_create_tasks makes for the stage's paths.
 * steps 0 to 6 share one set of settings, and step 7 has another. act 1's
 * paths name steps 0 to 6, and no path names step 7 */
void casino_path_camera_0(struct task *task);
void casino_path_camera_1(struct task *task);
void casino_path_camera_2(struct task *task);
void casino_path_camera_3(struct task *task);
void casino_path_camera_4(struct task *task);
void casino_path_camera_5(struct task *task);
void casino_path_camera_6(struct task *task);
void casino_path_camera_7(struct task *task);
/* the squared distance from one to other in x and z alone. nothing in this
 * image calls it */
Float casino_path_camera_flat_distance_squared(NJS_POINT3 *one,
                                               NJS_POINT3 *other);

#endif
