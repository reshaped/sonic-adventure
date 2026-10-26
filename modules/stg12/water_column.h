/* the column of water that falls from a burst water tank, and splashes */
#ifndef STG12_WATER_COLUMN_H
#define STG12_WATER_COLUMN_H

#include "system/task.h"

/* the column's first step. a water tank creates the column at its own
 * position as it bursts */
void water_column_start(struct task *task);

#endif
