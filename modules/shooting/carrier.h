/* the egg carrier, which the tornado fights at act 2's end */
#ifndef SHOOTING_CARRIER_H
#define SHOOTING_CARRIER_H

#include <sg_xpt.h>

#include "system/task.h"

/* shared by the carrier, its core and its blast */
struct carrier_vars {
    Float spin; /* the propellers' action frame */
    Uint8 unk_04;
    Sint8 flash; /* steps left of a hit's flash */
    Uint8 down; /* set once it's going down */
    Uint8 unk_07;
    Float hatch; /* how far the hatch is open, or the blast's action frame */
};

#define CARRIER_VARS(work) ((struct carrier_vars *)&(work)->vars)

/* the stage task sets it to 1 as the tornado reaches the carrier. the
 * carrier clears it as it takes it, and sets -1 once it has gone */
extern int carrier_arrived;
/* set while the carrier waits, opens or shuts. clear before the fight and
 * while the carrier fires */
extern Bool hodai_may_fire;

/* the carrier's step, which the stage task starts as each act starts */
void carrier_step(struct task *task);

#endif
