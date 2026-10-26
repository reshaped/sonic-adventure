/* the Perfect Chaos modes that the last story's events ask for */
#ifndef B_CHAOS7_EVENT_MODES_H
#define B_CHAOS7_EVENT_MODES_H

#include <sg_xpt.h>

/* each asks the boss for the mode whose number ends its name, unless he is in
 * it already. mode 1 is BOSS_READY, and mode 16 BOSS_EVENT_RAY. a request
 * sets boss_next_mode, and keeps the boss's mode in boss_last_mode. when
 * chaos7_event_ray asks, it also sets the ray's length to length */
void chaos7_event_mode_1(void);
void chaos7_event_mode_2(void);
void chaos7_event_mode_13(void);
void chaos7_event_mode_14(void);
void chaos7_event_ray(Float length);

#endif
