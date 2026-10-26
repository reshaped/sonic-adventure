/* CHEER CHAO, a Chao on the race course that cheers over and over */
#ifndef AL_RACE_CHEER_CHAO_H
#define AL_RACE_CHEER_CHAO_H

#include "system/task.h"

/* CHEER CHAO's step in the race's object kinds. it gives the task the
 * Chao's record and callbacks, and starts the cheer at a random frame */
void cheer_chao_init(struct task *task);

#endif
