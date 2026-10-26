/* an event's black over the screen, which clears at a rate */
#ifndef EVENT_BLACK_H
#define EVENT_BLACK_H
#include <sg_xpt.h>

/* set the rate the black clears at: the transparency it gains each step.
 * its transparency starts at 0, opaque, and by 1 it is gone. a call while there
 * is no black creates one, and a failed task_create is unchecked */
void event_black_set_rate(Float rate);
/* kill the black on its next step, if there is one */
void event_black_kill(void);

#endif
