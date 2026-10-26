/* Casinopolis water smoke particles, with a pool of their own */
#ifndef STG09_WATER_SMOKE_H
#define STG09_WATER_SMOKE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/dust.h"
#include "effect/pools.h"

/* 1ST_READ.BIN's casino_pools names it by its address */
extern struct particle_pool_spec water_smoke_pool;

/* each create takes its puff from water_smoke_pool. it makes none when the
 * pool is null or no particle is free. water_smoke_create's puff starts at
 * pos, moving at speed, at scale, and moves by the file's own params */
void water_smoke_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale);
/* a puff at params' pos, speed and scale, in params' color. it fades in to
 * params' alpha and moves by params. it keeps the pointer, not a copy, so
 * params must outlive the puff */
void water_smoke_create_from(struct dust_params *params);

#endif
