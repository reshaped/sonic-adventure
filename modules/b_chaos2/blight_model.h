/* the model blight.c draws */
#ifndef B_CHAOS2_BLIGHT_MODEL_H
#define B_CHAOS2_BLIGHT_MODEL_H

#include <ninja.h>

/* blight_object's one material, which blight_draw gives each texture in
 * turn. it is an array, as the model's mats points at it. SHC pools the
 * array's address and stores attr_texId at offset 12 from it (MOV.L
 * R0,@(12,R3) at 0x0c9067fc) */
extern NJS_MATERIAL blight_materials[];
extern NJS_OBJECT blight_object;

#endif
