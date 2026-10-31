/* the Egg Carrier's garden: the stage's task */
#ifndef AL_GARDEN01_GARDEN_H
#define AL_GARDEN01_GARDEN_H

#include "event/script.h"
#include "system/task.h"

/* the table 1ST_READ.BIN's stage_scripts names for STAGE_ALIFE_EG. the garden's
 * generated data holds it in section D, so it isn't const */
extern struct script_row egg_carrier_garden_script_table[];

/* the stage task's step, which 1ST_READ.BIN's stage_steps names for
 * STAGE_ALIFE_EG */
void egg_carrier_garden_step(struct task *task);

#endif
