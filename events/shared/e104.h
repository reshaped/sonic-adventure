/* an event's E-104, which draws the model each event defines */
#ifndef EVENT_E104_H
#define EVENT_E104_H

#include "system/task.h"

/* create E-104, or return null when task_create fails. he has an event
 * record for the motions he plays and the paths he runs. he draws his still
 * model while no motion is queued */
struct task *event_e104_create(void);

#endif
