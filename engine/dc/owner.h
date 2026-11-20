/* owner.c's calls beyond those it defines under Kamui's names, for its 60 fps
 * frame. sa_present_mode sets the rate. a split frame shows two pictures for
 * one frame of play, and present.c flips the first by owner_flip_on */
#ifndef ENGINE_DC_OWNER_H
#define ENGINE_DC_OWNER_H

#include <kamui.h>

/* where on is TRUE, show each picture when rendered, at 60 fps. each vblank
 * that owner.c's count_down lets through shows the last render. it also
 * starts the oldest picture the tile accelerator finished, whether a flip
 * waits or not. a flip waits for the tile accelerator to take its lists. it
 * waits for the slot before it, whose bank the next slot takes, to be past
 * its render. kmFlipFrameBuffer's flip also waits for such a vblank. where on
 * is FALSE the frame turns Kamui's at the first such vblank where no finished
 * picture waits. Kamui's frame starts a render only as a flip ends.
 * a flip ahead of that vblank waits for it, so it goes on in Kamui's frame.
 * sa_present_mode calls owner_sixty once a frame */
void owner_sixty(int on);
/* kmFlipFrameBuffer for a split frame's first picture. at 60 fps its flip
 * waits for no vblank. it goes on once the tile accelerator has the lists
 * and the next slot's bank is free */
KMSTATUS owner_flip_on(void);

#endif
