/* COCONUT, a coconut that falls and bounces to rest */
#ifndef AL_RACE_COCONUT_H
#define AL_RACE_COCONUT_H

#include "system/task.h"

/* COCONUT's step in the race's object kinds. it gives the task the
 * coconut's record and callbacks, and starts it falling */
void coconut_init(struct task *task);

#endif
