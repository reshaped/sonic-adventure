/* a Chao that's hit */
#ifndef AL_MAIN_HURT_H
#define AL_MAIN_HURT_H
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "system/task.h"

/* take a hit, unless the Chao is in its cooldown, the first 60 calls after its
 * last hit. a hit is COLLISION_DAMAGED in the work's flags or ENTRY_HIT in the
 * entry's. the result is TRUE for a hit, and FALSE for none. during the
 * cooldown the function returns no value, and R0 holds 122, which callers read
 * as TRUE.
 *
 * a hit sends the Chao into MODE_HURT and has it let go of what it holds. it
 * knocks the Chao away from the hitter, whose pos it reads through the
 * collision's hit, even for ENTRY_HIT. where the hitter lies nearly along x or
 * z from the Chao, the knock is toward +x instead, and a random way on z. the
 * record loses 200 hp, down to 0. status_hurt takes 10 from the current
 * character's like and from user_like, takes 1 from a life above 0 and clears
 * pleasure. one feeling gains 100, up to 200: sorrow where agres and kind are
 * both above 0, anger where agres alone is, and fear otherwise */
Bool alife_take_hit(struct task_work *work, struct alife_chao *chao);
/* MODE_HURT's behavior. the Chao cries and falls spinning until it lands,
 * then lies as the spin settles. past 200 steps from the hit it gets up,
 * and goes back to idle at the motion's end. on the ground, its accel is
 * -0.3 times its speed on x and z */
void alife_hurt(struct task_work *work, struct alife_chao *chao);

#endif
