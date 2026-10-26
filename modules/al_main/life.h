/* a Chao's two behaviors in a cocoon: growing up, and the end of its life */
#ifndef AL_MAIN_LIFE_H
#define AL_MAIN_LIFE_H

#include "al_main/chao.h"
#include "system/task.h"

/* the behaviors of MODE_GROW_UP and MODE_LIFE_OVER, in order. each shuts
 * the Chao in a cocoon, where no player can pick it up. there collision
 * shape 2 stands in for shapes 0 and 1, so the Chao's work must have all
 * three */

/* in the cocoon, the Chao takes its adult type, and its shape is built
 * again. once the cocoon is gone, the Chao goes back to idle */
void alife_evolve(struct task_work *work, struct alife_chao *chao);
/* in the cocoon, the Chao's record counts one more reincarnation. a Chao
 * with hp, a user_like above 60 and a type other than CHAO_CHAOS then
 * leaves an egg of its next generation where it stands. it does so only
 * where the records hold an empty one. the egg keeps still until the cocoon
 * is gone, and its task must last that long. then the Chao's task ends */
void alife_reincarnate(struct task_work *work, struct alife_chao *chao);

#endif
