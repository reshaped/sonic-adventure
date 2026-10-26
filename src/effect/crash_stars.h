/* crash stars: the stars that burst from a player as it crashes into a
 * wall */
#ifndef EFFECT_CRASH_STARS_H
#define EFFECT_CRASH_STARS_H
#include <sg_xpt.h>

struct task;

struct crash_stars_vars {
    /* the slot of the player it bursts from, which player_crash_burst sets */
    Uint8 player;
};

#define CRASH_STARS_VARS(work) ((struct crash_stars_vars *)&(work)->vars)

/* the step of the task player_crash_burst makes. six stars burst from a
 * point 5 along the player's up axis, and the task ends */
void crash_stars_step(struct task *task);

#endif
