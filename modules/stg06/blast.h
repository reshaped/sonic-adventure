/* the blasts and bursts the cannons' shells land with */
#ifndef STG06_BLAST_H
#define STG06_BLAST_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* a blast at pos: a ball that hurts players. it draws a ring, a fireball, a
 * pillar and a cloud about the ball. as it starts, it throws sparks and sets
 * off an explosion. scale sizes what it draws, its sparks and its explosion,
 * but not the ball that hurts. ang_x and ang_z tilt it to lie on its floor,
 * and sky_deck_blast_create_level leaves it untilted. null when no task can
 * be had */
struct task *sky_deck_blast_create_level(NJS_POINT3 *pos, Float scale);
struct task *sky_deck_blast_create(NJS_POINT3 *pos, Float scale, Angle ang_x,
                                   Angle ang_z);
/* a burst at pos: a ball half a blast's, which hurts players and which
 * nothing draws. as it starts, it sets off a puff and an explosion, which
 * scale sizes. null when no task can be had */
struct task *sky_deck_burst_create(NJS_POINT3 *pos, Float scale);

#endif
