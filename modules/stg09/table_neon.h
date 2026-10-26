/* Neon, whose three models trade places, and Neonkabe, a wall of six lamps
 * that blink and shine */
#ifndef STG09_TABLE_NEON_H
#define STG09_TABLE_NEON_H

#include "system/task.h"

/* the steps the object list names for Neon and Neonkabe. Neonkabe starts its
 * six lamps as its children, which draw their models without a translation,
 * whatever Neonkabe's position */
void table_neon_step(struct task *task);
void table_neon_wall_step(struct task *task);

#endif
