/* the frame timer, on the SH-4's timer 1, with the debug page and draw_off.
 * every time here is in thousandths of a vsync: 1000 is 1/60 s */
#ifndef SYSTEM_TIMER_H
#define SYSTEM_TIMER_H
#include <sg_xpt.h>

/* the debug page drawn over the game, or 0 for none */
extern int debug_page;
/* while TRUE, frame_wait renders nothing and flips no buffer */
extern Bool draw_off;

/* restart the timer at 0, with the mark there. a second later the timer
 * starts over at 0. timer_start also sets the callback that records the end
 * of each Kamui render. it sets timer_show_counts's counts to -1, and
 * draw_debug_counts to 0 */
void timer_start(void);
/* the time since the mark, which moves to now */
Float timer_lap(void);
/* move the mark to now */
void timer_mark(void);
/* the time since timer_start */
Float timer_elapsed(void);
/* the time since timer_start, and the mark moves to now */
Float timer_elapsed_and_mark(void);
/* the time from timer_start to the end of Kamui's last render */
Float timer_render_time(void);
/* while debug_page is set, print each count timer_start sets to -1 that is
 * now above 0, as the count plus 1. nothing else writes the counts, so
 * nothing prints */
void timer_show_counts(void);

#endif
