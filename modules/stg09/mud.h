/* O MUD tints the players' palette by player 0's mud level, and O MUDOUT
 * lowers the level */
#ifndef STG09_MUD_H
#define STG09_MUD_H

#include <ninja.h>

#include "system/task.h"

/* player 0's mud level, from 0 to 1. mud_set_level clamps level to that
 * range. O MUD tints the palette by a new level on its sixth step with the
 * change pending. for the first change after O MUD starts, it does so on the
 * seventh */
Float mud_get_level(void);
void mud_set_level(Float level);

/* the object list's steps. O MUDOUT waits for player 0 on land within scl.x
 * of it. the first time, it sets player 0's mode to PLAYER_MODE_MUD_OUT. it
 * then lowers the level by up to 0.45 over 150 steps.
 *
 * O MUD copies the players' palette and PL_9MB.BIN's to the heap as it
 * starts. the heap must have room for both, or it writes through null. it
 * puts the players' palette back as it is destroyed. in the set editor, it
 * prints "Only 1 Obj!" */
void mud_out_step(struct task *task);
void mud_step(struct task *task);

#endif
