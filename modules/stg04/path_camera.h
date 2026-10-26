/* Speed Highway's path cameras, which run the camera along a curve through
 * a path's points */
#ifndef STG04_PATH_CAMERA_H
#define STG04_PATH_CAMERA_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the steps of the tasks path_create_tasks makes for the stage's paths. each
 * takes settings of its own. the last five paths of highway_act0_paths name
 * the five steps in turn */
void highway_path_camera_0(struct task *task);
void highway_path_camera_1(struct task *task);
void highway_path_camera_2(struct task *task);
void highway_path_camera_3(struct task *task);
void highway_path_camera_4(struct task *task);
/* the squared distance from one to other in x and z alone. nothing in this
 * image calls it */
Float highway_path_camera_flat_distance_squared(NJS_POINT3 *one,
                                                NJS_POINT3 *other);

#endif
