/* O TREE, a tree that burns in PAST_FIRE */
#ifndef ADV03_TREE_H
#define ADV03_TREE_H

#include "system/task.h"

/* the step the object list names for O TREE. it draws the tree on each
 * step, burning in PAST_FIRE. once placement_try_leave finds the task out of
 * range, the step does nothing, and the task destroys itself on its next step
 */
void tree_step(struct task *task);

#endif
