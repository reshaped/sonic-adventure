/* jet smoke clouds that the avalanche's first wave raises */
#ifndef STG08_JET_SMOKE_H
#define STG08_JET_SMOKE_H
#include <ninja.h>

/* create a cloud at pos whose animation starts at frame frame. height scales
 * the height of its sprite, and height plus spread its width. no cloud
 * starts when no task is free */
void jet_smoke_create(NJS_POINT3 *pos, Float height, Float frame, Float spread);

#endif
