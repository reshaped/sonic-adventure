/* Chaos 4's cameras, and the dome round the swamp. before the fight, one
 * camera climbs while he is under the swamp, and another circles him. the
 * fight's camera keeps him in view past player 0 */
#ifndef B_CHAOS4_CAMERA_H
#define B_CHAOS4_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

/* start the fight's camera, behind player 0. pos goes unread */
void chaos4_camera_start(NJS_POINT3 *pos);
/* make the fight's camera aim past player 0 at pos in place of Chaos 4,
 * the next step it aims, and then forget pos. it doesn't aim while it
 * searches for room by a wall. pos must last until it aims */
void chaos4_camera_look_at(NJS_POINT3 *pos);
/* start a camera that looks at Chaos 4 from a fixed place, and climbs
 * while he is below the swamp's surface */
void chaos4_camera_rise_start(void);
/* start a camera that circles Chaos 4 at his height, looking at him */
void chaos4_camera_orbit_start(void);

/* draw the dome round the swamp, on a frame's first step only */
void chaos4_dome_draw(void);

#endif
