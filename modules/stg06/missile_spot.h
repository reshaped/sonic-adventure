/* the spot an aircraft's missile flies at */
#ifndef STG06_MISSILE_SPOT_H
#define STG06_MISSILE_SPOT_H

#include "system/task.h"

/* in the spot's work->tick, which its creator sets */
#define SPOT_FOLLOW 0x1 /* the spot follows player 0 */
#define SPOT_TURN 0x2 /* the spot is drawn turned to its ang.y */
#define SPOT_SCALE 0x4 /* the spot is drawn scaled by its scl */

/* the spot's step, which draws the spot just above its pos from its second step
 * on. with SPOT_FOLLOW, the spot follows player 0. each step it moves a quarter
 * of the way there along x and z. its y then takes the height of the floor
 * below player 0, if there is one. over a floor, SPOT_TURN also sets its ang.x
 * and ang.z to the floor's slope. the draw doesn't read them */
void missile_spot_step(struct task *task);

#endif
