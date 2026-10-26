/* making characters playable, and the notice of each */
#ifndef PROGRESS_PLAYABLE_H
#define PROGRESS_PLAYABLE_H
#include <sg_xpt.h>

struct task;

/* make character playable, and start a notice of it: a task that shows its
 * name in a message, and saves the story as it ends. return the task. for a
 * character the table doesn't hold, or one already playable, return null
 * and change nothing. where the task can't be made, return null too, with
 * the character playable */
struct task *playable_unlock(int character);
/* TRUE from a notice's start until its task is destroyed */
Bool playable_notice_active(void);

#endif
