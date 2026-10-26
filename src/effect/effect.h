/* the barriers and the invincibility a player wears */
#ifndef EFFECT_EFFECT_H
#define EFFECT_EFFECT_H

struct task;

/* the first steps of the tasks for a player's magnetic barrier, barrier and
 * invincibility. player_give_magnetic_barrier and the like make each task,
 * with the player's slot in its work's vars. a barrier ends once player 0's
 * record loses it or the player's task is gone. invincibility keeps
 * PLAYER_INVINCIBLE set in player 0's record for 1261 steps, or until the
 * player's task is gone, then clears it */
void magnetic_barrier_start(struct task *task);
void barrier_start(struct task *task);
void invincibility_start(struct task *task);

#endif
