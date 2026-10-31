/* EV00F8's own sprites: additive 3d sprites in ten slots. each shows one
 * texture of a texlist, at an offset from a task or at a fixed place */
#ifndef EV00F8_SPRITE_H
#define EV00F8_SPRITE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/sprite.h"
#include "system/task.h"

extern NJS_TEXLIST ev00f8_sprite_texlists[EVENT_SPRITE_TEXLISTS];

Sint32 ev00f8_sprite_load_texture(NJS_TEXLIST *texlist);
/* destroy the sprite in slot at once, if there is one, and empty the slot */
void ev00f8_sprite_kill(Sint16 slot);
/* create a sprite in slot, 0 to 9, showing texture of texlist. it keeps to
 * the offset x, y, z from base, turned by base's angles. with base null, it
 * stays at the offset. it starts white, at a scale of 1 and an angle of 0,
 * neither growing nor spinning. base and texlist must outlive it. a sprite
 * already in the slot runs on, and no call reaches it again. the record's
 * allocation is unchecked, and leaks when the task's fails */
void ev00f8_sprite_create(struct task *base, Sint16 slot, Float x, Float y,
                          Float z, NJS_TEXLIST *texlist, Sint16 texture);
/* these change the sprite in slot, and do nothing to an empty slot.
 * ev00f8_sprite_set_scale sets its scale, and ev00f8_sprite_set_color its
 * color. ev00f8_sprite_turn adds ang to its angle and sets spin, which each
 * draw adds. ev00f8_sprite_set_growth sets what each draw adds to the scale
 * while both sides are between 0 and 100 */
void ev00f8_sprite_set_scale(Sint16 slot, Float x, Float y);
void ev00f8_sprite_set_color(Sint16 slot, Float alpha, Float red, Float green,
                             Float blue);
void ev00f8_sprite_turn(Sint16 slot, Angle ang, Angle spin);
void ev00f8_sprite_set_growth(Sint16 slot, Float x, Float y);

#endif
