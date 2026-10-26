/* the tornado's gauge: a bar at the bottom of the screen that fills as the
 * chase starts, then follows the tornado's lives down */
#ifndef SHOOTING_GAUGE_H
#define SHOOTING_GAUGE_H
#include <sg_xpt.h>

struct task;

/* create a gauge units long, with every life left. units must fit a
 * Sint8. the work's pos takes x and y, or 288 and 420 where x is 0 or less.
 * the draw never reads pos: the bar always stands at the bottom. only one
 * gauge may run at a time, since every gauge shares gauge.c's state */
void gauge_create(int x, int y, int units);
/* the same, as a child of parent */
void gauge_create_child(int x, int y, int units, struct task *parent);

#endif
