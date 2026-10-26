/* an event's screen noise: a black screen that flickers into white noise,
 * and then both fade. each event_noise_start or event_noise_create makes a
 * new noise task. event_noise_show and event_noise_kill reach only the last
 * one made */
#ifndef EVENT_NOISE_H
#define EVENT_NOISE_H
#include <sg_xpt.h>

/* start a noise: opaque black for black_steps, then flickering between the
 * black and the noise for flicker_steps. the noise then shows over the black
 * for fade_steps. both start at an alpha of 240. each fade step but the last
 * takes 112 / fade_steps, rounded down, plus 1 from that alpha. the noise
 * then holds for hold_steps, and clears. fade_steps must not be 0 */
void event_noise_start(int black_steps, int flicker_steps, int fade_steps,
                       int hold_steps);
/* make a noise at an alpha of 0, unseen until event_noise_show */
void event_noise_create(void);
/* show the last noise made, over its black, at alpha for steps steps. it
 * then clears. it needs a noise made since the last event_noise_kill */
void event_noise_show(int steps, Uint8 alpha);
/* kill the last noise made, unless it's killed already */
void event_noise_kill(void);

#endif
