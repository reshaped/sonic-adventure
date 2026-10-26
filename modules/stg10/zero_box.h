/* C APP ZERO and C KILL ZERO invisible boxes that cue and remove Zero */
#ifndef STG10_ZERO_BOX_H
#define STG10_ZERO_BOX_H

#include "system/task.h"

/* the object list's steps for C APP ZERO and C KILL ZERO */
void zero_appear_box_step(struct task *task);
void zero_kill_box_step(struct task *task);

#endif
