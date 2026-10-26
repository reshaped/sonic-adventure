/* O MANNEQUIN mannequin that players pick up and carry */
#ifndef ADV00_MANNEQUIN_H
#define ADV00_MANNEQUIN_H

#include "system/task.h"

/* the step the object list names for O MANNEQUIN. a held mannequin turns
 * with player 0. while an event has control, the step does nothing, drawing
 * included */
void mannequin_step(struct task *task);

#endif
