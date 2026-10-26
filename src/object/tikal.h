/* Tikal's hints. O TIKAL in the object lists raises a light where it's
 * placed, and the light circles player 0 while Tikal's hint shows. a boss's
 * hint grows the same kind of light beside player 0 */
#ifndef OBJECT_TIKAL_H
#define OBJECT_TIKAL_H
#include <sg_xpt.h>

#include "event/script.h"
#include "system/task.h"

/* O TIKAL's step in the object lists. scl.z is a row of the stage's script
 * table. the light rises where it's placed once player 0 is within 200 and
 * script_pick passes for that row. while the set editor holds the task, the
 * script needn't pass */
void tikal_init(struct task *task);
/* start a boss's hint. its light grows beside player 0, and then the hint
 * shows pages and plays voice, unless voice is -1. the light circles the
 * player until the hint's message ends, then spirals off, and the hint's
 * task destroys itself. pages must last until the message ends. it returns
 * the hint's task, or NULL when task_create fails */
struct task *tikal_hint_create(struct script_page *pages, Sint32 voice);
/* end the hint tikal_hint_create last made, if its task lives. a failed
 * create, or the destruction of any hint's task, leaves no hint to end.
 * before the hint speaks, its light shrinks away, and its message never
 * starts. after it speaks and before its message ends, this calls
 * message_cut, which cuts short the message holding the window. that is
 * the hint's own, unless the hint's message still waits for the window.
 * the light leaves once the hint's message ends */
void tikal_hint_end(void);

#endif
