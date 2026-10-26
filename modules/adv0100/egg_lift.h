/* O EGGLIFT lift that takes player 0 down from the deck into the Egg Carrier */
#ifndef ADV0100_EGG_LIFT_H
#define ADV0100_EGG_LIFT_H

#include "system/task.h"

/* the event flag that holds the lift's next lift_command, which the field
 * task and events set. the lift takes the command and clears the flag to
 * LIFT_NO_COMMAND */
#define FLAG_LIFT 44

enum lift_command {
    LIFT_NO_COMMAND,
    LIFT_HIGH_NOW, /* stand at the top at once, bobbing */
    LIFT_LOW_NOW, /* stand at the bottom at once */
    LIFT_RISE, /* rise from the bottom */
    LIFT_SINK, /* sink from the top */
    LIFT_READY_NOW, /* stand at the top at once, to take player 0 down */
    /* rise from the bottom, commanding the gates open, to take player 0 down */
    LIFT_RISE_READY
};

/* the object list's step */
void egg_carrier_outside_egg_lift_step(struct task *task);

#endif
