/* O KIN: a heap of gold that rises as rings are deposited. O KINWALL: a
 * wall that stands until 400 rings are deposited */
#ifndef STG09_GOLD_H
#define STG09_GOLD_H
#include <sg_xpt.h>

#include "system/task.h"

/* the object list's steps. each task needs TASK_WORK's record */
void gold_heap_step(struct task *task);
void gold_wall_step(struct task *task);
/* the rings deposited, or 0 while no heap is the keeper. a heap that starts
 * outside the set editor while there is none becomes the keeper. it stays the
 * keeper until it is destroyed */
int gold_heap_get_rings(void);
/* set the rings deposited and return TRUE, or return FALSE while no heap is
 * the keeper */
Bool gold_heap_set_rings(int rings);

#endif
