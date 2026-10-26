/* the fish of Big's stages, and the generators that keep them swimming */
#ifndef STG01_FISH_H
#define STG01_FISH_H

#include "system/task.h"

/* the steps the object lists of Big's stages name for S GENE and S BASS.
 * each runs a generator, which keeps one fish swimming where it stands. each
 * stage has a list of kinds. a scale x from 1 up to the list's count picks
 * that entry. any other picks a random kind. a nonzero scale z makes the fish
 * larger. in Hot Shelter, a nonzero scale y makes the generator wait for
 * fishing_shelter_ready. fish_bass_step runs fish_generator_step */
void fish_generator_step(struct task *task);
void fish_bass_step(struct task *task);

#endif
