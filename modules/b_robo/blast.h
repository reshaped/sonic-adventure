/* a blast like the Egg Walker's: a ball of fire that swells and fades,
 * with beams swinging out. nothing makes one */
#ifndef B_ROBO_BLAST_H
#define B_ROBO_BLAST_H

#include "b_egm2/blast.h"

/* make a blast as params says. the blast copies *params. without
 * BLAST_COPY_POS, it follows params->pos, which must outlive it. beam_end
 * must differ from beam_start, since the blast divides by their difference
 * unchecked */
void erobo_blast_create(struct blast_params *params);

#endif
