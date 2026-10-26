/* O KNUTREE, a solid tree with a near model and a far one */
#ifndef ADV03_KNUCKLES_TREE_H
#define ADV03_KNUCKLES_TREE_H

#include "system/task.h"

/* the step the object list names for O KNUTREE. its first step gives the
 * tree its trunk's collision. each step draws the tree and registers the
 * collision, but while the set editor holds the task it only draws the tree.
 * the task has no draw, so the tree doesn't show under the pause menu */
void knuckles_tree_step(struct task *task);

#endif
