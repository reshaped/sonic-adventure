/* an event's lightning: sparks that flicker about a point */
#ifndef EVENT_LIGHTNING_H
#define EVENT_LIGHTNING_H
#include <sg_xpt.h>

/* make lightning of count sparks over x, y and z, placed by its first
 * step. each spark sits within 2.5 of x and of z, and 8 to 13 above y */
void event_lightning_create(Float x, Float y, Float z, int count);
/* kill the lightning made last, with its sparks, on its next step. it does
 * nothing unless a create has made lightning since the last kill */
void event_lightning_kill(void);

#endif
