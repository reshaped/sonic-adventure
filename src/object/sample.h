/* samples: stand-ins, drawn as the marker cylinder, for things a player
 * carries, in three sizes, and for one the player shakes */
#ifndef OBJECT_SAMPLE_H
#define OBJECT_SAMPLE_H

#include "system/task.h"

/* the first steps of SAMPLE S, M, L and SW, kinds of object the Mystic
 * Ruins place. each sets its sample up and gives the task the sample's own
 * step */
void sample_small_start(struct task *task);
void sample_medium_start(struct task *task);
void sample_large_start(struct task *task);
void sample_shake_start(struct task *task);

#endif
