/* Sonic as a player, and the board he rides */
#ifndef PLAYER_SONIC_H
#define PLAYER_SONIC_H

struct task;

/* the step of a player's task when the player is Sonic. make the task with
 * TASK_MOTION, TASK_WORK and TASK_FORCE, as every player's is, and the
 * player's number in its vars */
void sonic_step(struct task *task);
/* the first step of the board a player rides in a board stage, whose
 * module holds the board's model. make the task with TASK_WORK and
 * TASK_MOTION, and the rider's player number in its vars */
void sonic_board_start(struct task *task);

#endif
