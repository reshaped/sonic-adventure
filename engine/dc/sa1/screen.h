/* Ninja's screen and clips, which SA1's draws on the Dreamcast project and
 * clip by. a layer declares each name it lends as Ninja's own symbol, as
 * live.h says. a draw then reads each word at its address, not through a
 * pointer */
#ifndef SA1_SCREEN_H
#define SA1_SCREEN_H

#include <ninja.h>

/* sa1_live_clip is Ninja's 2D clip: its top left corner, then its bottom
 * right */
extern NJS_SCREEN sa1_live_ninja_screen;
extern Float sa1_live_far_clip;
extern NJS_POINT2 sa1_live_clip[2];
extern Float sa1_live_aspect_y;
extern Float sa1_live_near_clip;

#endif
