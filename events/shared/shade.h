/* an event's shade: a polygon of one color over the screen, whose alpha
 * eases toward a target */
#ifndef EVENT_SHADE_H
#define EVENT_SHADE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the depth a shade draws at */
#define EVENT_SHADE_DEPTH (-1.1F)

/* a shade: a filled polygon of count points, each in color, an ARGB word,
 * at EVENT_SHADE_DEPTH. the task reads points each step, so they must
 * outlive it. returns the task, or null when none can be made */
struct task *event_shade_create(Uint32 color, NJS_POINT2 *points, int count);
/* ease the shade's alpha toward alpha. each step it closes 1/ease of the
 * gap, truncated. the truncation stops a rising alpha up to ease - 1 short.
 * ease 0 or below sets alpha at once */
void event_shade_fade(struct task *task, int alpha, int ease);

#endif
