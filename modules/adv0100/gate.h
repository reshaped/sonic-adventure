/* O A GATE pair of deck gates that swing open and shut at the command in
 * FLAG_GATE */
#ifndef ADV0100_GATE_H
#define ADV0100_GATE_H

#include "system/task.h"

/* the event flag that holds the gates' next gate_command, which the field
 * task, the lift and events set. the gates take the command and clear the
 * flag to GATE_NO_COMMAND */
#define FLAG_GATE 43

enum gate_command {
    GATE_NO_COMMAND,
    GATE_SHUT_NOW, /* stand shut at once */
    GATE_OPEN_NOW, /* stand open at once */
    GATE_SHUT, /* swing shut from open */
    GATE_OPEN /* swing open from shut */
};

/* the object list's step */
void egg_carrier_gate_step(struct task *task);

#endif
