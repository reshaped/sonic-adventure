/* select: the stage select, which picks a character, a stage and an act
 * outside the story */
#ifndef PROGRESS_SELECT_H
#define PROGRESS_SELECT_H
#include <sg_xpt.h>

/* what stage_select_step returns as the selector ends. it returns 0 while
 * the selector runs */
enum select_step {
    SELECT_STEP_PICKED = 1, /* the character, stage and act are set */
    SELECT_STEP_RESET = -2 /* a pad held a, b, x, y and start together */
};

/* start the selector on a fresh heap and task tree, with the pads off */
void stage_select_start(void);
/* run a step of the selector. it returns 0 while the selector runs, then
 * SELECT_STEP_PICKED once a pick has faded out. once a pad holds a, b, x, y
 * and start together, it destroys every task and returns SELECT_STEP_RESET */
Sint16 stage_select_step(void);

#endif
