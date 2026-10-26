/* the gardens' fruit trees, the fruit they drop, and FLOWER */
#ifndef AL_MAIN_TREE_H
#define AL_MAIN_TREE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "system/task.h"

/* the tree's two collision shapes. garden_tree_init copies them into each
 * tree's record, then raises the templates' guard to 3. the first tree to
 * start keeps guard 0, and each later tree takes guard 3 */
extern struct collision_shape garden_tree_shapes[2];

/* the object list's step for a tree. task needs the work and motion that
 * TASK_WORK and TASK_MOTION allocate. it sets the tree's own step, draw and
 * destructor, and gives each branch a fruit of random growth at even odds.
 * a failed alife_entry_add goes unchecked, and garden_tree_step then reads
 * through a null entry */
void garden_tree_init(struct task *task);
/* the tree's step, which draws the tree as well */
void garden_tree_step(struct task *task);
/* draw the tree without stepping it */
void garden_tree_draw(struct task *task);
void garden_tree_destroy(struct task *task);
/* start a fruit of kind 1 to 10 falling from pos at velocity. a tree drops
 * kind 10. nothing checks the fruit's task, its record or its entry, so each
 * allocation must succeed */
void alife_fruit_create(NJS_POINT3 *pos, NJS_VECTOR *velocity, Sint8 kind);
/* the object list's step for FLOWER. it draws nothing and changes nothing
 * but its work's mode */
void garden_flower_step(struct task *task);

#endif
