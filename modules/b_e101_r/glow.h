/* E-101 mkII's glow: an object of its model file that fades in and out over
 * the boss */
#ifndef B_E101_R_GLOW_H
#define B_E101_R_GLOW_H
#include <sg_xpt.h>

/* set the glow up hidden, at its object in E101R.NB, which must be loaded.
 * e101_mk2_glow_clear forgets the object, so the glow draws nothing */
void e101_mk2_glow_init(void);
void e101_mk2_glow_clear(void);
/* set which way the glow fades: in over 50 steps, or out over 20 */
void e101_mk2_glow_show(Bool on);
/* step the glow's fade, and turn its texture, a new one every 2 steps. it
 * writes the texture into E101R.NB whether or not init has run, so the file
 * must be loaded */
void e101_mk2_glow_step(void);
/* draw the glow at the current matrix while it shows. it draws on a frame's
 * first step only, between init and clear */
void e101_mk2_glow_draw(void);

#endif
