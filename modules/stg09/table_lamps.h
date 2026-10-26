/* AkaAoMido lamps on the tables, lit always or by a stopper, a Shoogai or a
 * drop target piece */
#ifndef STG09_TABLE_LAMPS_H
#define STG09_TABLE_LAMPS_H

#include "system/task.h"

/* the step the object list names for AkaAoMido. it starts the 19 lamps as
 * its children, each at a fixed place, whatever AkaAoMido's position */
void table_lamps_step(struct task *task);

#endif
