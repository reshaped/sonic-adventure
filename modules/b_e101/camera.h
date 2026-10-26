/* the fight's camera: held at a point watching player 0 and the boss, or
 * sweeping round a point */
#ifndef B_E101_CAMERA_H
#define B_E101_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

/* hold the camera at pos. it looks halfway from player 0 to target, but
 * never more than 40 units from player 0. pos is copied. target is kept and
 * read each step, so it must outlive the hold */
void e101_camera_hold_start(NJS_POINT3 *target, NJS_POINT3 *pos);
/* swing the camera about target, looking at it from 30 degrees above. the
 * camera stays 50 units plus the reach from target. it starts 50 degrees
 * before ang and turns about 100 degrees over steps steps, then stops
 * moving. the reach starts at 0. target is copied. steps must not be 0 */
void e101_camera_sweep_start(NJS_POINT3 *target, Angle ang, int steps);
/* set the reach: how much farther than 50 units the sweeping camera stays
 * from its target */
void e101_camera_sweep_set_reach(Float reach);

#endif
