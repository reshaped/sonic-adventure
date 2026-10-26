/* the modes and the action Chaos 2's event, EV0087.PRS, asks the boss
 * for */
#ifndef B_CHAOS2_EVENT_MODES_H
#define B_CHAOS2_EVENT_MODES_H

#include <sg_xpt.h>

/* ask the boss for BOSS_MELT or BOSS_FORM through boss_next_mode, and keep
 * his mode in boss_last_mode, unless he is in that mode already. each sets
 * boss_return_mode either way: chaos2_event_melt to BOSS_MODE_19, and
 * chaos2_event_form to BOSS_WAIT */
void chaos2_event_melt(void);
void chaos2_event_form(void);
/* clear MOTION_HOLDING in motion_flags, set his motion to ACTION_EVENT, and
 * set that action's speed to speed */
void chaos2_event_action(Float speed);

#endif
