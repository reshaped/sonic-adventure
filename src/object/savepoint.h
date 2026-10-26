/* the savepoint: a gate with two lamps. player 0's touch saves player 0's
 * place as the restart point */
#ifndef OBJECT_SAVEPOINT_H
#define OBJECT_SAVEPOINT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* forget the restart point, so restart_keep_this_act returns FALSE */
void restart_clear(void);
/* forget the restart point when it was saved in another stage or act.
 * TRUE while a point saved in this stage and act remains, else FALSE */
Bool restart_keep_this_act(void);
/* set *pos and *ang to the restart point, and the clock to its time. it
 * also takes every ring away and makes player 0 invincible */
void restart_restore(NJS_POINT3 *pos, struct angles *ang);
/* make *pos and *ang the restart point of this stage and act, at the
 * clock's time now */
void restart_save(NJS_POINT3 *pos, struct angles *ang);
/* the step of a savepoint. its first step takes the savepoint's record
 * from the heap and its lamps' two tasks from the task pool. it uses each
 * unchecked, so with no room it goes through null */
void savepoint_step(struct task *task);

#endif
