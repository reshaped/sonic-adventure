/* Froggy as an event actor that welds nodes 1 and 12 of his model */
#ifndef EV00E3_FROG_H
#define EV00E3_FROG_H

#include "system/task.h"

/* Froggy as event_actor_create makes an actor, with a step that welds node
 * 12 of the playing action's object onto node 1 before it runs
 * event_actor_step. every action played on him must hold Froggy's model.
 * allocation failure is unchecked */
struct task *ev00e3_frog_create(void);

#endif
