/* Card: the second table's thirteen cards, which a touch deals into a hand
 * of five. CardDisp: the display of the hand dealt so far */
#ifndef STG09_CARDS_H
#define STG09_CARDS_H

#include "system/task.h"

/* the object list's steps. a Card's task needs TASK_WORK's record, and its
 * scl.x is its place on the table, 0 to 12 */
void card_display_step(struct task *task);
void card_step(struct task *task);

#endif
