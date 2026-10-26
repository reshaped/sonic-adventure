/* the Egg Hornet's shots: volleys of missiles that curve toward a target */
#ifndef B_EGM1_SHOTS_H
#define B_EGM1_SHOTS_H
#include <ninja.h>
#include <sg_xpt.h>

struct task_work;

/* the most shots a volley fires */
#define MOST_SHOTS 6

struct volley {
    struct task_work *owner; /* the boss's work, which places the launchers */
    NJS_POINT3 *target;
    Uint16 interval; /* the steps between shots */
    /* shot n, from 0, fires after the volley's first n * interval steps. it
     * follows target for its first n * (follow_interval - interval) steps.
     * so it stops following about n * follow_interval steps into the
     * volley. follow_interval must be at least interval */
    Uint16 follow_interval;
    Uint16 count; /* from 1 to MOST_SHOTS */
};

/* fire the volley's shots over the steps to come. the volley is copied.
 * owner must last until the last shot is fired, and target until the last
 * shot stops following it */
void egg_hornet_volley_create(struct volley *volley);

#endif
