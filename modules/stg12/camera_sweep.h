/* the camera's sweep around E-105 once it is beaten, and the rays of light
 * it gives off */
#ifndef STG12_CAMERA_SWEEP_H
#define STG12_CAMERA_SWEEP_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* end the sweep, or another camera_override, with camera_restore. task is
 * not read: E-105 passes its own */
void shelter_camera_restore(struct task *task);
/* empty, with no caller in this image */
void fun_0c9166a6(void);
/* swing the camera about target, looking at it from 100 units out and 20
 * degrees above. it starts 120 degrees before ang and turns about 240 degrees
 * over steps steps. then its turn slows by 1 angle unit a step until it stops.
 * target is copied. steps must not be 0 */
void shelter_camera_sweep_start(NJS_POINT3 *target, Angle ang, int steps);
/* a ray of light from pos, length units long. pos is kept, not copied, and
 * must outlive the ray */
void shelter_ray_create(NJS_POINT3 *pos, Float length);

#endif
