/* the capsule, O RELEASE in the object lists. a player standing on its
 * button sinks it. then the capsule breaks into flying pieces and lets three
 * animals go. 60 steps later, it clears the stage */
#ifndef OBJECT_CAPSULE_H
#define OBJECT_CAPSULE_H

#include "system/task.h"

/* the player on the stage capsule's button, 0 or 1. player 1 counts only
 * when the character is Tails. -1 for none, and -2 with no stage capsule.
 * once the button is pushed, it is the player that pushed it */
int capsule_get_player(void);
/* forget the stage's capsule, for a new stage */
void capsule_reset(void);
/* the object lists' step for O RELEASE: start the capsule, with its button
 * as a child task. it becomes the stage's capsule unless the set editor
 * holds it. with the task pool or the heap out of room, it reads through a
 * null button */
void capsule_start(struct task *task);
/* read capsule_active, to no effect */
void capsule_test_active(void);
/* the stage's capsule task, or NULL */
struct task *capsule_get(void);

#endif
