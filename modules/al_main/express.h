/* a Chao's mark for its favorite character, and its behaviors of voices and
 * a prop */
#ifndef AL_MAIN_EXPRESS_H
#define AL_MAIN_EXPRESS_H
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "system/task.h"

/* the behaviors of modes 38, MODE_MARK_FAVORITE, to 42, which their
 * declarations below follow in order. each but the last keeps the mode in
 * last_mode as it goes back to idle, and clears mode_timer. each but
 * fun_0c912ca8 starts only while a bit of the record's minimal is set, which
 * status_require_minimal tests. where the bit is clear, status_require_minimal
 * sends the Chao back to idle and raises its tedious */

/* the Chao plays motion 101 and leaves a mark on the ground 2 along its facing.
 * the mark is alife_favorite_sprite's frame for the character the Chao likes
 * most, or frame 6 when it likes none above 0. the mark is a child task,
 * created unchecked, which lasts as long as the Chao's task.
 *
 * the behavior starts only while bit 0 of the record's minimal is set. it also
 * needs a random draw to win, at a chance of curio + 110 in 210. no more than
 * 100 marks may have been made so far. where the draw or the count fails, it
 * tries again the next step. once its timer passes a limit of 500 to 799, the
 * Chao goes back to idle */
void alife_mark_favorite(struct task_work *work, struct alife_chao *chao);
/* motion 90 and voice 0x36e, with alife_face_look's x at 0x4000, until the
 * timer passes a limit of 300 to 599. it starts only while bit 4 of the
 * record's minimal is set */
void fun_0c912ba0(struct task_work *work, struct alife_chao *chao);
/* motion 5 and voice 0x388. once the timer passes a limit of 600 to 1199, the
 * Chao plays both again. it goes back to idle when the motion reaches its
 * end */
void fun_0c912ca8(struct task_work *work, struct alife_chao *chao);
/* motion 76, with the prop dat_0ca50650 at node 7 of the Chao's pose, for 501
 * to 800 steps. once the timer passes 60, motion 77 and voice 0x371 start at 1
 * in 100 each step, and the timer restarts. the behavior starts only while bit
 * 2 of the record's minimal is set. it also needs a random draw to win, at a
 * chance of curio + 110 in 210. where the draw fails, it tries again the next
 * step */
void fun_0c912daa(struct task_work *work, struct alife_chao *chao);
/* motion 142 and chatter, until the timer passes a limit of 180 to 379 as the
 * motion reaches its end. it starts only while bit 6 of the record's minimal is
 * set */
void fun_0c9130c6(struct task_work *work, struct alife_chao *chao);

#endif
