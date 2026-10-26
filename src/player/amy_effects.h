/* Amy's effects: hearts, the chicks round her head as she's dizzy, and her
 * hammer's shock */
#ifndef PLAYER_AMY_EFFECTS_H
#define PLAYER_AMY_EFFECTS_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* fling eight hearts out from pos, spread round a cone that ang_x and ang_z
 * tilt. they fall, spin and fade out over 30 steps */
void amy_heart_burst(NJS_POINT3 *pos, Angle ang_x, Angle ang_z);
/* create a heart at pos that drifts off a random way, slowing, and fades in
 * and out over 15 steps */
void amy_heart_create(NJS_POINT3 *pos);
/* the step of a task with TASK_WORK that draws chicks round player 0's head
 * while amy_get_action(0) gives AMY_DIZZY. after that, the chicks shrink
 * away and the task ends. it ends at once when player 0 has no task */
void amy_chicks_step(struct task *task);
/* the steps of the hammer's two shocks, for a task with TASK_WORK whose
 * work->pos places it and whose work->ang.x and work->ang.z tilt it. the
 * ring and the wave, a model, spread and swell along y, and the ring fades.
 * each lasts 30 steps, or less once placement_try_leave finds it out of
 * range */
void amy_shock_ring_step(struct task *task);
void amy_shock_wave_step(struct task *task);

#endif
