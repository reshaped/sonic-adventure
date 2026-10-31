/* the course Sonic races Tails on in the first act: its points and their
 * count. 1ST_READ.BIN's race_lines and race_counts name the two */
#include <sg_xpt.h>

#include "player/race.h"
#include "stg06/race_course.h"

const struct race_point sky_deck_race_points[111] = {
#include "stg06/race_course/sky_deck_race_points.inc"
};
const Sint32 sky_deck_race_count = {
#include "stg06/race_course/sky_deck_race_count.inc"
};
