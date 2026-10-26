/* a collision box that no word in ADV00.BIN or 1ST_READ.BIN names. its
 * bytes are those of 1ST_READ.BIN's scene_changer_shape */
#include "adv00/unused_shape.h"
#include "collision/collision.h"

struct collision_shape station_square_unused_shape = {
#include "adv00/unused_shape/station_square_unused_shape.inc"
};
