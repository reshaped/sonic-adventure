/* O SAND ENTRY wall of sand in the jungle that pours away once the sand switch
 * is pressed */
#ifndef ADV02_SAND_ENTRY_H
#define ADV02_SAND_ENTRY_H

#include "system/task.h"

/* the step the object list names for O SAND ENTRY, which the Mystic Ruins field
 * task also creates in the jungle act. the wall pours away once
 * sand_switch_flag is TRUE, and creates a puff of dust each step while it
 * pours. it starts gone if the flag is set already */
void sand_entry_step(struct task *task);

#endif
