/* O_Saku01, O_Saku01b, O_Saku02 and O_Saku02b: solid fences */
#ifndef STG08_FENCES_H
#define STG08_FENCES_H

#include "system/task.h"

/* the object list's steps */
void fence_01_step(struct task *task);
void fence_01b_step(struct task *task);
void fence_02_step(struct task *task);
void fence_02b_step(struct task *task);

#endif
