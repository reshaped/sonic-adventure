/* the Station Square garden: the stage's task */
#ifndef AL_GARDEN00_GARDEN_H
#define AL_GARDEN00_GARDEN_H

#include "event/script.h"
#include "system/task.h"

/* the table 1ST_READ.BIN's stage_scripts names for STAGE_ALIFE_SS. the garden's
 * generated data holds it in section D, so it isn't const */
extern struct script_row station_square_garden_script_table[];

/* the stage task's step, which 1ST_READ.BIN's stage_steps names for
 * STAGE_ALIFE_SS */
void station_square_garden_step(struct task *task);

#endif
