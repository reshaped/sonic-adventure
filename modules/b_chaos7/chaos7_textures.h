/* Perfect Chaos's textures and the water's, with their texlists, and the
 * shape motion of his forming figure */
#ifndef B_CHAOS7_CHAOS7_TEXTURES_H
#define B_CHAOS7_CHAOS7_TEXTURES_H

#include <ninja.h>

/* the shape motion the boss plays on his forming figure */
extern NJS_MOTION form_shape;
/* the texlist chaos7_act_pvms loads CHAOS7_0 into. the boss, his breath beams
 * and his tornadoes draw with it */
extern NJS_TEXLIST chaos7_texlist;
/* four rows of four frames: c7mizu, c7_umibody, mizu_nagare and nagare, each
 * _0 to _3. the boss copies each row's frame into a slot of chaos7_texlist */
extern NJS_TEXNAME water_textures[];
/* the texlist over water_textures, which chaos7_act_pvms loads
 * CHAOS7_0SURFACE into */
extern NJS_TEXLIST chaos7_surface_texlist;

#endif
