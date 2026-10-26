/* SCOOP, a scoop that a racing Chao picks up and digs with */
#ifndef AL_RACE_SCOOP_H
#define AL_RACE_SCOOP_H
#include <sg_xpt.h>

#include "system/task.h"

/* the scoop's item kind, scoop_tell's index in item_tells */
#define SCOOP_KIND 5

/* the commands a Chao sends the scoop it holds, as it starts to dig with the
 * scoop and as it stops */
#define SCOOP_DIG 11
#define SCOOP_DONE 12

/* the scoop's step in the object list, which makes the task a scoop */
void scoop_init(struct task *task);
/* take command from other, and return 1 when it takes effect, else 0. only a
 * command from the Chao that holds the scoop takes effect. ITEM_LET_GO lets
 * go of the scoop. SCOOP_DIG starts digging with a held scoop, and SCOOP_DONE
 * stops the digging, lets go and leaves the scoop used */
int scoop_tell(struct task *task, struct task *other, Sint8 command);

#endif
