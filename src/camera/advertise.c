/* the collisions camera_create_advertise gives the menus' camera.
 * camera_advertise_layout holds one collision, the first of
 * advertise_collisions. the array has room for CAMERA_COLLISIONS_MAX, the
 * most collisions the camera editor lets a layout hold */
#include "camera/camera.h"

static struct camera_collision advertise_collisions[CAMERA_COLLISIONS_MAX] = {
#include "camera/advertise/advertise_collisions.inc"
};
struct camera_layout camera_advertise_layout = {
#include "camera/advertise/camera_advertise_layout.inc"
};
