/* O TAIHOU cannon, which loads players and fires them off */
#ifndef ADV0100_CANNON_H
#define ADV0100_CANNON_H

#include "system/task.h"

/* the object list's step for O TAIHOU. the placement gives the elevation in
 * ang.x, the heading in ang.y and the speed in scl.x. the cannon takes the
 * angles as 16 bits, and the speed as no less than 0. outside the set editor
 * it then clears ang.x, ang.z and scl.
 *
 * while the cannon waits, it lowers its barrel and turns to the heading. it
 * shuts its hatch while the nearest player is farther than 40 from the
 * cannon's axis. while the character is Sonic, the nearest player opens the
 * hatch from within 40 of the axis. from within 9 of the axis and between 0
 * and 24 above the base, the player starts the cannon loading instead, again
 * only for Sonic. as it loads, the cannon holds each player that comes there
 * 24 above the base, in PLAYER_MODE_STILL, and shuts the hatch.
 *
 * once the hatch has shut, the barrel rises 0x180 a step, to no more than
 * the elevation. on the step a rise would pass the elevation, SE_BLAST plays
 * and the cannon fires. it flings each player it holds at the speed. the
 * line they fly along is the heading turned a half turn, raised by the
 * elevation. for each player, a shot task follows player 0 with a sphere
 * that hurts objects. from its second step on, a shot ends once player 0 is
 * on the ground. the cannon goes back to waiting on the 62nd step after the
 * one it fires on.
 *
 * the cannon registers its shapes as it loads and aims. as it waits, it
 * registers them while the nearest player is within 40 of its axis but
 * outside where it loads. after it fires, it registers them on each step
 * from the one its recoil is gone.
 *
 * made while the set editor holds it, the cannon stays the editor's. each
 * step it takes the placement's setting again, sets its barrel to the
 * elevation and draws the path the setting gives. while pad 1 doesn't hold
 * both X and Y, it labels the placement's fields.
 *
 * the heap must have room for its settings, its copy of its model's tree
 * and each shot's copy of its sphere. a player must exist as it waits, and
 * player 0 while a shot runs. without them, it writes or reads through
 * null */
void egg_carrier_cannon_step(struct task *task);

#endif
