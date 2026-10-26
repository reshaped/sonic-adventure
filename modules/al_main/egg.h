/* a garden's Chao egg */
#ifndef AL_MAIN_EGG_H
#define AL_MAIN_EGG_H
#include "system/task.h"

/* an egg's first step, which alife_chao_create gives its task. the task
 * needs a work, and a struct alife_chao at its motion from alife_chao_alloc.
 * the step gives the task a collision record and an entry at its extra,
 * NULL when the heap has no room. it then sets the egg's destructor, step
 * and draw. the destructor passes the record to chao_list_remove and the
 * entry to alife_entry_remove. an egg whose record is the carried Chao goes
 * into player 0's hands, and the carried Chao is cleared */
void alife_egg_start(struct task *task);

#endif
