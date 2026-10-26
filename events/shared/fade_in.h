/* an event's fade in: one color that comes up over the whole screen */
#ifndef EVENT_FADE_IN_H
#define EVENT_FADE_IN_H
#include <sg_xpt.h>

/* cover the screen with the color, each part from 0 to 255. its alpha
 * rises from 0 by 0.01 a step to full, and holds there. nothing happens when
 * task_create fails. a second create leaves the first fade in running, out
 * of event_fade_in_kill's reach */
void event_fade_in_create(Float red, Float green, Float blue);
void event_fade_in_kill(void);

#endif
