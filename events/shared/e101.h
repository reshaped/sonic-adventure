/* an event's E-101, with a bobbing jet on his back. each event that
 * links e101.c defines his model and the jet's */
#ifndef EVENT_E101_H
#define EVENT_E101_H

#include <ninja.h>

#include "system/task.h"

/* the jet: one node, which e101_jet_object.c defines over the
 * event_e101_jet_model each event defines */
extern NJS_OBJECT event_e101_jet_object;

/* create E-101, or return null when task_create fails. he has an event
 * record for the motions he plays and the paths he runs. he draws his still
 * model while no motion is queued */
struct task *event_e101_create(void);

#endif
