/* sprites drawn through the current matrix. assembly writes their vertices
 * straight into Kamui's vertex buffer */
#ifndef DRAW_SPRITE_H
#define DRAW_SPRITE_H
#include <ninja.h>
#include <sg_xpt.h>

/* set the depth sprite_draw_3d adds to the z of each sprite it draws with
 * NJD_SPRITE_SCALE. the near and far clip tests take the sum, and so does
 * the 1/z of the sprite's vertices. where the sprite lands on the screen,
 * and its size, still take its own z. the depth is 0 before any call */
void sprite_set_depth(Float depth);
/* the depth sprite_set_depth last set */
Float sprite_get_depth(void);
/* draw sprite->tanim[entry] at sprite->p, through the current matrix, on a
 * frame's first step only, while frame_step is 0. with NJD_SPRITE_SCALE in
 * attr the sprite faces the screen, and otherwise lies in the plane
 * z = sprite->p.z. NJD_SPRITE_ALPHA draws it translucent, and
 * NJD_SPRITE_COLOR colors it with the constant material in place of white.
 * NJD_SPRITE_ANGLE turns it by sprite->ang, and NJD_SPRITE_HFLIP and
 * NJD_SPRITE_VFLIP flip its texture. a facing sprite's height scales by
 * sprite->sx, not sprite->sy */
void sprite_draw_3d(NJS_SPRITE *sprite, Int entry, Uint32 attr);

#endif
