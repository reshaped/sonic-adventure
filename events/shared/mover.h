/* an event's mover: the task that carries out a move from shared/move.h */
#ifndef EVENT_MOVER_H
#define EVENT_MOVER_H

#include "system/task.h"

/* a mover's step, draw and destructor. the draw draws nothing. the
 * destructor frees the move, and leaves the mover's slot in event_movers
 * set */
void event_mover_step(struct task *task);
void event_mover_draw(struct task *task);
void event_mover_destroy(struct task *task);

#endif
