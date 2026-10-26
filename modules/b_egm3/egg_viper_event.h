/* the Egg Viper for EV0024, where Eggman flies off in it. the event's script
 * moves the boss and Eggman with these calls */
#ifndef B_EGM3_EGG_VIPER_EVENT_H
#define B_EGM3_EGG_VIPER_EVENT_H
#include <ninja.h>
#include <sg_xpt.h>

/* start the tasks of the boss, its needles and its segments. the boss
 * stands at x, y and z, turned ang_y about y, and waits there. open, shut
 * and fly each set its mode, and cut short the mode it was in */
void egg_viper_event_create(Float x, Float y, Float z, Angle ang_y);
/* kill the tasks create started, and Eggman's if he has one. call it once
 * for each create. it leaves egg_viper_task, egg_viper_needles and
 * egg_viper_segments pointing at the tasks it kills */
void egg_viper_event_kill(void);
/* the boss opens, then Eggman rises out of it */
void egg_viper_event_open(void);
/* Eggman talks once he is up, until hush. each call leaves him a message
 * that waits until he can act on it */
void egg_viper_event_talk(void);
void egg_viper_event_hush(void);
/* Eggman sinks back, then the boss shuts. the boss waits for him to sink,
 * so call it once the boss has opened */
void egg_viper_event_shut(void);
/* the boss flies off along a spline from where it is */
void egg_viper_event_fly(void);

#endif
