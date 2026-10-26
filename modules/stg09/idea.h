/* Ideacap and Idea, which show the second table's idea pieces */
#ifndef STG09_IDEA_H
#define STG09_IDEA_H

#include "system/task.h"

/* the object list's steps. idea_step also starts the pieces over: one in
 * place, and none carried */
void ideacap_step(struct task *task);
void idea_step(struct task *task);

#endif
