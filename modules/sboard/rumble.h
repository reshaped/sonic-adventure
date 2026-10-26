/* the pad's rumble for what moves near the camera on the sand hill */
#ifndef SBOARD_RUMBLE_H
#define SBOARD_RUMBLE_H
#include <ninja.h>

/* vibrate pad 0 if pos is within range of the camera. the call asks for
 * power 5 and freq 15, and vibration_start clamps the power to 4. a NULL
 * pos, or a NULL camera_work, vibrates nothing */
void rumble_near_camera(const NJS_POINT3 *pos, Float range);

#endif
