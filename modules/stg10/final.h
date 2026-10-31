/* Final Egg stage task, the sky task that sets each act's fog, and a light
 * task */
#ifndef STG10_FINAL_H
#define STG10_FINAL_H

#include "system/task.h"

struct death_zone;

/* the zones a player goes down in, a list for each act, ended by a null
 * object. 1ST_READ.BIN's death_zones holds the array's address for
 * Final Egg. the stage's generated data holds them */
extern struct death_zone *final_death_zones[];
#ifdef GCC
struct path;

/* the lists 1ST_READ.BIN's path_lists names for the stage's acts 0 and 1.
 * each ends with a null, where path_create_tasks stops. act2_paths.h declares
 * act 2's. the stage's generated data holds them */
extern struct path *final_act0_paths[];
extern struct path *final_act1_paths[];
#endif

/* task steps that stage_steps and sky_steps name for the stage */
void final_step(struct task *task);
void final_sky_step(struct task *task);
/* a light task's step, which neither this image nor 1ST_READ.BIN points at.
 * its first step makes and turns on a directional light of its own. each
 * later step gives the stage's dir_light its own light's intensities, color
 * and direction */
void final_light_step(struct task *task);

#endif
