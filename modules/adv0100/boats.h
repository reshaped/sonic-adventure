/* O BOAT boat and O IKADA raft, which take player 0 off the sunken Egg
 * Carrier. the piers create them, and the object list can place them too */
#ifndef ADV0100_BOATS_H
#define ADV0100_BOATS_H

#include "system/task.h"

/* each vessel's step, for a task with a work. the first step sets the
 * vessel up, and each later one does nothing until egg_carrier_sunk is set.
 * a story flag of player 0's character unlocks the vessel, which until then
 * is neither drawn nor solid. the vessel leaves once player 0 has stayed in
 * its cylinder for sixteen steps in a row. the boat takes player 0 to
 * Station Square, and the raft to the Mystic Ruins */
void egg_carrier_boat_step(struct task *task);
void egg_carrier_raft_step(struct task *task);

#endif
