/* BUGLE, a bugle the deal gives a lane's racer, whose Chao takes it and
 * blows it */
#ifndef AL_RACE_BUGLE_H
#define AL_RACE_BUGLE_H
#include <sg_xpt.h>

#include "system/task.h"

/* the bugle's item kind, bugle_tell's index in item_tells */
#define BUGLE_KIND 4

/* the commands a Chao sends the bugle it holds, as it starts to blow the
 * bugle and as it stops */
#define BUGLE_BLOW 9
#define BUGLE_DONE 10

/* BUGLE's step in the race's object kinds. it gives the task the bugle's
 * record and callbacks, and starts it */
void bugle_init(struct task *task);
/* the bugle's handler. a command takes effect only while other is the Chao
 * that holds the bugle. ITEM_LET_GO drops the hold. BUGLE_BLOW starts a
 * held bugle's blowing. BUGLE_DONE ends it, which spends the bugle. returns
 * 1 when the command takes effect, else 0 */
int bugle_tell(struct task *task, struct task *other, Sint8 command);

#endif
