/* the Egg Hornet near the ground: its nodes' places, its shadow, and dust
 * and clods where it lands */
#ifndef B_EGM1_GROUND_H
#define B_EGM1_GROUND_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* the dust ring's and the clods' modes. GROUND_ON keeps them going, and any
 * other mode ends them on their next step */
enum ground_mode { GROUND_ON, GROUND_OFF };

/* the last node egg_hornet_node_matrix poses */
#define POSE_LAST_NODE 15

/* copy node's matrix to matrix, from a pose of the boss's current action at
 * its motion's frame. the pose starts from Ninja's current matrix and stops
 * at POSE_LAST_NODE. it replaces the last pose. task's extra must be a
 * struct egg_hornet. returns TRUE, or FALSE, leaving matrix as it was, if
 * the pose didn't reach node */
Bool egg_hornet_node_matrix(struct task *task, Uint16 node, NJS_MATRIX *matrix);
/* draw the boss's shadow on the floor under its node 0, posing it as
 * egg_hornet_node_matrix does. work->pos ends as it was */
void egg_hornet_shadow(struct task *task);
/* start a ring of dust round center, which is copied. each step, a puff
 * rises from a random point on the ring, level with center.
 * dust_ring_set_mode ends the ring */
void dust_ring_create(NJS_POINT3 *center);
/* set the mode of the ring made last, which must not have ended */
void dust_ring_set_mode(enum ground_mode mode);
/* make four puffs of dust that rise from random points round origin, level
 * with it */
void landing_puffs(NJS_POINT3 *origin);
/* start clods of earth about pos, which is copied. every second step, a
 * clod flies out and up. it starts level with pos, in a random direction
 * from it. clods_set_mode ends them */
void clods_create(NJS_POINT3 *pos);
/* set the mode of the clods made last, which must not have ended */
void clods_set_mode(enum ground_mode mode);

#endif
