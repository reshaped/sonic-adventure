/* the past's stage task and sky task, its first texture list, and O SNAKE's
 * model */
#ifndef ADV03_PAST_H
#define ADV03_PAST_H

#include <ninja.h>

#include "system/task.h"

/* the Past's acts, as stage_get_stage_act numbers them. the stage select names
 * them PAST KNUCK, PAST CHAOS and PAST FIRE */
enum { PAST_KNUCKLES, PAST_CHAOS, PAST_FIRE };

/* the steps of the stage task and the sky task, which 1ST_READ.BIN's
 * stage_steps and sky_steps name for the past by address. when the act
 * changes, each ends what it started for the old act and starts the new
 * act's */
void past_step(struct task *task);
void past_sky_step(struct task *task);

/* OBJ_PAST's texture list, past_pvms' first. texture_load_stage loads it
 * into the stage's first slot, so texture_set_stage makes it current */
extern NJS_TEXLIST obj_past_texlist;

/* the model O SNAKE's task draws */
extern NJS_OBJECT pyramid_snake_object;

#endif
