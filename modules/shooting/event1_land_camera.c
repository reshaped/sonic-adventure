/* the camera action event.c plays once event1_camera's has run, and its
 * motion */
#include <ninja.h>

#include "shooting/event1_land_camera.h"

extern NJS_CAMERA dat_0c9dee40;
extern NJS_MDATA4 dat_0c9e0050[];

static NJS_MOTION dat_0c9e0070 = {
#include "shooting/event1_land_camera/dat_0c9e0070.inc"
};
NJS_CACTION event1_land_camera = {
#include "shooting/event1_land_camera/event1_land_camera.inc"
};
