/* Ninja's draw state: its 3D control flags and constant attributes, saved
 * around a draw and set for a translucent one */
#ifndef DRAW_STATE_H
#define DRAW_STATE_H
#include <ninja.h>
#include <sg_xpt.h>

/* draw_state_save saves Ninja's 3D control flags and constant attributes,
 * and draw_state_restore puts them back. the save uses the one slot each of
 * draw_control_save and draw_attr_save, so it doesn't nest, with itself or
 * with them. the restore also sets the source blend to
 * NJD_COLOR_BLENDING_BOTHSRCALPHA and the constant material to 0 in every
 * channel, neither of which the save keeps */
void draw_state_save(void);
void draw_state_restore(void);
/* turn on NJD_CONTROL_3D_CONSTANT_ATTR, and in the constant attributes clear
 * clear_attr's bits, then set set_attr's */
void draw_constant_attr(Uint32 clear_attr, Uint32 set_attr);
/* set Ninja up to draw translucent at alpha. it sets the source blend to
 * NJD_COLOR_BLENDING_BOTHSRCALPHA, and turns the offset material off. it
 * turns the constant material on, white at alpha, and the constant
 * attributes on, with NJD_FLAG_USE_ALPHA added. each stays until the caller
 * puts it back, as draw_state_restore does after draw_state_save */
void draw_translucent(Float alpha);

#endif
