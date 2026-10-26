/* the animals player 0 brings into a garden, which run loose there */
#ifndef AL_MAIN_ANIMAL_H
#define AL_MAIN_ANIMAL_H
#include <ninja.h>
#include <sg_xpt.h>

/* make an animal of kind, an enum animal_kind, at pos, moving at velocity
 * and facing ang. it starts thrown, and goes loose once it settles. a kind
 * out of range makes ANIMAL_MOGU. the allocations of its task, its record
 * and its entry are unchecked */
void alife_animal_create(NJS_POINT3 *pos, NJS_VECTOR *velocity, Angle ang,
                         Sint8 kind);

#endif
