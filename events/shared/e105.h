/* an event's E-105, which draws the model each event defines under
 * event_e105_object */
#ifndef EVENT_E105_H
#define EVENT_E105_H

#include <ninja.h>

#include "system/task.h"

/* the root of his still model: a hidden node without a model, whose
 * child, event_e105_1, each event defines */
extern NJS_OBJECT event_e105_object;

/* create E-105, or return null when task_create fails. he draws his still
 * model while no motion is queued. his event record, for the motions he
 * plays and the paths he runs, comes with his first step. wait a step
 * before the calls that need it */
struct task *event_e105_create(void);

#endif
