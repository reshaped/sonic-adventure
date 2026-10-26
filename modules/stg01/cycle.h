/* Emerald Coast's texture cycles. each steps the materials of an act's
 * sand, waves or water through a run of textures */
#ifndef STG01_CYCLE_H
#define STG01_CYCLE_H

#include "system/task.h"

/* the cycles' steps, whose tasks beach_act_start creates as each act
 * starts. each task needs a work. outside its own act, a step kills its
 * task */
void beach_act0_sand_step(struct task *task);
void beach_act0_wave_step(struct task *task);
void beach_act1_wave_step(struct task *task);
void beach_act1_sand_step(struct task *task);
void beach_act1_water_step(struct task *task);
void beach_act2_wave_step(struct task *task);
void beach_act2_sand_step(struct task *task);
void beach_act2_water_step(struct task *task);

#endif
