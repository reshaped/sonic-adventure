/* the arches O ARCH 1 and O ARCH 2, the lights O LIGHT 1 and O LIGHT 2, and
 * the pole O POLE, whose beam swings */
#ifndef MINICART_ARCH_H
#define MINICART_ARCH_H

#include "system/task.h"

/* the object list's steps for O ARCH 1, O ARCH 2, O LIGHT 1, O LIGHT 2 and O
 * POLE. each task needs a work. outside Twinkle Park's act 0 and the cart
 * course, each step only checks the task's range */

/* O ARCH 1: the set entry's scale x, rounded down, is how far the texture
 * scrolls each step */
void arch_1_step(struct task *task);
/* O ARCH 2: scale x is as O ARCH 1's. scale y picks the sub arches: two
 * above 10, one above 0, and none otherwise. scale z is how far their
 * spotlights rock either way, in degrees. the set editor draws each spotlight
 * at a limit of its rock */
void arch_2_step(struct task *task);
void light_1_step(struct task *task);
/* O LIGHT 2: scale y is how far the light turns about y each step, in
 * degrees. it turns in the set editor too, while the editor holds it */
void light_2_step(struct task *task);
/* O POLE: the set entry's scale x, rounded down, is how far the texture
 * scrolls each frame. it is also how far the beam swings each step, in angle
 * units. scale y is how far the beam swings either way, in degrees. while the
 * set editor holds the pole, the beam's swing is scale y, rounded down, in
 * angle units */
void pole_step(struct task *task);

#endif
