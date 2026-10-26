/* O TREE1, a standing tree, and O TREE2, a fallen one. each is solid by a
 * capsule. the set entry scales both the tree and its capsule */
#ifndef STG05_TREES_H
#define STG05_TREES_H

#include "system/task.h"

/* the steps mountain_object_list names for O TREE1 and O TREE2 */
void mountain_tree_1_step(struct task *task);
void mountain_tree_2_step(struct task *task);

#endif
