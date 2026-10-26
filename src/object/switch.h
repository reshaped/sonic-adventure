/* switches: floor buttons that turn numbered flags on and off, which other
 * objects read */
#ifndef OBJECT_SWITCH_H
#define OBJECT_SWITCH_H
#include <sg_xpt.h>

#include "system/task.h"

/* the step of a switch, as the object lists place it */
void switch_step(struct task *task);
/* turn switch id's flag on or off, and mark it changed. id must be below
 * 32, the number of switches: switch_set writes past the flags for a
 * larger one, unchecked */
void switch_set(Uint32 id, Bool on);
/* TRUE while switch id's flag is on, else FALSE. id as for switch_set */
Bool switch_is_on(Uint32 id);
/* turn every switch off and clear its change, for a new stage */
void switch_reset(void);

#endif
