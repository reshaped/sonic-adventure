/* O KDANL and O KDANM: a large and a medium flower bed, whose soil Knuckles
 * can dig into */
#ifndef STG09_FLOWER_BED_H
#define STG09_FLOWER_BED_H

#include "system/task.h"

/* the object list's steps. each task needs TASK_WORK's record */
void flower_bed_large_step(struct task *task);
void flower_bed_medium_step(struct task *task);

#endif
