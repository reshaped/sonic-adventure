/* path cameras that run the camera along a stretch of a Final Egg path */
#ifndef STG10_PATH_CAMERA_H
#define STG10_PATH_CAMERA_H

#include "system/task.h"

/* steps a struct path can name for the task path_create_tasks makes for it.
 * each runs once, and gives the task the stretch of its path that it runs
 * the camera along. steps 0 and 1 give the first stretch, and steps 2 and 3
 * the second. the paths on act 0's list, which 1ST_READ.BIN's path_lists
 * names, name steps 2 and 3. nothing in this image or 1ST_READ.BIN points at
 * the struct paths that name steps 0 and 1 */
void final_path_camera_0(struct task *task);
void final_path_camera_1(struct task *task);
void final_path_camera_2(struct task *task);
void final_path_camera_3(struct task *task);

#endif
