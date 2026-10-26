/* an event's orbit: up to seven tasks that circle a point, and scatter from
 * it on event_orbit_scatter. each can carry a sprite.
 * event_orbit_add_sprite, event_orbit_scatter and event_orbit_kill reach the
 * last orbit made */
#ifndef EVENT_ORBIT_H
#define EVENT_ORBIT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

#define EVENT_ORBIT_TASKS 7

/* put task_1 to task_7, any of which can be null, on a circle around x, y
 * and z. the circle is level, of the given radius, and turns by speed each
 * step. the tasks lie a seventh of a turn apart. the orbit sets their places
 * from its third step */
void event_orbit_create(Float x, Float y, Float z, Float radius, Angle speed,
                        struct task *task_1, struct task *task_2,
                        struct task *task_3, struct task *task_4,
                        struct task *task_5, struct task *task_6,
                        struct task *task_7);
/* kill the orbit and the sprites in slots 0 to 6. it does nothing if no
 * orbit was made since the last kill */
void event_orbit_kill(void);
/* put a sprite of the texlist on task number, 1 to 7, unless that task is
 * null. it goes in sprite slot number - 1, at scale on both axes and in the
 * color given. it needs an orbit made since the last event_orbit_kill */
void event_orbit_add_sprite(Sint32 number, NJS_TEXLIST *texlist, Float scale,
                            Float alpha, Float red, Float green, Float blue);
/* stop the circle, and fly each task straight out from the point, level,
 * at speed a step. it needs an orbit made since the last event_orbit_kill */
void event_orbit_scatter(Float speed);

#endif
