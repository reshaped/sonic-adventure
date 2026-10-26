/* the explosion an E-100 series robot goes down in: rays, bombs, and a
 * sphere that grows and fades */
#ifndef STG05_EXPLOSION_H
#define STG05_EXPLOSION_H
#include <ninja.h>
#include <sg_xpt.h>

/* an explosion as e100_explosion_create takes it. its times count the
 * explosion's steps, 1 being its first */
struct explosion_params {
    /* the point it follows, or with EXPLOSION_STILL the point it stays at */
    NJS_POINT3 *pos;
    int flags;
    /* it casts rays on the steps after ray_start and before ray_end. their
     * length swells from 0 to ray_length and back over that span */
    int ray_start;
    int ray_end;
    Float ray_length;
    Angle ray_turn; /* the most a ray swings a step, about each axis */
    /* the sphere grows to radius from grow_start up to grow_end, then by 5%
     * less each step. its light fades out from fade_start to fade_end. the
     * explosion ends at the later of ray_end and fade_end */
    int grow_start;
    int grow_end;
    int fade_start;
    int fade_end;
    Float radius; /* the sphere's radius at grow_end */
};

/* the bits of flags */
#define EXPLOSION_STILL 0x1 /* stays where *pos was at the create */
/* its sphere can hurt the players, enemies and objects it touches */
#define EXPLOSION_HURTS 0x2

/* create an explosion from a copy of params. it creates none when it can't
 * allocate its task or its record. without EXPLOSION_STILL it keeps
 * params->pos, which must stay valid while the explosion lives. ray_end must
 * not equal ray_start. each of its steps sets the camera sweep's reach to
 * the sphere's radius */
void e100_explosion_create(struct explosion_params *params);

#endif
