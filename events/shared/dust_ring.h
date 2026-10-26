/* an event's ring of dust: glowing puffs in a ring about a point */
#ifndef EVENT_DUST_RING_H
#define EVENT_DUST_RING_H
#include <sg_xpt.h>

/* what the ring's step does, by work->mode */
enum event_dust_ring_mode {
    EVENT_DUST_RING_WAIT, /* nothing */
    EVENT_DUST_RING_ON, /* it puffs a ring every 16 steps */
    EVENT_DUST_RING_OFF /* nothing */
};

/* create a ring at x, y and z, waiting. the calls act on the last ring
 * created, and do nothing without one */
void event_dust_ring_create(Float x, Float y, Float z);
void event_dust_ring_set_mode(enum event_dust_ring_mode mode);
/* kill the ring on its next step, and forget it */
void event_dust_ring_kill(void);

#endif
