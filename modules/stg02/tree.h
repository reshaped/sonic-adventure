/* O TREEM: a tree that plays its action over and over */
#ifndef STG02_TREE_H
#define STG02_TREE_H

#include "system/task.h"

/* the object list's step for O TREEM. task needs TASK_WORK's record. the tree
 * advances scl.x of its motion's frames a step. it stands still while the set
 * editor holds it */
void windy_tree_step(struct task *task);

#endif
