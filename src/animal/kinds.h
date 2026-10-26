/* make one of the current stage's kinds of animal */
#ifndef ANIMAL_KINDS_H
#define ANIMAL_KINDS_H
#include <sg_xpt.h>

/* the current stage must be STAGE_SHELTER or below, the last in the table
 * of each stage's kinds. neither animal_create_random nor animal_create_rare
 * checks */

/* make one of the stage's animals at x, y, z: its rare kind one time in
 * ten, and otherwise one of its four others */
void animal_create_random(int unused, Float x, Float y, Float z);
/* make the stage's rare animal at x, y, z */
void animal_create_rare(Float x, Float y, Float z);

#endif
