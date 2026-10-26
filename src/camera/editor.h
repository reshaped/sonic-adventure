/* the debug editor of an act's camera collisions */
#ifndef CAMERA_EDITOR_H
#define CAMERA_EDITOR_H
#include <sg_xpt.h>

struct camera_view;

/* create the editor's task. the editor stays closed until
 * camera_editor_request_open */
void camera_editor_create(void);
/* ask the editor's task to open the editor on its next step, if it's
 * closed. it returns -1 until the task first steps and while the editor
 * closes, and 0 otherwise. it must not be called once the task is
 * destroyed, whose freed work it would read. opening calls pad_disable and
 * pause_disable, and holds the camera at CAMERA_LEVEL_DEBUG. pad 0 then
 * drives the editor, and closes it from its system mode. closing releases
 * the camera, calls pad_enable and pause_enable, and calls
 * debug_set_flag(0) */
int camera_editor_request_open(void);
/* set view's pos and target to the edited collision's cam_pos and dir_pos,
 * and its angles to cam_ang_x and cam_ang_y. the camera calls it before
 * each step it runs at CAMERA_LEVEL_DEBUG. view's distance and steps stay
 * as they were */
void camera_step_editor(struct camera_view *view);

#endif
