/* the camera's sweep around an E-100 series robot as it goes down */
#ifndef STG05_CAMERA_SWEEP_H
#define STG05_CAMERA_SWEEP_H
#include <ninja.h>
#include <sg_xpt.h>

/* swing the camera about target, looking at it from 30 degrees above. the
 * camera stays 50 units plus the reach from target. it starts 50 degrees
 * before ang and turns about 100 degrees over steps steps, then stops moving.
 * the reach starts at 0. target is copied. steps must not be 0 */
void camera_sweep_start(NJS_POINT3 *target, Angle ang, int steps);
/* set the reach: how much farther than 50 units the sweeping camera stays
 * from its target */
void camera_sweep_set_reach(Float reach);

#endif
