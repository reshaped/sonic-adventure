/* the marker Zero's laser leaves on player 0. it keeps the lock, the point
 * Zero aims its throw at */
#ifndef B_ROBO_MARKER_H
#define B_ROBO_MARKER_H
#include <ninja.h>
#include <sg_xpt.h>

/* copy the lock into *lock. erobo_marker_create sets the lock to player 0's
 * position. from the marker's second step, the lock follows 5 above that
 * position. it holds still after erobo_marker_wind_up, and once the marker
 * lets go */
void erobo_marker_get_lock(NJS_POINT3 *lock);
/* ask the marker to hold the lock where it is, and to spin faster in red */
void erobo_marker_wind_up(void);
/* ask the marker to let go of the lock. it lets go by itself once
 * erobo_beam_reached is FALSE. once it has let go, it turns blue, waits 30
 * steps, then swells and shrinks away */
void erobo_marker_lose(void);
/* create a marker on player 0, and drop any request. when task_create
 * fails, nothing changes */
void erobo_marker_create(void);

#endif
