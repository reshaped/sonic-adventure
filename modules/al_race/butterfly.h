/* BUTTERFLY, a butterfly that flaps and wanders around its place */
#ifndef AL_RACE_BUTTERFLY_H
#define AL_RACE_BUTTERFLY_H

#include "system/task.h"

/* does nothing; confetti_task_create calls it */
void butterflies_init(void);
/* BUTTERFLY's step in the race's object kinds. it starts the butterfly and
 * hands the task its step, draw and destructor. while the set editor holds
 * the task, this stays the step: each step it labels the scale's rows,
 * starts the butterfly again and steps it */
void butterfly_init(struct task *task);

#endif
