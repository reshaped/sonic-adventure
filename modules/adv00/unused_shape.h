/* a collision box that no word in ADV00.BIN or 1ST_READ.BIN names */
#ifndef ADV00_UNUSED_SHAPE_H
#define ADV00_UNUSED_SHAPE_H

#include "collision/collision.h"

/* a box with the bytes of 1ST_READ.BIN's scene_changer_shape. no code
 * reads it: scene_change.c, this image's scene changer, makes no collision */
extern struct collision_shape station_square_unused_shape;

#endif
