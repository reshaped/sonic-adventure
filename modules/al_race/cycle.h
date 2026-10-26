/* the course's texture cycles, each a material that shows textures in turn */
#ifndef AL_RACE_CYCLE_H
#define AL_RACE_CYCLE_H

/* add the course's three texture cycles to the scenery. each sets its
 * material's texture to the next of textures 55 to 68, and starts over
 * after 68. the first two change every 4 steps, and the third every 2 */
void cycles_create(void);

#endif
