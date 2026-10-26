/* the Egg Carrier outside's field task: each act's start, step and sounds,
 * and act 6, the transformation */
#ifndef ADV0100_EGG_CARRIER_H
#define ADV0100_EGG_CARRIER_H
#include <sg_xpt.h>

#include "system/task.h"

/* room in field_act_tasks for the tasks an act starts, which act_end
 * destroys */
#define FIELD_ACT_TASKS 4

/* only egg_carrier.c reads captain_switch_was and field_act_tasks. bss
 * follows each name's first declaration, so declared here they lie ahead of
 * egg_carrier_sunk, as in the image. as statics in egg_carrier.c, they would
 * follow it */
extern Sint8 captain_switch_was; /* switch 0's state at act 3's previous step */
extern struct task *field_act_tasks[FIELD_ACT_TASKS];
/* TRUE once the character's story has sunk the Egg Carrier into the sea, as
 * egg_carrier_has_sunk reported when the field task started */
extern Bool egg_carrier_sunk;

/* the field task step, which 1ST_READ.BIN's stage_steps names */
void egg_carrier_step(struct task *task);

#endif
