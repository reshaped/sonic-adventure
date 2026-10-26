/* the blasts E-101 mkII's beam sets off, as the Egg Walker's blast_params
 * describe them */
#ifndef B_E101_R_BLAST_H
#define B_E101_R_BLAST_H

#include "b_egm2/blast.h"

/* set off a blast as params describes it. E-102 can lock on to the blast.
 * *params is copied. without BLAST_COPY_POS, params->pos is kept and must
 * outlive the blast. with BLAST_HURTS, the ball hurts players until it
 * touches one or begins to fade. params->beam_end must differ from
 * params->beam_start: the blast divides by their difference unchecked.
 *
 * no blast starts when its task or its record can't be allocated. its other
 * allocations go unchecked. without its shapes' copy,
 * e101_mk2_blast_create writes through null. without its lock-on record,
 * enemy_alloc_record kills its task, which ends on its next step */
void e101_mk2_blast_create(struct blast_params *params);

#endif
