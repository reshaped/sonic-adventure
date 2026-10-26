/* Zero's last explosion. beams swing out of it, and a ball of fire swells
 * round a column of rings and fades */
#ifndef B_ROBO_EXPLOSION_H
#define B_ROBO_EXPLOSION_H

#include "b_egm2/blast.h"

/* make an explosion as params says, which hurts nothing. the explosion
 * copies *params, but follows params->pos, which must outlive it: it
 * ignores BLAST_COPY_POS. beam_end must differ from beam_start, since the
 * explosion divides by their difference unchecked. each step it sets the
 * fight camera's reach to the ball's size. from fade_start, it sets
 * EROBO_CAMERA_CIRCLE_OUT. when it can't allocate, it makes nothing */
void erobo_explosion_create(struct blast_params *params);

#endif
