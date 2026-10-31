/* the claw's textures, which the events that use it load from MGHAND */
#ifndef SHARED_CLAW_TEXTURES_H
#define SHARED_CLAW_TEXTURES_H

#include <ninja.h>
#include <sg_xpt.h>

extern NJS_TEXLIST event_claw_texlist;
/* the materials of a model in the generated data of each event that links
 * them, and the strips of that model's first three meshsets */
extern NJS_MATERIAL event_claw_materials[4];
extern Sint16 event_claw_strips_0[20];
extern Sint16 event_claw_strips_1[23];
extern Sint16 event_claw_strips_2[31];

#endif
