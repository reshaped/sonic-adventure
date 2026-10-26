/* the deck's floors, for a thing that falls onto them */
#ifndef STG06_FLOOR_H
#define STG06_FLOOR_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* move pos by speed, and return TRUE if it lands. the floor it lands on is the
 * one under pos before the move. pos lands when it ends less than radius above
 * that floor, and then stands radius above it. the floor is at -198 for x
 * within (-850, -550) and z within (-454, 362). it is at -100 for x within
 * (0, 850), and nowhere else. ang is unread */
Bool sky_deck_floor_move(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                         Float radius);

#endif
