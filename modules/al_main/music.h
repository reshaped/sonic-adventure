/* a Chao that plays the trumpet, plays the drum, or sings */
#ifndef AL_MAIN_MUSIC_H
#define AL_MAIN_MUSIC_H

#include "al_main/chao.h"
#include "system/task.h"

/* the behaviors of MODE_TRUMPET, MODE_DRUM and MODE_SING, which their
 * declarations below follow in order. each starts only while bit 1, 5 or 9 of
 * the record's minimal is set, which status_require_minimal tests. where the
 * bit is clear, status_require_minimal sends the Chao back to idle and raises
 * its tedious. each draws a limit as it starts, and plays until its work's
 * timer passes it. each step slows the Chao by alife_drag with a drag of 0.3 */

/* motion 41, with the trumpet at node 26 of the Chao's pose and mouth 0, for a
 * limit of 500 to 1299. while sound_timer is past 40, the trumpet sounds at 1
 * in 5 each step, with eyes 2. the sound is 0x36d or 0x39b at even odds, and
 * restarts sound_timer. the behavior doesn't restart sound_timer as it starts.
 * past the limit, the Chao plays motion 6 without the trumpet, and goes back to
 * idle when that motion reaches its end */
void alife_trumpet(struct task_work *work, struct alife_chao *chao);
/* motion 138, for a limit of 200 to 399. the drumming waits until
 * sound_timer passes 60, which the behavior doesn't restart as it starts.
 * then dat_0ca4cc08 shows at node 4 of the Chao's pose, and the drum before
 * the Chao. while sound_timer is past 40, a beat comes at 1 in 5 each step,
 * with eyes 2. it is sound 0x28c, and restarts sound_timer. a Chao with a
 * kind above 0 shows eyes 2 and mouth 1, and any other mouth 2 */
void alife_drum(struct task_work *work, struct alife_chao *chao);
/* motion 85 and sound 0x35c, for a limit of 200 to 999. once sound_timer passes
 * 180, a verse comes at 1 in 50 each step. it is sound 0x35c or 0x361 at even
 * odds, and restarts sound_timer. the Chao shows eyes 2, and mouth 3 and mouth
 * 0 by turns of 20 steps. each turn is 40 of the work's ticks, which the
 * behavior and chao_step each count up */
void alife_sing(struct task_work *work, struct alife_chao *chao);

#endif
