/* the adjust modes, and camera_mode_table, which gives the camera its modes
 * and adjust modes */
#include "camera/camera.h"

/* the adjust modes, by CAMERA_ADJUST_ number */
static struct camera_adjust_mode camera_adjust_list[CAMERA_ADJUSTS] = {
#include "camera/mode_table/camera_adjust_list.inc"
};
struct camera_mode_table camera_mode_table = {
#include "camera/mode_table/camera_mode_table.inc"
};
