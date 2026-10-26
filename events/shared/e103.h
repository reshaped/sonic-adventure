/* an event's E-103, which draws the model each event defines */
#ifndef EVENT_E103_H
#define EVENT_E103_H

#include "system/task.h"

/* create E-103, or return null when task_create fails. he has an event
 * record for the motions he plays and the paths he runs. he draws his still
 * model while no motion is queued */
struct task *event_e103_create(void);

#endif
