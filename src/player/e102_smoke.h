/* E-102's smoke: puffs that grow and rise, from a pool of their own */
#ifndef PLAYER_E102_SMOKE_H
#define PLAYER_E102_SMOKE_H
#include <ninja.h>
#include <sg_xpt.h>

/* a puff at pos, moving at speed, at scale, if the pool has a free
 * particle: none before e102_smoke_init. it starts at a random point in
 * its first four frames */
void e102_smoke_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale);
/* make the pool the puffs come from */
void e102_smoke_init(void);

#endif
