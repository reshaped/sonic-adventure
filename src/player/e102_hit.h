/* E-102's hit: a flash, and a burst of sparks, on E102HIT's textures */
#ifndef PLAYER_E102_HIT_H
#define PLAYER_E102_HIT_H
#include <ninja.h>
#include <sg_xpt.h>

/* E102HIT's textures, which the flash and the sparks play through */
extern NJS_TEXLIST e102hit_texlist;

/* bind the flash's frames to E102HIT's textures. with no sparks' pool,
 * bind the sparks' frames too and make one */
void e102_hit_init(void);
/* end the sparks' pool */
void e102_hit_free(void);
/* a flash at pos, and a burst of 20 sparks from there if the pool is made.
 * both add depth to their distance as they draw */
void e102_hit_create(NJS_POINT3 *pos, Float depth);

#endif
