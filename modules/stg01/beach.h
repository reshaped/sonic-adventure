/* Emerald Coast's stage task and sky task. the stage task starts each act,
 * and the sky task draws each act's sky and lens flares */
#ifndef STG01_BEACH_H
#define STG01_BEACH_H

#include "system/task.h"

/* the stage task's modes. in BEACH_INIT, the task starts the act the stage
 * is in, and takes that act's mode. BEACH_END ends act 0's orca control or
 * clears act 2, but no code sets it */
enum beach_mode {
    BEACH_INIT,
    BEACH_ACT0, /* until player 0 reaches act 0's exit, and the stage moves
                 * to act 1 */
    BEACH_ACT1,
    BEACH_ACT2,
    BEACH_END
};

/* task steps that stage_steps and sky_steps name for the stage */
void beach_step(struct task *task);
void beach_sky_step(struct task *task);

#endif
