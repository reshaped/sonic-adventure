/* O HYOUJIKI: a four-digit counter. its digits roll, then lock one at a time
 * from the ones up */
#ifndef STG09_COUNTER_H
#define STG09_COUNTER_H
#include <sg_xpt.h>

#include "system/task.h"

/* the indexes a counter can hold, 0 to COUNTERS - 1. a counter takes its
 * index from scl.x, truncated. the first counter to start with an index
 * holds it until it is destroyed. counter 0 starts at a random number, and
 * rolls to a new one at each hit. the others take the rings
 * gold_heap_get_rings counts as a player first comes near */
#define COUNTERS 5

/* the object list's step. task needs TASK_WORK's record */
void counter_step(struct task *task);
/* roll the digits of the counter holding index again, to lock on value
 * capped at 9999. returns TRUE, or FALSE while no counter holds index */
Bool counter_set(int index, int value);
/* the value the counter holding index locks on, or -1 while no counter
 * holds index. nothing in this image calls it */
int counter_get(int index);

#endif
