/* O SHAT1 and O SHAT2, shutters that drop behind a player who passes
 * through. they rise again for one in front */
#ifndef STG09_SHUTTER_H
#define STG09_SHUTTER_H

#include "system/task.h"

/* the object list's steps: O SHAT1's, and O SHAT2's taller shutter. the
 * shutters that share scl.z drop together. a shutter with bit 0 of scl.x set
 * ignores the players: it starts no drop and never rises. it drops only when
 * a shutter with its scl.z drops, and then stays down */
void shutter_step(struct task *task);
void shutter_tall_step(struct task *task);

#endif
