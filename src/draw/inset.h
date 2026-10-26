/* the inset: a box on the screen that draws the stage again, looking down
 * on player 1 */
#ifndef DRAW_INSET_H
#define DRAW_INSET_H
#include <sg_xpt.h>

struct task;

/* how inset_clip limits ninja's drawing: not at all, to the full screen
 * outside the inset, or to the inset alone */
enum inset_clipping { INSET_CLIP_NONE, INSET_CLIP_AROUND, INSET_CLIP_INSIDE };

/* limit ninja's drawing as clip says, and set ninja's screen to match. the
 * limit is in njUserClipping's tiles of 32 pixels a side: those the inset
 * covers within the full screen. for INSET_CLIP_INSIDE, ninja's screen takes
 * the inset's center and size, and the tiles are filled white. for the
 * other two, it takes the full screen's. while the inset is down, every
 * clip acts as INSET_CLIP_NONE */
void inset_clip(enum inset_clipping clip);
/* ask for the inset, stop asking for it, or flip the ask. the inset goes up
 * or down at the next inset_update */
void inset_show(void);
void inset_hide(void);
void inset_toggle(void);
/* put the inset up if it's asked for, or else take it down. TRUE if it's
 * up. inset_clip clips and inset_draw draws only while it's up */
Bool inset_update(void);
/* set the width and height of the full screen, for INSET_CLIP_NONE and
 * INSET_CLIP_AROUND, or of the inset, for INSET_CLIP_INSIDE. each keeps its
 * center. the full screen starts at SCREEN_WIDTH by SCREEN_HEIGHT. the
 * inset starts at a quarter of each, centered three quarters of the way
 * across and down. inset_resize is the same code as inset_set_size */
void inset_set_size(enum inset_clipping clip, Float width, Float height);
void inset_resize(enum inset_clipping clip, Float width, Float height);
/* create a task with a work on level 0, and keep it. inset_step_task calls
 * the kept task's step once more, besides the call level 0's walk makes. it
 * skips a null task or step. the kept task isn't cleared when the task ends,
 * so inset_step_task must not run after that */
void inset_create_task(void (*step)(struct task *task));
void inset_step_task(void);
/* while the inset is up, draw every task level but 0 again. the view looks
 * straight down from 100 above player 1, with a 30-degree field of view.
 * player 1 must be there. it puts back the screen distance that
 * njSetPerspective changes */
void inset_draw(void);

#endif
