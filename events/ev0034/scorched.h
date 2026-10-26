/* EV0034.PRS's scorched Egg Mobile: Eggman in his craft, with a scorched
 * face. nothing in the event creates it: no word in EV0034.PRS points at
 * ev0034_scorched_create */
#ifndef EV0034_SCORCHED_H
#define EV0034_SCORCHED_H

#include "system/task.h"

/* create the craft, with an event record, for the caller to place. its first
 * step gives it an event motion that loops its one action. null when
 * task_create fails. a failure to allocate the record or the motion goes
 * unchecked */
struct task *ev0034_scorched_create(void);

#endif
