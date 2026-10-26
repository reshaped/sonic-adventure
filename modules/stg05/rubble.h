/* O ZAKO and O ZAKO_B, pieces of rubble, and O BPOLE, a pole. each is
 * solid, in a model its set entry picks. O ZAKO_B blows away once E-102
 * shoots its dynamite */
#ifndef STG05_RUBBLE_H
#define STG05_RUBBLE_H

#include "system/task.h"

/* the steps mountain_object_list names for O ZAKO, O ZAKO_B and O BPOLE. each
 * set entry's scale z, truncated, picks a model, and a scale out of range picks
 * the first. O ZAKO draws at 0.2 more than its scale x on x and z. on y it
 * draws at 0.2 more than its scale y. O ZAKO_B draws at 0.2 more than its scale
 * y on every axis. its scale x is the id of the targets that blow it away. the
 * pole draws at its model's own scale.
 *
 * unless the set editor holds it, each one's first step adds a copy to the
 * world. that step writes through null when every object of the world's pool is
 * lent */
void rubble_step(struct task *task);
void rubble_b_step(struct task *task);
void pole_step(struct task *task);

/* the pieces' six models, which O Erupt and the bursting rocks draw too */
extern NJS_OBJECT rubble_0_object;
extern NJS_OBJECT rubble_1_object;
extern NJS_OBJECT rubble_2_object;
extern NJS_OBJECT rubble_3_object;
extern NJS_OBJECT rubble_4_object;
extern NJS_OBJECT rubble_5_object;

#endif
