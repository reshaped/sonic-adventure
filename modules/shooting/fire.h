/* the sky chase's fire: FIRE_L, one flame, and pom, six on the tornado */
#ifndef SHOOTING_FIRE_H
#define SHOOTING_FIRE_H

#include "system/task.h"

/* FIRE_L: a flame at its parent's pos, which burns for eight steps. the
 * task needs a work and a parent. nothing in this image starts one */
void fire_l_step(struct task *task);
/* draw FIRE_L's frame vars.l[2], 0 to 7, at work's pos. vars.l[2] then
 * moves on to the next of the eight, round and round. nothing in this image
 * calls it */
void fire_l_loop(struct task_work *work);
/* pom: six flames at points on its parent, the tornado, which turn as it
 * turns, for eight steps. the task and its parent need a work */
void pom_step(struct task *task);

#endif
