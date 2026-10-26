/* jump panels, O JPanel: numbered panels that player 0 jumps along. each
 * jump flies the player to the next panel by number, and from the last to
 * panel 0 */
#ifndef OBJECT_PANEL_H
#define OBJECT_PANEL_H
#include <sg_xpt.h>

#include "system/task.h"

struct player;

/* forget every panel, for a new stage */
void panel_reset(void);
/* TRUE when work is player 0's and its collision record touched a panel
 * this step, else FALSE */
Bool panel_touched(struct task_work *work);
/* whether player 0 can jump from the panel it touches. it returns 1 on the
 * top shape of a numbered panel, -1 on panel 0, and 0 otherwise. a panel
 * numbered 2 or more returns 1 only in the player's first 30 steps touching
 * it. for player 0, a result other than 1 also clears every panel's mark as
 * left or next */
int panel_jump_ready_or_idle(struct task_work *work);
/* mark the panel the player touches as left, and the next by number as
 * next. after panel 9, or where no panel has the next number, the next is
 * panel 0. the player must touch a panel, and the next panel must exist */
void panel_jump(struct task_work *work);
/* fly the player a step toward the panel marked next, turning it the way
 * it flies. a step that would reach the panel stands the player on it
 * instead, clears every panel's mark, and flashes a spark. it returns the
 * distance to the panel before the step over 9, truncated: 9 is the
 * flight's least speed a step. it returns 0 once the player stands on the
 * panel. its search for the marked panel stops at the first empty number,
 * and a search that stops there returns 0 without a move. so with panels
 * 0, 2 and 3, a jump from 2 marks 3, but the player never flies */
int panel_fly(struct task_work *work, struct task_motion *motion,
              struct player *player);
/* the number of the panel marked left, which the player last jumped from,
 * or 0 when none is */
int panel_get_left(void);
/* the object lists' step for O JPanel. a panel's number is its scale x,
 * truncated. a scale x below 0, or of 10 or more, reads as 0 */
void panel_start(struct task *task);

#endif
