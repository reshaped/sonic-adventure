/* Chaos 4's sweep, an arm he stretches out over the swamp and swings
 * round */
#ifndef B_CHAOS4_SWEEP_H
#define B_CHAOS4_SWEEP_H
#include <ninja.h>
#include <sg_xpt.h>

#include "chaos/chaos.h"
#include "system/task.h"

/* his step in BOSS_SWEEP, which his task's step calls */
void chaos4_sweep(struct task_work *work, struct task_motion *motion,
                  struct chaos *chaos);
/* set NJD_EVAL_HIDE in the evalflags of object, its children and its
 * later siblings, all the way down, or clear it when hide is FALSE */
void chaos4_hide_tree(NJS_OBJECT *object, Bool hide);
/* step the swept arm's springs once, and draw the arm from the current
 * matrix, which must be its shoulder's. his draw calls it on a frame's
 * first step while DRAW_ARM is set */
void chaos4_sweep_draw(struct task_work *work);
/* while DRAW_ARM is set, put the centers of work's shapes 4 and 5 at the
 * swept arm's last two joints, in the world */
void chaos4_sweep_shapes(struct task_work *work);

#endif
