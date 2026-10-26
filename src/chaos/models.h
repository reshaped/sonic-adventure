/* what barrier.c defines: Chaos 2's barrier, and the texlists of the Chaos
 * bosses' effects, of the emeralds and of Chaos's surface */
#ifndef CHAOS_MODELS_H
#define CHAOS_MODELS_H
#include <ninja.h>

extern NJS_OBJECT chaos_barrier_object;
/* the textures the Chaos stages load from CHAOS_EFFECT, for the bosses'
 * effects */
extern NJS_TEXLIST chaos_effect_texlist;
/* the textures loaded from CHAOS_EMERALD: three glints of each of the seven
 * emeralds, slot by slot */
extern NJS_TEXLIST chaos_emerald_texlist;
/* the names of CHAOS_SURFACE's textures: the six frames of water that
 * chaos_animate_surface flows Chaos's surface through */
extern NJS_TEXNAME chaos_surface_textures[];
extern NJS_TEXLIST chaos_surface_texlist;

#endif
