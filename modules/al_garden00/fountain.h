/* the fountain: O AR_FUN */
#ifndef AL_GARDEN00_FOUNTAIN_H
#define AL_GARDEN00_FOUNTAIN_H

#include "system/task.h"

/* the step O AR_FUN names. it does nothing, so a placed fountain never draws */
void fountain_init(struct task *task);
/* does nothing. no caller in this image */
void fun_0cb80744(struct task *task);
/* the step a fountain would run. it draws the jets, their texture run down
 * by the steps it has counted, and prints UVSPEED in the set editor. no
 * caller in this image */
void fountain_step(struct task *task);

#endif
