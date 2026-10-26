/* explosions: a blast's sprite, and the sparks that fly from it */
#ifndef EFFECT_EXPLOSION_H
#define EFFECT_EXPLOSION_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/pools.h"

struct task_work;

/* exp01 to exp08 */
extern NJS_TEXLIST explosion_texlist;
extern struct particle_pool_spec explosion_pool;

/* an explosion at work's position, or for explosion_create_at at x, y, z.
 * each plays the blast's sound and shows the blast at scale. the explosion
 * then throws three sparks a step for 17 steps */
void explosion_create(struct task_work *work, Float scale);
void explosion_create_at(Float x, Float y, Float z, Float scale);

#endif
