/* a spray of rings that fall and bounce, and turn into gold coins as they
 * first bounce */
#ifndef STG09_COIN_SPRAY_H
#define STG09_COIN_SPRAY_H
#include <ninja.h>
#include <sg_xpt.h>

/* add a piece to the spray. its velocity is velocity, each component varied
 * by up to spread either way at random. it starts a step on from pos, and
 * falls. it bounces as it crosses the height ground at a vertical speed over
 * 4 units a step. it draws at scale: as a ring, then from its first bounce as a
 * gold coin. it lasts 181 steps. the add returns FALSE when the spray's task
 * or the piece can't be allocated, and TRUE otherwise.
 *
 * one task runs every piece, made by an add when none runs. the task ends on
 * the step after one that finds no piece, and frees any piece added between */
Bool coin_spray_add(NJS_POINT3 *pos, NJS_VECTOR *velocity, Float spread,
                    Float ground, Float scale);

#endif
