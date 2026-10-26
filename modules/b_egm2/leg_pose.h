/* the Egg Walker's leg copies and its close attack. the copies are its legs'
 * trees on the heap, posed apart from its motion. the attack stomps with
 * them */
#ifndef B_EGM2_LEG_POSE_H
#define B_EGM2_LEG_POSE_H
#include "system/task.h"

/* copy the boss's four leg trees onto the heap, and save each copy's rest
 * pose. the copies share their models with the boss's legs. nothing checks
 * the allocations, and nothing frees the copies */
void egg_walker_leg_copies_init(void);
/* draw the shown copies in the boss's frame, on the body's matrix that
 * egg_walker_parts_update last kept. pad 0's up and down presses also
 * raise and lower all four copies, shown or not */
void egg_walker_leg_copies_draw(struct task *task);
/* move each blending copy one step of its blend. once a blend that
 * egg_walker_legs_walk or egg_walker_legs_rest began ends, the copies hide.
 * the boss's own legs then show again. egg_walker_leg_copies_init must have
 * run */
void egg_walker_leg_pose_step(void);
/* lift the three shapes of each leg whose copy is shown. the lift grows to
 * 35 over a raise, and falls back to 0 over a lower. call it after each
 * egg_walker_parts_update, which places the shapes on the boss's own pose
 * again. work is the boss's */
void egg_walker_leg_shapes_lift(struct task_work *work);
/* the boss's step in EGG_WALKER_CLOSE. it stomps, and each leg it stomps
 * with glows. once every glow is hit, the boss falls and its cockpit opens
 * to hits. task is the boss's, and player 0 must exist */
void egg_walker_close(struct task *task);
/* blend every copy to its walking pose, or to its rest pose, over 15 steps.
 * a walk turns each angle the short way round. the copies show in place of
 * the boss's legs until the blend ends */
void egg_walker_legs_walk(void);
void egg_walker_legs_rest(void);
/* 1 from a call of egg_walker_legs_walk or egg_walker_legs_rest until its
 * blend ends, else 0. a raise or a lower in between returns it to 0 */
int egg_walker_all_legs_blending(void);

#endif
