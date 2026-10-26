/* O CARGOSTART, a row of five cargo cars that takes player 0 to the cargo
 * ride. once player 0 stands on them, they pull away and the screen fades to
 * black. player 0 is then set down in the tunnel */
#ifndef STG12_CARGO_START_H
#define STG12_CARGO_START_H

#include "system/task.h"

/* the step act 2's object list names for O CARGOSTART */
void cargo_start_step(struct task *task);

#endif
