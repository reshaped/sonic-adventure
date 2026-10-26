/* the flags that hand Chaos from his event to his fight */
#ifndef CHAOS_START_H
#define CHAOS_START_H

/* set boss_waiting and clear chaos_handed_over. the Chaos bosses but Chaos 4
 * call it as they set up */
void chaos_start(void);
/* set chaos_handed_over. the events before the Chaos fights but Chaos 4's
 * call it as they hand Chaos to the boss */
void chaos_end_event(void);

#endif
