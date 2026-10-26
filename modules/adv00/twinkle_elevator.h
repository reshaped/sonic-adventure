/* O ELEVATOR elevator to Twinkle Park, locked until the story opens it */
#ifndef ADV00_TWINKLE_ELEVATOR_H
#define ADV00_TWINKLE_ELEVATOR_H

#include "system/task.h"

/* event flag 2: a request of the elevator, one of elevator_request.
 * ELEVATOR_TO_TOP and ELEVATOR_TO_BOTTOM put the elevator at that end at
 * once, and ELEVATOR_GO sends it from the end it is at to the other. the
 * elevator answers the flag each step, so a request holds until it
 * changes, and a locked elevator answers none. the elevator sets the flag
 * to ELEVATOR_STAY only as it starts unlocked */
#define FLAG_ELEVATOR 2
enum elevator_request {
    ELEVATOR_STAY,
    ELEVATOR_TO_TOP,
    ELEVATOR_TO_BOTTOM,
    ELEVATOR_GO
};

/* the object list's step */
void twinkle_elevator_init(struct task *task);

#endif
