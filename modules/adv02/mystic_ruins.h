/* the Mystic Ruins' field and sky tasks. the field task creates each act's
 * tasks and its music, and the sky task sets each act's light and fog and draws
 * its sky */
#ifndef ADV02_MYSTIC_RUINS_H
#define ADV02_MYSTIC_RUINS_H

#include "system/task.h"

/* the field's acts, as stage_get_stage_act numbers them and the stage select
 * names them */
enum {
    MYSTIC_RUINS_STATION,
    MYSTIC_RUINS_ISLAND,
    MYSTIC_RUINS_JUNGLE,
    MYSTIC_RUINS_BASE
};

/* the field and sky task steps, which 1ST_READ.BIN's stage_steps and sky_steps
 * name */
void mystic_ruins_step(struct task *task);
void mystic_ruins_sky_step(struct task *task);

#endif
