/* Knuckles' hint, O KN HINT: Tikal's light, which glows brighter as
 * Knuckles nears it. once he reaches it, it calls him and leads him toward
 * the nearest piece of the master emerald */
#ifndef OBJECT_KNUCKLES_HINT_H
#define OBJECT_KNUCKLES_HINT_H

#include "system/task.h"

/* the object lists' step for O KN HINT, which starts the hint. outside the
 * set editor, for a character other than Knuckles, it kills the hint's
 * placement but then sets its own step over the kill's task_destroy. that
 * hint runs on as Knuckles' does, and its placement makes no more tasks
 * until placement_reset */
void knuckles_hint_init(struct task *task);

#endif
