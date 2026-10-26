/* an event's stopwatch, for the debug screen */
#ifndef EVENT_STOPWATCH_H
#define EVENT_STOPWATCH_H

/* a stopwatch, stopped at 0, whose count of steps prints at the screen's
 * top left. on the second pad, R starts and stops it. L takes a lap while
 * it runs, and resets it while it's stopped. a second create leaves the
 * first running, out of reach */
void event_stopwatch_create(void);
void event_stopwatch_kill(void);

/* the calls below act on the stopwatch the last create made. they read
 * through null before a create, after event_stopwatch_kill, and when no
 * task could be made */

/* stop the count at 0, and drop the lap */
void event_stopwatch_reset(void);
/* run the count */
void event_stopwatch_start(void);
/* halt the count */
void event_stopwatch_stop(void);
/* hold the count as it stands, printed beside it */
void event_stopwatch_lap(void);

#endif
