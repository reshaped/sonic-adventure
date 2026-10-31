/* the past's stage task and sky task, its first texture list, and O SNAKE's
 * model */
#ifndef ADV03_PAST_H
#define ADV03_PAST_H

#include <ninja.h>

#include "system/task.h"

/* the Past's acts, as stage_get_stage_act numbers them. the stage select names
 * them PAST KNUCK, PAST CHAOS and PAST FIRE */
enum { PAST_KNUCKLES, PAST_CHAOS, PAST_FIRE };

struct death_zone;

/* the zones a player goes down in, a list for each act, ended by a null
 * object. 1ST_READ.BIN's death_zones holds the array's address for
 * the past. the stage's generated data holds them */
extern struct death_zone *past_death_zones[];

/* the steps of the stage task and the sky task, which 1ST_READ.BIN's
 * stage_steps and sky_steps name for the past. when the act changes, each ends
 * what it started for the old act and starts the new act's */
void past_step(struct task *task);
void past_sky_step(struct task *task);

/* OBJ_PAST's texture list, past_pvms' first. texture_load_stage loads it
 * into the stage's first slot, so texture_set_stage makes it current */
extern NJS_TEXLIST obj_past_texlist;

/* the skies' texture lists, act 0's and act 1's, which act 2 shares.
 * 1ST_READ.BIN's past_act_pvms names them, for MR_SKY02 and MR_SKY00 */
extern NJS_TEXLIST sky_act0_texlist;
extern NJS_TEXLIST sky_act1_texlist;

/* the model O SNAKE's task draws */
extern NJS_OBJECT pyramid_snake_object;

#endif
