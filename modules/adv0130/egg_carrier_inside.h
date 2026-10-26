/* the field and sky tasks of the Egg Carrier's inside stage, and the texture
 * lists the stage and its acts load */
#ifndef ADV0130_EGG_CARRIER_INSIDE_H
#define ADV0130_EGG_CARRIER_INSIDE_H
#include <ninja.h>

#include "system/task.h"
#include "system/texture.h"

/* the field task's step and the sky task's, which 1ST_READ.BIN's stage_steps
 * and sky_steps name by address. on entering each act, the field task starts
 * the act's tasks and music, and the sky task sets the act's fog */
void egg_carrier_inside_step(struct task *task);
void egg_carrier_inside_sky_step(struct task *task);

/* the stage's PVM list and its acts', which texture_load_stage and
 * texture_load_act reach by address through 1ST_READ.BIN's stage_pvms and
 * adv0130_acts */
extern struct texture_pvm egg_carrier_inside_pvms[];
extern struct texture_pvm egg_carrier_inside_act_pvms[];

/* texlists in ADV0130, valid only while it is loaded. texture_load_stage
 * fills obj_ec30_texlist and ec_tarai_texlist, and texture_load_act fills
 * ec_water_texlist for act 4 */
extern NJS_TEXLIST obj_ec30_texlist;
extern NJS_TEXLIST ec_water_texlist;
extern NJS_TEXLIST ec_tarai_texlist;

#endif
