/* the charger, a hovering can on a disc. it turns to face player 0, then
 * rams the spot where player 0 stood */
#ifndef ENEMY_CHARGER_H
#define ENEMY_CHARGER_H

struct task;

/* a task's first step. it sets the charger up, then hands the task to the
 * charger's own step and destructor */
void charger_start(struct task *task);

#endif
