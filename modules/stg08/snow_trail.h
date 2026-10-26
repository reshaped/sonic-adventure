/* Ice Cap's snow effects: a player's trail of snow strips, a board's flakes,
 * and the snow emitter */
#ifndef STG08_SNOW_TRAIL_H
#define STG08_SNOW_TRAIL_H
#include "system/task.h"

/* start a strip of the trail at the player, if it stands on the ground.
 * player must index player_work, player_motion and player_get. a player on
 * the ground must have a motion and a record. the strip ends at an age of
 * 8 steps. it holds its own copies of trail_object, its model and its
 * points. it shares the rest of the model, so Ice Cap must stay loaded while
 * the strip lives. where an allocation fails, the strip lives on without its
 * copies, and reads through null as trail_join's @bug says */
void icecap_trail_create(char player);
/* spray up to seventeen flakes from the player. it sprays none unless the
 * player stands on the ground and its record's speed.x is above 4. a flake
 * whose task can't be created is left out. player's low byte must index
 * player_work, player_motion and player_get. a flake never ages out, as
 * flake_step's @bug says. it lasts until neither the camera nor player 0 is
 * within 410 of it */
void icecap_board_spray(int player);
/* the step icecap_object_list names for COLD, a snow emitter. each step, it
 * puffs snow with a chance of ang.x / 65535. a puff's scale is ang.y / 65535
 * plus up to 0.01, and its speed is scl. the puffs' drag is 0 until the set
 * editor holds the emitter. while it does, the step sets the drag from
 * ang.z / 65535. the puffs keep a pointer to Ice Cap's params. Ice Cap must
 * stay loaded while any puff lives */
void icecap_snow_emitter_create(struct task *task);
#endif
