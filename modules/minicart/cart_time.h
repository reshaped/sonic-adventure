/* the race's time, from the steps the race counts to the minutes, seconds
 * and hundredths on screen */
#ifndef MINICART_CART_TIME_H
#define MINICART_CART_TIME_H

#include "save/minigame.h"

/* the hundredths of a second that steps last at 60 a second, rounded to the
 * nearest */
int cart_hundredths_of(int steps);
/* hundredths of a second as a time: its sign, and its magnitude in minutes,
 * seconds and hundredths. a magnitude of 100 minutes or more gives
 * 99:59.99 */
struct cart_time cart_time_of(int hundredths);

/* draw the clock's label, which the constant material doesn't color */
void cart_time_draw_label(void);
/* draw a time of hundredths as the clock shows it, in the constant
 * material's color */
void cart_time_draw(int hundredths);
/* draw the run's time as cart_time_draw does, yellow if it is under best.
 * the constant material is then white */
void cart_time_draw_final(int hundredths, int best);
/* draw the gap hundredths - best in a place of its own. a gain draws in
 * yellow after a minus. a loss draws in the constant material's color after
 * a plus. a tie draws with no sign. the constant material is then white */
void cart_time_draw_gap(int hundredths, int best);

#endif
