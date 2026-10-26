/* the tornado, which the player flies through the sky chase */
#ifndef SHOOTING_TORNADO_H
#define SHOOTING_TORNADO_H
#include <sg_xpt.h>

#include "system/task.h"

/* player 0's step in the sky chase, which 1ST_READ.BIN's
 * stage_create_players gives Sonic or Tails in STAGE_SHOOTING1 and
 * STAGE_SHOOTING2. the task needs a work and a motion. its work's
 * PLAYER_VARS must hold slot 0 and the character.
 *
 * the first step makes the player's record and the reticle, and the
 * destructor frees the record */
void shooting_player_step(struct task *task);
/* count in shooting_flight.charge the steps in a row that player 0's pad
 * holds a fire button. the fire buttons are pad_action_buttons and
 * pad_jump_buttons. a step with none held sets the count to 0. it returns
 * TRUE from the 32nd held step on, else FALSE */
Bool tornado_count_charge(void);

#endif
