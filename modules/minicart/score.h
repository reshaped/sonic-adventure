/* the race's end: the score screen, the race's mail, then the game over's
 * choice */
#ifndef MINICART_SCORE_H
#define MINICART_SCORE_H

/* end the race. the camera holds where it is, at CAMERA_LEVEL_COLI,
 * cart_race_over becomes TRUE, and a level-3 task runs the rest:
 *
 * - after 181 steps, the task starts the score screen, score_step
 * - once that ends, outside act 0, the task writes the race's mail with
 *   save_write_cart_mail: the act, the character and the best time
 *   cart_copy_times gives
 * - then the task offers the game over's choice. continuing ends the act
 *   with STAGE_END_RESTART, and quitting ends it with STAGE_END_CLEAR
 *
 * the task disables the pause at each step */
void cart_score_start(void);
/* mark the score screen as ended, so the game over's choice can follow.
 * 1ST_READ.BIN's score screen calls it at its address in the module */
void cart_score_done(void);

#endif
