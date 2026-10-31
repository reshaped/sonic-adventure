/* poms, the bursts of textures pom01 to pom08 that play once where they're
 * made; the targets' box and boards; and the list of the particle pools Final
 * Egg adds */
#ifndef STG10_POM_H
#define STG10_POM_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/pools.h"

/* the box a pop-up target's board rises from, which also stands alone,
 * shut */
extern NJS_OBJECT target_box_object;

/* the boards with Sonic, Knuckles and Tails on them. a board is drawn as its
 * pieces, one for each node of its tree */
extern NJS_OBJECT target_sonic_object;
extern NJS_OBJECT target_knuckles_object;
extern NJS_OBJECT target_tails_object;

/* Final Egg's own particle pools, ending in NULL, which particle_create_pools
 * makes after those every stage has. stage_pools in effect/pools.c names the
 * list */
extern struct particle_pool_spec *final_pools[];

/* the poms' texlist. 1ST_READ.BIN's final_pvms names it, for
 * EFF_FINALEGG_POM's textures */
extern NJS_TEXLIST final_pom_texlist;

/* make a pom at pos and scale. it makes none when its pool is null or no
 * particle is free */
void final_pom_create(NJS_POINT3 *pos, Float scale);

#endif
