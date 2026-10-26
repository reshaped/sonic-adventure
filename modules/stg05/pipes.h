/* O PIPEI, O PIPEBOX and O PIPEL: a straight pipe, a box of pipes and a bent
 * pipe. each is a solid model, drawn and shaped at one more than the set
 * entry's scale */
#ifndef STG05_PIPES_H
#define STG05_PIPES_H

#include "system/task.h"

/* the steps mountain_object_list names for O PIPEI, O PIPEBOX and O PIPEL */
void pipe_i_step(struct task *task);
void pipe_box_step(struct task *task);
void pipe_l_step(struct task *task);

#endif
