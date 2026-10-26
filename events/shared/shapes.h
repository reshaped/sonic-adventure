/* an event's switch for whether a task's collision shapes stand firm */
#ifndef EVENT_SHAPES_H
#define EVENT_SHAPES_H

#include "system/task.h"

/* make every shape of the task's collision record stand firm against the
 * groups PUSH_PLAYERS and PUSH_ENEMIES. the task's work must have a
 * collision record */
void event_shapes_set_firm(struct task *task);
/* stop every shape of the task's collision record standing firm against
 * PUSH_PLAYERS, leaving PUSH_ENEMIES as it is. the task's work must have a
 * collision record */
void event_shapes_clear_firm_players(struct task *task);

#endif
