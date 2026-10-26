/* behaviors of motions and voices, most of which a Chao's moods choose */
#ifndef AL_MAIN_MOOD_H
#define AL_MAIN_MOOD_H

#include "al_main/chao.h"
#include "system/task.h"

/* the behaviors of modes 13 to 24 and 26, which their declarations below follow
 * in order. MODE_HUNGRY's is alife_hungry, and MODE_SLEEPY's is alife_sleepy.
 * each goes back to idle when it ends. each step slows the Chao by alife_drag
 * with a drag of 0.3. those after fun_0c911c38 keep the mode in last_mode as
 * they go back, and clear mode_timer.
 *
 * fun_0c911d5e, fun_0c9124c4 and fun_0c912600 start only while bit 8, 7 or 11
 * of the record's minimal is set, which status_require_minimal tests. where the
 * bit is clear, status_require_minimal sends the Chao back to idle and raises
 * its tedious */

/* motion 11 with icon ICON_SYMBOL_5, and voice 0x346 with mouth 1 or 0x384 with
 * mouth 3, at even odds. at each end of the motion, the Chao goes back to idle
 * once the timer passes a limit of 200 to 399. otherwise, once sound_timer
 * passes 80, it plays 0x384 and starts again. sound_timer counts the ends */
void fun_0c911680(struct task_work *work, struct alife_chao *chao);
/* MODE_HUNGRY's: motion 62 with eyes 1 and mouth 6, until the timer passes
 * a limit of 400 to 799. it starts with voice 0x360, 0x365 or 0x38f, or at
 * 1 in 10 none. once sound_timer passes 120, a voice comes at 1 in 200 each
 * step, and half of them are 0x358 */
void alife_hungry(struct task_work *work, struct alife_chao *chao);
/* motion 75 with icon ICON_SYMBOL_5. once sound_timer passes 60, voice 0x346 or
 * 0x347 plays at even odds. at the next end of the motion, the Chao goes back
 * to idle once the timer passes a limit of 200 to 399. otherwise it starts
 * again, with a new limit */
void fun_0c911ac0(struct task_work *work, struct alife_chao *chao);
/* motion 64 with icon ICON_SYMBOL_5, until the timer passes a limit of 200 to
 * 399. each step, motion 66 starts at 3 in 1000, and voice 0x367 plays at 1 in
 * 100 */
void fun_0c911c38(struct task_work *work, struct alife_chao *chao);
/* voice 0x36e and motion 68, to its end */
void fun_0c911d5e(struct task_work *work, struct alife_chao *chao);
/* motion 115 with icon ICON_SYMBOL_3, until the timer passes a limit of 120 to
 * 219. its voice is 0x348 for a curio below 0, and 0x355 otherwise */
void fun_0c911e14(struct task_work *work, struct alife_chao *chao);
/* motion 84 with mouth 0, until the timer passes a limit of 300 to 599.
 * each step, voice 0x367 plays at 1 in 100 */
void fun_0c911f40(struct task_work *work, struct alife_chao *chao);
/* motion 63 with mouth 0, to its end. voice 0x37a plays 8 steps in */
void fun_0c912040(struct task_work *work, struct alife_chao *chao);
/* motion 70 with icon ICON_HURT and mouth 2, until the timer passes 180.
 * its voice is 0x38c for an agres below 0, and 0x39c otherwise */
void fun_0c912120(struct task_work *work, struct alife_chao *chao);
/* motion 82 with mouth 5, until the timer passes a limit of 600 to 1199.
 * it starts with voice 0x35f or 0x360 at even odds. then every 302 steps it
 * plays 0x35f, or 0x360 for a rand() of 0 */
void fun_0c91221a(struct task_work *work, struct alife_chao *chao);
/* MODE_SLEEPY's: motion 71 with mouth 3, and alife_face_look's x at 0x2000.
 * it starts with voice 0x36c, but for a rand() of 0. past 250 steps it
 * plays motion 71 again, and goes back to idle at the motion's next end */
void alife_sleepy(struct task_work *work, struct alife_chao *chao);
/* voice 0x36e and motion 153, to its end, with speed.y set to 1 at timer 20.
 * each step also sets unk_0b4 to 1 plus a product: skills.fly / 2000, in whole
 * numbers, times the cosine of unk_0c4, which stays 0. at the end the behavior
 * clears unk_0b0, unk_0b4, unk_0b8 and unk_0c4. no code reads any of the
 * four */
void fun_0c9124c4(struct task_work *work, struct alife_chao *chao);
/* voice 0x36e and motion 155, until the timer passes a limit of 300 to
 * 2099 */
void fun_0c912600(struct task_work *work, struct alife_chao *chao);

#endif
