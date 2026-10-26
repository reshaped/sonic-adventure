/* an event's white screen, which fades in, holds and fades out */
#ifndef EVENT_WHITE_H
#define EVENT_WHITE_H

/* fade the screen to white, hold it a step, and fade it back. each step,
 * the climb adds 255 / in_steps to the alpha, truncated. it ends at 255 once
 * the gap left is that change or less. each step, the fall takes
 * 255 / out_steps, truncated, from the alpha. it ends once the alpha is that
 * change or less. 0 steps climbs or falls in one step, and more than 255
 * never ends.
 *
 * the start waits a step with event_wait, so only the event's thread may
 * call it. a second start leaves the first running, out of
 * event_white_kill's reach. when the first ends, it puts the second out of
 * reach too. when no task can be made, it reads through null */
void event_white_start(int in_steps, int out_steps);
/* the same, holding the white hold + 1 steps */
void event_white_start_hold(int in_steps, int hold, int out_steps);
void event_white_kill(void);

#endif
