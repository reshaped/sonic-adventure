/* ENTRY BUTTON, a button on a base of its own, which sinks under player 0
 * and opens the race window */
#ifndef AL_RACE_ENTRY_BUTTON_H
#define AL_RACE_ENTRY_BUTTON_H

#include "system/task.h"

/* ENTRY BUTTON's step in the race's object kinds. it gives the task the
 * button's record and callbacks, and starts the button with its base's task
 * as its child. when the heap or the world pool runs out, the button's task
 * or its base's destroys itself on its next step */
void entry_button_init(struct task *task);

#endif
