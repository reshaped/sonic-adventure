/* the glows on the Egg Walker's legs, and its stomps */
#ifndef B_EGM2_LEGS_H
#define B_EGM2_LEGS_H
#include <ninja.h>

#include "system/task.h"

/* restart the glows' textures from the first, and set up a sprite of them,
 * which nothing draws */
void egg_walker_glows_init(void);
/* draw a glow on each leg in LEG_GLOW. on each in LEG_FADE, draw a glow that
 * shrinks to nothing, with lightning about it. task is the boss's. it runs
 * only on a frame's first step, where frame_step is 0 */
void egg_walker_glows_step(struct task *task);
/* start a stomp at STAND_Y below foot 0 to 3. a foot of LEGS or more puts
 * it below player 0 instead. each of its 31 steps starts a ripple and a
 * wall. its shape widens, and hurts players it meets. nothing starts when
 * task_create fails */
void egg_walker_stomp_create(int foot);

/* the glows' texlist, which egm2_pvms fills from EGM2_EFFECT */
extern NJS_TEXLIST egm2_effect_texlist;

#endif
