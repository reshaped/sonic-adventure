/* the Mystic Ruins garden: the stage's task */
#ifndef AL_GARDEN02_GARDEN_H
#define AL_GARDEN02_GARDEN_H

#include "event/script.h"
#include "system/task.h"

/* the table 1ST_READ.BIN's stage_scripts names for STAGE_ALIFE_MR. the garden's
 * generated data holds it in section D, so it isn't const */
extern struct script_row mystic_ruins_garden_script_table[];

/* the stage task's step, which 1ST_READ.BIN's stage_steps names for
 * STAGE_ALIFE_MR */
void mystic_ruins_garden_step(struct task *task);

#endif
