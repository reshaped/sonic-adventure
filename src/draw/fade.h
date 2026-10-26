/* fades: the screen to and from black, the game over and coming soon
 * screens, and the color that covers the screen */
#ifndef DRAW_FADE_H
#define DRAW_FADE_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* fade the screen in from black. fade_in_start stops the streams and holds
 * the screen black for five steps. fade_in_step returns 0 while the fade
 * moves, keeping the cover clear. it returns -1 otherwise: while it holds
 * the black, and once the fade is done */
void fade_in_start(void);
Sint16 fade_in_step(void);
/* fade the screen out to black. fade_out_start stops the streams and starts
 * the fade from the cover's alpha. fade_out_step holds it for one call, then
 * returns 0 until the screen is black. it then makes the cover and the back
 * color black and returns -1 */
void fade_out_start(void);
Sint16 fade_out_step(void);
/* mark a fade as running, or as done. each fade step clears the mark itself
 * once its fade is done, and takes a cleared mark as done */
void fade_start(void);
void fade_stop(void);

/* the game over. game_over_start starts a task that fades the screen to
 * black, 4 of 255 a step. it also starts the fade that fade_in_step and
 * fade_out_step move, from clear. game_over_step returns 0 while it moves
 * that fade toward black, 6 of 255 a step, and -1 once it is black. each
 * step after the -1 draws the game over screen and returns 0. the first of
 * them stops the streams, loads the textures and makes the back color black.
 * choice is 0 to continue and 1 to quit, and the screen marks the one
 * chosen. game_over_end releases the textures and leaves that fade at black */
void game_over_start(void);
Sint16 game_over_step(int choice);
void game_over_end(void);

/* the coming soon screen. coming_soon_start stops the streams, loads its
 * textures and draws it. coming_soon_step draws it and returns 0. its 120th
 * call returns -1 instead, once it releases the textures and makes the back
 * color black */
void coming_soon_start(void);
Sint16 coming_soon_step(void);

/* steps of tasks that fade the screen, 4 of 255 a step. each step draws the
 * black at the task's alpha as it fades, and so does the task's draw.
 * fade_task_black_and_back fades to black, waits 62 steps, in which only the
 * task's draw draws the black, then fades back. fade_task_from_black fades
 * from black. each task kills itself once its fade is done */
void fade_task_black_and_back(struct task *task);
void fade_task_from_black(struct task *task);

/* set or get the color that covers the screen, and draw it over the whole
 * screen unless its alpha is 0. fade_get_cover does nothing with a null
 * color */
void fade_set_cover(NJS_COLOR *color);
void fade_get_cover(NJS_COLOR *color);
void fade_draw_cover(void);

#endif
