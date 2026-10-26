/* O BOX SWITCH, a box a player can carry to its place in a box hole. there
 * it sinks, and turns its switch on */
#ifndef STG12_BOX_SWITCH_H
#define STG12_BOX_SWITCH_H

#include <sg_xpt.h>

#include "system/task.h"

/* number of box switches: four for each of the two holes */
#define BOX_SWITCHES 8

/* the step act 1's object list names for O BOX SWITCH */
void box_switch_step(struct task *task);
/* turn every box switch off, so that boxes placed afterward start loose */
void box_switch_reset(void);
/* TRUE once the box numbered box, 0 to 7, has sunk into its place since the
 * last box_switch_reset */
Bool box_switch_is_on(int box);

#endif
