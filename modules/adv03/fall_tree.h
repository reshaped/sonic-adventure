/* O FALL TREE, a tree that falls once a player comes near */
#ifndef ADV03_FALL_TREE_H
#define ADV03_FALL_TREE_H

#include "system/task.h"

/* the step the object list names for O FALL TREE. it draws the tree on each
 * step. its first step stands the tree up, or lays it down once its
 * placement is used. while the set editor holds the task, the fall plays over
 * and over. once placement_try_leave finds the task out of range, the step
 * does nothing, and the task destroys itself on its next step */
void fall_tree_step(struct task *task);

#endif
