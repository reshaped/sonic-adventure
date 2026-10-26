/* EV0050.PRS's Egg Walker, with Eggman in its seat. it falls to the ground
 * and lands */
#ifndef EV0050_WALKER_H
#define EV0050_WALKER_H
#include <sg_xpt.h>

/* the walker's modes. its first step, in WALKER_START, finds the ground and
 * sets WALKER_STILL. in WALKER_FALL it falls to the ground. it then plays
 * its landing in WALKER_LAND, and passes through WALKER_LANDED to
 * WALKER_STILL */
enum walker_mode {
    WALKER_START,
    WALKER_FALL,
    WALKER_LAND,
    WALKER_LANDED,
    WALKER_STILL,
    WALKER_MODE_5
};

/* create the walker at x, y and z with those angles. the calls below act
 * on the last walker created. when task_create fails, they act on the one
 * created before, or on none. the walker's first step finds the ground up
 * to 1000 below it, or kills the walker. a walker that has killed itself
 * leaves the calls below a dangling pointer */
void ev0050_walker_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                          Angle ang_z);
/* set the walker's mode, as WALKER_FALL to drop it to the ground. the
 * walker must be alive and past its first step, which finds the ground.
 * unlike the two calls below, this one doesn't test for a walker */
void ev0050_walker_set_mode(Sint8 mode);
void ev0050_walker_kill(void);
/* give the walker a shadow of size, drawn at height above the ground its
 * first step found. a size of 0 or less removes the shadow. with no walker,
 * it does nothing */
void ev0050_walker_set_shadow(Float size, Float height);

#endif
