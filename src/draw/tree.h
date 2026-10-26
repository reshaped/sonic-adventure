/* model trees: drawn node by node, as each node's evalflags say */
#ifndef DRAW_TREE_H
#define DRAW_TREE_H
#include <ninja.h>

/* draw the tree at object in the current matrix, inside one njGetkmPointer
 * and njSetkmPointer, where draw_model calls the pair for each model. the
 * tree is object, each sibling after it, and the nodes under each. a node
 * without a child must set NJD_EVAL_BREAK: the walk follows child without
 * testing it for null */
void draw_tree(NJS_OBJECT *object);

#endif
