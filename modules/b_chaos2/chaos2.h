/* Chaos 2's stage: its task, its sky's task, and the boss music's wait */
#ifndef B_CHAOS2_CHAOS2_H
#define B_CHAOS2_CHAOS2_H
#include <ninja.h>

#include "system/task.h"

/* the steps of the stage's task and its sky's, which 1ST_READ.BIN's
 * stage_steps and sky_steps name */
void chaos2_step(struct task *task);
void chaos2_sky_step(struct task *task);
/* a step that counts its work's timer down. once the count falls below 0,
 * it asks for the music in its work's mode and destroys its task. it
 * forgets every stream request first, and keeps the music for
 * stream_restore_music. the boss makes the task and sets its timer and
 * mode */
void chaos2_music_wait_step(struct task *task);

#endif
