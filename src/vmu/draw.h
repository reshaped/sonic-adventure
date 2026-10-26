/* drawing on a vmu screen's pixels, and writing them to its lcd */
#ifndef VMU_DRAW_H
#define VMU_DRAW_H
#include <sg_xpt.h>

#include "vmu/screen.h"

/* clear VMU_WIDTH * VMU_HEIGHT writable bytes, one byte per pixel */
void vmu_clear(Uint8 *pixels);
/* shift the screen's picture by by, losing what falls off and clearing what
 * it uncovers */
void vmu_shift(struct vmu_screen *screen, struct vmu_point *by);
/* coarsen the picture into blocks of size, cut from the top left. a block
 * turns solid when at least half its pixels, rounded down, are lit, and
 * blank otherwise. a size outside 1..VMU_WIDTH by 1..VMU_HEIGHT, or 1 by 1,
 * changes nothing */
void vmu_pixelate(struct vmu_screen *screen, struct vmu_point *size);
/* on the screen's own update, draw its animation's frame over the whole
 * screen */
void vmu_draw_picture(struct vmu_screen *screen);
/* on the screen's own update, draw its animation's frame as the Chao, in the
 * screen's middle */
void vmu_draw_chao(struct vmu_screen *screen);
/* on the screen's own update, move the animation to its next frame once the
 * frame has shown for its updates, and back to the first after the last */
void vmu_step_anim(struct vmu_screen *screen);
/* show anim from its first frame. the screen keeps the pointer, so anim and
 * its bitmaps must stay in place while it shows. anim needs a frame before
 * its NULL bitmap. a frame of more than 65535 updates never ends, as the
 * screen's timer is 16 bits */
void vmu_set_anim(struct vmu_screen *screen, const struct vmu_frame *anim);
/* draw the sliding puzzle, and clear the rest of the screen. the tiles are
 * cut from the Chao's square in the screen's middle, whatever it holds, with
 * its edge lit. each cell shows its tile, and the blank's cell a
 * checkerboard. a box round the cursor's cell switches between two sizes
 * with screen->blink. it draws on every call, not only on the screen's own
 * update */
void vmu_draw_puzzle(struct vmu_screen *screen);
/* draw screen->puzzles, up to 30, as marks up the screen's left, ten to a
 * column and each fifth twice as wide. the last mark blinks with
 * screen->blink */
void vmu_draw_tally(struct vmu_screen *screen);
/* on the screen's own update, with its lcd ready, write the pixels there,
 * turned half round when the unit is upside down */
void vmu_write(struct vmu_screen *screen);

#endif
