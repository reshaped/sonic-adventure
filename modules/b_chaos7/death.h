/* the three effects that play as Perfect Chaos dies */
#ifndef B_CHAOS7_DEATH_H
#define B_CHAOS7_DEATH_H

#include <ninja.h>

#include "system/task.h"

/* CHAOS7_0DEAD_PTCHG's textures, eight for each effect in turn, which
 * chaos7_fight_pvms loads */
extern NJS_TEXLIST chaos7_death_texlist;

/* the effects' first steps, for a task with a work. each draws its effect at
 * the work's pos. the first two fade in as they play their action once, then
 * hold its last frame. the third turns about y as it plays its action twice.
 * it fades in and out each time, then kills its task */
void chaos7_death_1_start(struct task *task);
void chaos7_death_2_start(struct task *task);
void chaos7_death_3_start(struct task *task);

#endif
