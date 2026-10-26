/* O WSR and O WSR2 water sprays, a puff of water smoke every few frames */
#ifndef STG09_WATER_SPRAY_H
#define STG09_WATER_SPRAY_H

#include "system/task.h"

/* the object list's steps. a spray puffs every n + 2 frames, n being ang.x's
 * low byte. a puff starts at the work's position, at scale scl.z + 0.1. its
 * speed is scl.x up, turned about z by ang.z, then about y by ang.y. each axis
 * of the speed then moves up to scl.y either way at random.
 *
 * O WSR2 sprays in spells between rests, starting with a rest. each spell and
 * each rest lasts 1 to 120 frames at random. the count of frames between puffs
 * runs only during a spell */
void water_spray_step(struct task *task);
void water_spray2_step(struct task *task);

#endif
