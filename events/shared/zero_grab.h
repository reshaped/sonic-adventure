/* an event's request that Zero throw its hand at a point */
#ifndef EVENT_ZERO_GRAB_H
#define EVENT_ZERO_GRAB_H
#include <sg_xpt.h>

/* ask the event's Zero to throw its hand at x, y, z. that Zero is the
 * player event_create_player makes with zero_event_init as its first
 * step. a call before that step is lost, since zero_event_init clears any
 * request. the throw starts on Zero's second step after the call. returns
 * TRUE, or FALSE when a throw was already asked for since zero_event_init
 * last ran */
Bool event_zero_grab(Float x, Float y, Float z);

#endif
