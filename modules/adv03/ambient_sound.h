/* O SOUND, a tone that plays at the object's position */
#ifndef ADV03_AMBIENT_SOUND_H
#define ADV03_AMBIENT_SOUND_H

#include "system/task.h"

/* the step the object list names for O SOUND. ang.x picks one of seven
 * tones, clamped to the first and the last. scl.y, truncated, is the tone's
 * volume before its falloff by distance from the camera. scl.x, which the set
 * editor labels RADIUS, goes unread. each step renews the tone, so it plays
 * while the object is in range, as placement_try_leave tests it. while the
 * set editor is open, the step also draws a sphere at the object */
void ambient_sound_step(struct task *task);

#endif
