/* a pulley that carries player 0 from a path's start to its end. its cable
 * is strung between the path's points */
#ifndef STG05_PULLEY_H
#define STG05_PULLEY_H

#include "system/task.h"

/* the step act 0's two pulley paths name for their tasks. player 0 must
 * exist while a task runs. a task destroys itself once the stage or act
 * changes */
void pulley_step(struct task *task);

#endif
