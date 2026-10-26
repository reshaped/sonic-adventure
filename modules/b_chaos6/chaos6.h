/* Chaos 6's stage: its task, and the task that starts its music */
#ifndef B_CHAOS6_CHAOS6_H
#define B_CHAOS6_CHAOS6_H

#include "system/task.h"

/* the stage's task step. 1ST_READ.BIN's stage_steps reaches it by its
 * address, 0x0c900000, not by its name */
void chaos6_step(struct task *task);
/* the step of a task that waits work->timer steps, then asks for music
 * work->mode, keeps it for stream_restore_music, and destroys itself. it
 * first forgets every stream request and starts the streams' task unless
 * one runs. its creator sets both fields, as chaos6_begin does when it
 * destroys the streams' task */
void chaos6_music_wait_step(struct task *task);

#endif
