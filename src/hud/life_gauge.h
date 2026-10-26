/* the life gauge: a bar at the top right of the screen that fills as a
 * fight starts, then follows the boss's health down */
#ifndef HUD_LIFE_GAUGE_H
#define HUD_LIFE_GAUGE_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* CHAOS_LIFEGAUGE's textures, which each stage with a gauge loads */
extern NJS_TEXLIST chaos_lifegauge_texlist;
/* life_gauge_health is the boss's health in units, which the boss sets and
 * the bar follows. life_gauge_whole is its whole part, which the gauge sets
 * at each step and nothing reads */
extern Sint8 life_gauge_whole;
extern Float life_gauge_health;

/* create a gauge for a boss with units of health, at full health. units
 * must fit a Sint8. the work's pos takes x and y, or 600 and 48 where x is
 * 0 or less. the draw never reads pos: the bar always stands at the top
 * right. only one gauge may run at a time, since every gauge shares this
 * file's state */
void life_gauge_create(int x, int y, int units);
/* the same, as a child of parent */
void life_gauge_create_child(int x, int y, int units, struct task *parent);

#endif
