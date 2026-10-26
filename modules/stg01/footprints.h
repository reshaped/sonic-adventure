/* O ASIATO: the footprints a player leaves on ground with WORLD_FOOTPRINTS */
#ifndef STG01_FOOTPRINTS_H
#define STG01_FOOTPRINTS_H

#include "system/task.h"

/* the step beach_object_list names for O ASIATO: the footprints of the
 * player numbered work->scl.x. beach_act_start makes such a task for player
 * 0 in acts 0 to 2. the task needs a work, and scl.x a whole number from 0
 * to 127.
 *
 * the step waits until the player has a work. with a number of PLAYER_COUNT
 * or more, it waits for ever. then the prints start. they take 2,844 bytes
 * of the heap until the task ends. without that room, the start writes
 * through null. the player's work must then have a collision with a shape.
 * the task kills itself when the stage or act changes. a task that starts in
 * the set editor only shows its player's number */
void footprints_start(struct task *task);

#endif
