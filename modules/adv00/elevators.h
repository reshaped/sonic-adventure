/* O TENBOU ELE, O HOTEL ELE and O HIGH ELE elevators to the lookout, the Chao
 * Garden and the highway. O ROUKA and the lights on the way to Twinkle Park are
 * here too */
#ifndef ADV00_ELEVATORS_H
#define ADV00_ELEVATORS_H

#include "system/task.h"

/* the steps the object list names for O TENBOU ELE, O HOTEL ELE and O HIGH ELE.
 * an elevator's doors open while player 0 is near them. once player 0 has stood
 * inside for 51 steps in a row, the doors shut. the act then fades to the
 * elevator's stage at act 0 and entrance 0: STAGE_CHAOS2, STAGE_ALIFE_SS or
 * STAGE_HIGHWAY. the lookout's elevator opens only for Knuckles, from his
 * story's event 160 until he clears Chaos 2. the highway's opens only for Sonic
 * and Tails, once their stories open the way. the Chao Garden's always opens */
void lookout_elevator_init(struct task *task);
void hotel_elevator_init(struct task *task);
void highway_elevator_init(struct task *task);
/* the step the object list names for O ROUKA. outside the set editor, the task
 * destroys itself on its next step, and its placement makes no more */
void corridor_step(struct task *task);
/* in the Twinkle Park act, create the task of the lights on the way to Twinkle
 * Park, which ends once the stage or act changes */
void twinkle_lights_start(void);

#endif
