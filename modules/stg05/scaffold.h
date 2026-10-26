/* O ASIBA1, a solid scaffold footing that its set entry scales */
#ifndef STG05_SCAFFOLD_H
#define STG05_SCAFFOLD_H

#include "system/task.h"

/* the step mountain_object_list names for O ASIBA1. unless the set editor holds
 * the scaffold, its first step adds a scaled copy to the world. that step
 * writes through null when the heap can't hold the copy's model. it also writes
 * through null when every object of the world's pool is lent */
void scaffold_step(struct task *task);

#endif
