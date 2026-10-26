/* the title: its picture, the cards that choose one of six characters, each
 * character's guide, and a debug menu */
#ifndef PROGRESS_TITLE_H
#define PROGRESS_TITLE_H
#include <sg_xpt.h>

/* what title_step returns once the title ends. it returns 0 before then */
enum title_result {
    TITLE_STAGE_SELECT = -1, /* x and y held, which ask for the stage select */
    TITLE_PLAY = 1 /* a stage is set to play */
};

/* call menu_begin, then start the title at its picture on a fresh heap and
 * task tree. it sets draw_off to TRUE, recording_kind to RECORDING_OFF and
 * the fade to black, and paces two vsyncs and two steps to a frame */
void title_setup(void);
/* run a frame of the title, or of its debug menu. return 0 while it runs,
 * then a title_result */
Sint16 title_step(void);
/* send the title back to its picture, and the debug menu back to its first
 * rows at full brightness. no code calls it */
void title_debug_reset(void);
/* the fade over the screen: black at a level of 0 or less, and clear at
 * 255 or more. title_fade_set sets the level */
void title_fade_set(int level);
/* add step to the fade's level and return 0. a step down from below -30
 * instead sets the level to 0 and returns -1. a step up from above 255
 * sets it to 255 and returns 1 */
int title_fade_step(int step);
/* draw the fade in black over the screen */
void title_fade_draw(void);

#endif
