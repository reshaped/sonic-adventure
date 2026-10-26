/* the two modes Chaos 6's events ask the boss for */
#ifndef B_CHAOS6_EVENT_MODES_H
#define B_CHAOS6_EVENT_MODES_H

/* ask the boss for BOSS_MELT or BOSS_FORM through boss_next_mode, and keep
 * his mode in boss_last_mode, unless he is in that mode already. each sets
 * boss_return_mode either way: chaos6_event_melt to BOSS_MELTED, and
 * chaos6_event_form to BOSS_WAIT */
void chaos6_event_melt(void);
void chaos6_event_form(void);

#endif
