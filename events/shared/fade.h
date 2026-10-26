/* an event's fade: one color over the whole screen, which fades in or out */
#ifndef EVENT_FADE_H
#define EVENT_FADE_H
#include <sg_xpt.h>

#include "system/task.h"

/* a fade speed too small to show. from alpha 0, a fade at this speed stays
 * clear, and from 1, one at -EVENT_FADE_STILL stays opaque, until
 * event_fade_set_speed changes the speed */
#define EVENT_FADE_STILL 1e-32F

/* create a fade of the color from alpha. the alpha changes by speed each step:
 * above 0 it fades in, and below 0 out. the color's parts and the alpha run
 * from 0 to 1, and the alpha holds at either end. it draws at depth -1, the
 * draw priority njDrawPolygon2D takes as pri, until event_fade_set_depth. a
 * failed task_create is unchecked, and only a kill ends the fade */
struct task *event_fade_create(Float speed, Float alpha, Float red, Float green,
                               Float blue);
void event_fade_set_speed(struct task *task, Float speed);
void event_fade_set_alpha(struct task *task, Float alpha);
void event_fade_set_color(struct task *task, Float red, Float green,
                          Float blue);
void event_fade_set_depth(struct task *task, Float depth);

#endif
