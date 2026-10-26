/* path steps: drawing a path, putting a player on it, or handing over the
 * camera near a stretch of it. each kills its task once the stage or act
 * changes */
#ifndef PATH_STEPS_H
#define PATH_STEPS_H

#include "system/task.h"

/* draw lines along the path while its first point is near the camera */
void path_draw_step(struct task *task);
/* put a player that comes near one of the path's points on the path.
 * path_join_step joins the player to it where the player is nearest.
 * path_hang_step hangs the player from that point, and path_ledge_step has
 * the player hold on to a ledge there */
void path_join_step(struct task *task);
void path_hang_step(struct task *task);
void path_ledge_step(struct task *task);
/* path_camera_n hands the camera to the stretch path_cameras[n] gives, a
 * run of the path's points, when a player comes near it. camera_step_path
 * then follows the whole path */
void path_camera_0(struct task *task);
void path_camera_1(struct task *task);
void path_camera_2(struct task *task);
void path_camera_3(struct task *task);
void path_camera_4(struct task *task);
void path_camera_5(struct task *task);
void path_camera_6(struct task *task);
void path_camera_7(struct task *task);
void path_camera_8(struct task *task);
void path_camera_9(struct task *task);
void path_camera_10(struct task *task);
void path_camera_11(struct task *task);
void path_camera_12(struct task *task);
void path_camera_13(struct task *task);

#endif
