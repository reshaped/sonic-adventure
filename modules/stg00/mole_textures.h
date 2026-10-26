/* the textures and texlist of the Hedgehog Hammer's machine, and the first
 * piece of Eggman's model. STG00 and ADV0130 each link this object */
#ifndef STG00_MOLE_TEXTURES_H
#define STG00_MOLE_TEXTURES_H

#include <ninja.h>

/* the machine's texlist, which the PVM MOGURATATAKI fills. the machine, its
 * targets, their pieces and the board's letters draw with it */
extern NJS_TEXLIST mole_texlist;
/* the first piece Eggman's target breaks into: its model, and the object
 * that holds it */
extern NJS_MODEL mole_eggman_piece_0_model;
extern NJS_OBJECT mole_eggman_piece_0_object;

#endif
