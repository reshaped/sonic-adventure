/* O HAKA, a tomb, and O CHAIR, a chair. O MASIBA, a raft that rises with the
 * lava and tips toward player 0 while it is near */
#ifndef STG05_RAFT_H
#define STG05_RAFT_H

#include "system/task.h"

/* the steps mountain_object_list names for O HAKA, O CHAIR and O MASIBA.
 * O MASIBA writes through null when every object of the world's pool is
 * lent */
void tomb_step(struct task *task);
void chair_step(struct task *task);
void raft_step(struct task *task);

#endif
