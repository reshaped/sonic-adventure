/* the modes and the action Chaos 0's events ask the boss for */
#ifndef B_CHAOS0_EVENT_MODES_H
#define B_CHAOS0_EVENT_MODES_H

#include <sg_xpt.h>

/* ask the boss for BOSS_MELT or BOSS_FORM through boss_next_mode, and keep
 * his mode in boss_last_mode, unless he is in that mode already. each sets
 * boss_return_mode either way: chaos0_event_melt to BOSS_MODE_21, and
 * chaos0_event_form to BOSS_WAIT */
void chaos0_event_melt(void);
void chaos0_event_form(void);
/* clear MOTION_HOLDING in motion_flags, set his motion to ACTION_EVENT, and
 * set that action's speed to speed */
void chaos0_event_action(Float speed);

#endif
