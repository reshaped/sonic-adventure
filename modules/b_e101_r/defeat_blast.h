/* a blast for the boss's defeat. nothing makes one: no image holds
 * e101_mk2_defeat_blast_create's address, and no branch in B_E101_R
 * reaches it */
#ifndef B_E101_R_DEFEAT_BLAST_H
#define B_E101_R_DEFEAT_BLAST_H
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm2/blast.h"

/* burst a puff at scale, radius from pos in a random direction. nothing
 * calls it: no image holds its address, and no branch in B_E101_R reaches
 * it */
void e101_mk2_defeat_blast_scatter(NJS_POINT3 *pos, Float radius, Float scale);
/* start a defeat blast with params' pos, beams and end. params' other fields
 * go unread. *params is copied, but params->pos is kept and read each step,
 * so it must outlive the blast. params->beam_end must differ from
 * params->beam_start: the blast divides by their difference unchecked. each
 * step, the blast gives e101_r_camera_set_blast_size its size. at its end, it
 * sets the camera's mode to E101_R_CAMERA_ORBIT_END. its size stays 0, so
 * it draws its ball, column and pieces at a scale of 0. no blast
 * starts when its task or its record can't be allocated */
void e101_mk2_defeat_blast_create(struct blast_params *params);

#endif
