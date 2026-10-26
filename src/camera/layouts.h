/* each act's camera collisions */
#ifndef CAMERA_LAYOUTS_H
#define CAMERA_LAYOUTS_H
#include "camera/camera.h"
#include "stage/stage.h"

/* each act's camera collisions, by stage and act, or NULL for an act with
 * none. the first word of an entry's element 0 is a Sint32 count of the
 * act's collisions, which start at element 1 */
extern struct camera_collision *camera_layouts[STAGE_COUNT][STAGE_ACTS];

#endif
