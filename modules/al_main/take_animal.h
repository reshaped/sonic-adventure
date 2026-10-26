/* a Chao that takes an animal from a player, and what the animal gives it */
#ifndef AL_MAIN_TAKE_ANIMAL_H
#define AL_MAIN_TAKE_ANIMAL_H
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "system/task.h"

/* unless the Chao is in MODE_TAKE_ANIMAL already, send it after the first
 * animal it touches that a player holds and no Chao holds. the Chao's entry
 * then holds the animal's entry, and the Chao enters MODE_TAKE_ANIMAL. one
 * of two voices plays, and status_raise_like adds 2 to the Chao's like and
 * user_like. TRUE then; otherwise FALSE, with nothing changed */
Bool alife_try_take_animal(struct task_work *work, struct alife_chao *chao);
/* MODE_TAKE_ANIMAL's step. while the Chao goes for the animal its entry holds,
 * it turns toward it. it returns to idle where the animal or its task is gone.
 * on a step that leaves it facing the animal, it takes the animal where it
 * touches it and no Chao holds it. where it touches nothing then, it returns to
 * idle. the work's timer counts from the mode's start. past 120, such a step
 * returns the Chao to idle whatever it touched. an animal it took then keeps
 * ENTRY_HELD, and its held still points at the Chao's entry. a Chao that holds
 * the animal holds it up until the timer passes 120, then lets it go with a
 * push. the animal's kind then gives the Chao skills, a lean and its bit of
 * minimal, and parts unless the Chao is a chaos Chao. once two more motions
 * end, the Chao goes on to mode 13, which runs fun_0c911680.
 *
 * the animal's entry must stay while the Chao holds it. a step that finds the
 * Chao's held null returns the Chao to idle, then still writes through that
 * null pointer. where the animal or its task is gone as the Chao lets go, the
 * kind passed on is never set */
void alife_take_animal(struct task_work *work, struct alife_chao *chao);

#endif
