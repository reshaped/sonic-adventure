/* the Dreamcast's crash screen: where the game stopped, written straight into
 * the frame buffer the screen shows, with no draw of the renderer's. an
 * address error or an illegal instruction writes it and stops there, on a
 * stack of its own, where Shinobi's handler, syIntBrkRteRte, would put the
 * stopped code's registers back and jump to 0xac000010. what a retail
 * console does there is not established. dreamcast.h's port_stop writes it
 * for a failure the game names. a game that ends no frame and sends no
 * picture to be shown for CRASH_HANG vblanks writes it from the vblank
 * interrupt, and goes on.
 *
 * the vblank's count finds a game that has stopped sending pictures while
 * the interrupt still runs. it finds no stop with interrupts masked, and none
 * inside a handler the vblank's chain runs ahead of its own */
#ifndef ENGINE_DC_CRASH_H
#define ENGINE_DC_CRASH_H

/* a frame ended: the game is running. the first call joins the vblank's
 * chain */
void crash_frame(void);

#endif
