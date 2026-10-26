/* E-101 mkII's files, E101R.NB and E101R.PB, and the actions and trail
 * objects that point into E101R.NB */
#ifndef B_E101_R_FILES_H
#define B_E101_R_FILES_H
#include <ninja.h>
#include <sg_xpt.h>

/* where e101_r_load reads E101R.NB, and the texlist of E101R.PB's textures.
 * both are null before the load, and the texlist again after the release */
extern Uint8 *e101_r_models;
extern NJS_TEXLIST *e101_r_texlist;
/* the object of the boss that leans out of a turn, as a byte offset in
 * E101R.NB */
#define SWAY_OBJECT 0x8238

/* the object of the light each arm trails, and the boss's actions. each is
 * zero until e101_r_load points it into E101R.NB */
extern NJS_OBJECT e101_mk2_trail_0_object;
extern NJS_OBJECT e101_mk2_trail_1_object;
extern NJS_ACTION e101_mk2_dash_action;
extern NJS_ACTION e101_mk2_swat1_action;
extern NJS_ACTION e101_mk2_swat0_action;
extern NJS_ACTION e101_mk2_dash_end_action;
extern NJS_ACTION e101_mk2_hurt_action;
extern NJS_ACTION e101_mk2_hurt_dashing_action;
extern NJS_ACTION e101_mk2_beam_action;
extern NJS_ACTION e101_mk2_defeat_action;
extern NJS_ACTION e101_mk2_hover_action;

/* read E101R.PB to the first 32-byte boundary past the module's bss, and
 * load its textures. then read E101R.NB over it, and relocate its models and
 * motions. point the actions and the trails' objects into them. no read or
 * relocation is checked. e101_r_texlist is null when pvb_load returns
 * null */
void e101_r_load(void);
/* release E101R.PB's textures, and clear e101_r_texlist. e101_r_models
 * still points where E101R.NB was read */
void e101_r_release(void);

#endif
