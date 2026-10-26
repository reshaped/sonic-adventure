/* the Egg Walker's parts: twenty points on its posed model and its turret.
 * each has a ball in the boss's collision record */
#ifndef B_EGM2_PARTS_H
#define B_EGM2_PARTS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* give the boss's task a COLLISION_ENEMY record with a ball for each part */
void egg_walker_shapes_init(struct task *task);
/* place each part, its up and its ball by the boss's pose. keep the matrix
 * of the pose's node 0. egg_walker_shapes_init must have run */
void egg_walker_parts_update(struct task *task);
/* node 0's matrix as the last egg_walker_parts_update found it */
NJS_MATRIX *egg_walker_body_matrix(void);
/* give each ball of the eight foot parts from PART_FEET its full attack
 * while above player 0. otherwise give it none. a foot hurts the player only
 * from above. work is the boss's */
void egg_walker_feet_attack(struct task_work *work);
/* part 0 to 19's place in the world, as the last egg_walker_parts_update
 * left it. part goes unchecked */
NJS_POINT3 *egg_walker_part_pos(int part);
/* part PART_BODY is the body, and part PART_COCKPIT the cockpit. leg i's
 * point is part PART_LEGS + i, and its foot part PART_FEET + i */
#define PART_BODY 0
#define PART_COCKPIT 1
#define PART_LEGS 2
#define PART_FEET 6

#endif
