/* the tornado's reticle, and the locks it puts on targets */
#ifndef SHOOTING_RETICLE_H
#define SHOOTING_RETICLE_H
#include <sg_xpt.h>

#include "system/task.h"

/* the task a bullet flies for, which reticle_step sets. the candidates are
 * the first LOCKS targets near the reticle with no lock, within 3000 of the
 * tornado. reticle_target is the candidate nearest the tornado. with none,
 * it is the reticle's child, which sits 1000 ahead of the camera */
extern struct task *reticle_target;

/* the most locks at once */
#define LOCKS 6

/* the collisions lock_take found under the reticle, and each lock's
 * target, or null for a free lock. only reticle.c reads lock_found. SHC
 * lays out bss in the order of each name's first declaration. so lock_found
 * is declared here, ahead of lock_targets */
extern struct collision_work *lock_found[LOCKS];
extern struct task *lock_targets[LOCKS];

/* the reticle's step. the task needs a work. each step moves the reticle
 * by player 0's stick and sets reticle_target. it takes a lock while
 * tornado_count_charge returns TRUE. it stops while an event or the score
 * plays */
void reticle_step(struct task *task);
/* forget every lock's target. the locks' tasks live on */
void locks_clear(void);
/* take the lock off task, if it has one. the lock is destroyed, and task
 * gets its own destructor back */
void lock_release(struct task *task);
/* TRUE if a lock holds target, else FALSE */
Bool lock_holds(struct task *target);

#endif
