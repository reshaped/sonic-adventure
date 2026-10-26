/* a Chao that picks a fruit up and eats it */
#ifndef AL_MAIN_HOLD_H
#define AL_MAIN_HOLD_H
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "system/task.h"

/* unless the Chao is in MODE_HOLD already, send it after the first fruit it
 * touches that no Chao holds. a fruit a player carries counts too. the
 * Chao's entry's held becomes the fruit's entry, the Chao enters MODE_HOLD,
 * and a sound plays. TRUE when it goes after one, and FALSE, with nothing
 * changed, otherwise */
Bool alife_try_hold(struct task_work *work, struct alife_chao *chao);
/* MODE_HOLD's behavior. the Chao turns toward the fruit its entry holds, and
 * picks it up where it faces and touches it, while no other Chao holds it.
 * before the pick-up, it goes back to idle where the fruit's entry or task is
 * gone. it does too where it faces the fruit and touches nothing, or faces it
 * past 120 steps.
 *
 * the Chao holds the fruit in front of it and bites it every 91 steps. each
 * bite takes a quarter of the fruit's scale, and the fruit asks the Chao to let
 * go once none is left. once it asks, or past 1200 steps from alife_try_hold,
 * the Chao lets go and puts the fruit down. a fruit let go before its last bite
 * falls. either way, letting go adds 100 to the record's pleasure, up to 200,
 * and passes the fruit's kind to status_eat_fruit. while the Chao holds the
 * fruit, its entry's held must stay set: the step reads the fruit's task
 * through it */
void alife_hold(struct task_work *work, struct alife_chao *chao);

#endif
