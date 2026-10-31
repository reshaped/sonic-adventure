/* the field and sky tasks of the Egg Carrier's inside stage, and the texture
 * lists the stage and its acts load */
#ifndef ADV0130_EGG_CARRIER_INSIDE_H
#define ADV0130_EGG_CARRIER_INSIDE_H
#include <ninja.h>

#include "system/task.h"
#include "system/texture.h"

struct death_zone;

/* the zones a player goes down in, a list for each act, ended by a null
 * object. 1ST_READ.BIN's death_zones holds the array's address for
 * inside the Egg Carrier. the stage's generated data holds them */
extern struct death_zone *inside_death_zones[];

/* the field task's step and the sky task's, which 1ST_READ.BIN's stage_steps
 * and sky_steps name. on entering each act, the field task starts the act's
 * tasks and music, and the sky task sets the act's fog */
void egg_carrier_inside_step(struct task *task);
void egg_carrier_inside_sky_step(struct task *task);

/* the stage's PVM list and its acts', which texture_load_stage and
 * texture_load_act reach through 1ST_READ.BIN's stage_pvms and adv0130_acts */
extern struct texture_pvm egg_carrier_inside_pvms[];
extern struct texture_pvm egg_carrier_inside_act_pvms[];

/* texlists in ADV0130, valid only while it is loaded. texture_load_stage
 * fills obj_ec30_texlist and ec_tarai_texlist, and texture_load_act fills
 * ec_water_texlist for act 4 */
extern NJS_TEXLIST obj_ec30_texlist;
/* ADV0100 defines a texlist of this name too, which adv0100/texlists.h
 * declares, so a file that includes both headers declares it twice */
/* NOLINTNEXTLINE(readability-redundant-declaration) */
extern NJS_TEXLIST ec_water_texlist;
extern NJS_TEXLIST ec_tarai_texlist;
/* the texlists that the acts' lands in the generated data name */
extern NJS_TEXLIST adv_ec30_texlist;
extern NJS_TEXLIST adv_ec31_texlist;
extern NJS_TEXLIST adv_ec32_texlist;
extern NJS_TEXLIST adv_ec33_texlist;
extern NJS_TEXLIST adv_ec34_texlist;
extern NJS_TEXLIST adv_ec35_texlist;

#endif
