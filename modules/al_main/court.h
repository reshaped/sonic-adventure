/* the behaviors of courtship: a Chao that calls for a partner, the partner
 * that joins it, and the two as they breed */
#ifndef AL_MAIN_COURT_H
#define AL_MAIN_COURT_H

#include "al_main/chao.h"
#include "system/task.h"

/* MODE_COURT. the Chao scatters nine flowers, one each 26 steps, then calls
 * for a partner and forgets what it holds. touching any Chao then starts
 * MODE_BREED. after 3600 steps with none, its breed falls to 0 and it goes
 * back to idle */
void alife_court(struct task_work *work, struct alife_chao *chao);
/* MODE_PARTNER, which a Chao takes as it joins a courting one. it goes
 * straight back to idle, and the courting Chao, which it touches, breeds */
void alife_partner(struct task_work *work, struct alife_chao *chao);
/* MODE_BREED: breed with the partner the Chao holds, or with itself when it
 * holds none, then go back to idle. a child or Chaos partner is let go, but
 * breeds all the same.
 *
 * breeding needs fewer than ALIFE_CHAO_FULL Chao and eggs in the garden, and
 * two random draws. the first wins at a chance of the partner's charm in
 * 200. the second wins at one half plus the sum of the partner's four skills
 * in 40000. winning both costs the Chao its breed and 50 of its life, down to
 * 0. where chao_find_empty finds a free record, an egg that takes after both
 * then lies 3 along the Chao's facing. the egg's task creation is
 * unchecked */
void alife_breed(struct task_work *work, struct alife_chao *chao);

#endif
