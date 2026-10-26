/* Super Sonic's aura, wake, spin balls and dash in the Perfect Chaos fight */
#ifndef B_CHAOS7_SUPER_H
#define B_CHAOS7_SUPER_H
#include <ninja.h>

#include "system/task.h"

/* Super Sonic's textures, which chaos7_act_pvms loads from the SUPERSONIC pvm.
 * his body's 16 come first, then the aura's, the wake's, the spin balls' and
 * the dash's. sonic.c draws him with them during his super actions */
extern NJS_TEXLIST super_sonic_texlist;

/* the first steps of the tasks sonic.c creates for Super Sonic, each with the
 * player's number in PLAYER_VARS(work)->player. the balls and the dash take
 * the place of effects.h's ball_follow_start, ball_shrink_start and
 * dash_start */
void super_aura_start(struct task *task); /* his aura, and his wake */
void super_ball_follow_start(struct task *task); /* a step behind the player */
void super_ball_shrink_start(struct task *task); /* left where the player was */
/* the dash is a streak from where the player's middle was to where it is.
 * without room on the heap for its copy of the streak, it ends at once */
void super_dash_start(struct task *task);

#endif
