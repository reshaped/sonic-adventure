/* O ARCADE01 to O ARCADE03 pavement awnings that bounce players up */
#ifndef STG04_ARCADE_H
#define STG04_ARCADE_H

#include "system/task.h"

/* task steps for O ARCADE01, O ARCADE02 and O ARCADE03, named in the object
 * list */
void arcade_1_step(struct task *task);
void arcade_2_step(struct task *task);
void arcade_3_step(struct task *task);

#endif
