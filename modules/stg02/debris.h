/* the tornado's debris, which flies into it, and the spiral of bridge pieces
 * that circles it. the haze about the camera in the tornado's storm.
 * O_SETSMO, which puffs dust and sends debris to the tornado */
#ifndef STG02_DEBRIS_H
#define STG02_DEBRIS_H

#include "system/task.h"

/* task step for a piece of debris, which tumbles from work->pos into the
 * tornado. it flies at the height it starts at. it kills itself within 10 of
 * the tornado across the ground, or on its 151st step of flight. an scl.z of
 * exactly 1 draws sai_object in place of the debris models. until the
 * tornado's first run step, debris flies at an unset x and z */
void windy_tornado_debris_step(struct task *task);
/* task step for the haze about the camera in the tornado's storm. it ends
 * once the stage or act differs from the one it started in. it also ends
 * once player 0 is farther than 1500 from the tornado's eye. player 0 must
 * exist */
void windy_haze_step(struct task *task);
/* task step for the tornado's child, a spiral of ten bridge pieces about
 * the tornado. the spiral turns and sinks */
void windy_tornado_spiral_step(struct task *task);
/* the object list's step for O_SETSMO. every 16 steps, it puffs dust at its
 * position and sends debris from there into the tornado. scl.x adds to the
 * dust's speed along each axis. the dust's scale is 10 plus scl.y */
void windy_dust_source_step(struct task *task);

#endif
