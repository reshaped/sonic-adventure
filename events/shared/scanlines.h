/* an event's scan lines: rows of color across the screen that scroll */
#ifndef EVENT_SCANLINES_H
#define EVENT_SCANLINES_H
#include <sg_xpt.h>

/* draw a line of the color across the screen every spacing + 1 rows, until
 * event_scanlines_kill. a second create leaves the first running, out of
 * event_scanlines_kill's reach. when no task can be made, it reads through
 * null.
 *
 * the lines move down a row each step, and each frame drawn while paused.
 * the top line's row climbs to spacing - 1 and then wraps to 0. the wrap
 * moves every line down two. the row carries over from the last lines
 * made. lines whose spacing is at or below that row never wrap, and move
 * down off the screen */
void event_scanlines_create(Uint8 alpha, Uint8 red, Uint8 green, Uint8 blue,
                            int spacing);
void event_scanlines_kill(void);

#endif
