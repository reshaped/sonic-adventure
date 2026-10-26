/* each act's camera collisions. an entry is NULL, or one of loader.h's
 * CAMERA_AREA_0 to CAMERA_AREA_5, which loader_read_stage_files fills from a
 * camera file */
#include "camera/layouts.h"
#include "camera/camera.h"
#include "stage/stage.h"

struct camera_collision *camera_layouts[STAGE_COUNT][STAGE_ACTS] = {
#include "camera/layouts/camera_layouts.inc"
};
