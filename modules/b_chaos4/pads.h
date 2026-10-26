/* Chaos 4's lily pads, and the stalks that sway beside them */
#ifndef B_CHAOS4_PADS_H
#define B_CHAOS4_PADS_H
#include <ninja.h>
#include <sg_xpt.h>

/* the pads, one at each of the places in pad_places, which Chaos 4 swims
 * between */
#define PADS 13
/* @hack type: pad_places declared without its extent, which has boss.c's
 * swim widen the byte index before it scales it by 12 (EXTU.B R5,R4 at
 * 0x0c900740); declared with PADS, 132 of 199 words differ */
extern NJS_POINT3 pad_places[];
/* the index of the pad a player last touched, or PADS for none. it starts
 * at 0, as if pad 0 had been touched. boss.c's choose_pad, which picks the
 * pad Chaos 4 swims to, reads it and sets it to PADS */
extern Uint8 pad_stood_on;

/* make a pad at each of pad_places, and a stalk near each. call it after
 * swamp_create: a stalk made without the waves rides their point 0 */
void pads_create(void);

#endif
