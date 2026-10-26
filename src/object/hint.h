/* the hint box, O HINT in the object lists and O HINTBOX in Hot Shelter's.
 * as player 0 comes near in front of it, the box lights up and turns to
 * face the player. a player that talks to it is shown its message, with the
 * camera on the box and the time stopped */
#ifndef OBJECT_HINT_H
#define OBJECT_HINT_H

#include "system/task.h"

/* an event flag: while an event has control, a box neither moves nor draws
 * unless it is set */
#define FLAG_HINTS_IN_EVENTS 483

/* the object lists' step for O HINT and O HINTBOX. it starts the box when
 * script_pick passes for the box's row and the heap has room for its
 * records, and otherwise destroys the task. while edit_mode is set, it only
 * marks the box's place and labels the set editor's rows, and stays the
 * task's step */
void hint_init(struct task *task);

#endif
