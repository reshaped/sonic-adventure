/* the vmu screens: what the memory unit in each pad shows. an update is one
 * pass of vmu_frame_update over the screens. a screen's own update is one
 * update in eight, picked by its port: the one on which it draws its
 * animation and writes its lcd */
#ifndef VMU_SCREEN_H
#define VMU_SCREEN_H
#include <sg_xpt.h>

#include "main.h"

#define VMU_WIDTH 48
#define VMU_HEIGHT 32
/* the side of the Chao's square pictures, which draw in a screen's middle */
#define VMU_CHAO_SIDE 24
#define PUZZLE_SIDE 3
#define PUZZLE_CELLS (PUZZLE_SIDE * PUZZLE_SIDE)

enum vmu_request {
    VMU_REQUEST_CLEAR = 1, /* show the Chao's clear animation for 480 updates */
    VMU_REQUEST_GAME_OVER = 3
};

/* an animation is an array of these, ended by a NULL bitmap */
struct vmu_frame {
    /* a bit to a pixel, msb first, by rows from the top left: 192 bytes for
     * the whole screen, and 72 for the Chao */
    const Uint8 *bitmap;
    Uint32 updates; /* how long it shows, in the screen's own updates */
};

/* a place on a screen, or a size, in pixels */
struct vmu_point {
    Sint16 x;
    Sint16 y;
};

/* the screen of the memory unit in one port's pad. it shows the Chao that
 * follows the player, the logo, the Chao in a garden, or a sliding puzzle */
struct vmu_screen {
    Uint8 pixels[VMU_HEIGHT][VMU_WIDTH]; /* a byte to a pixel, lit if not 0 */
    Sint8 ready; /* pdVmsLcdIsReady's result: 1 when the port holds an lcd */
    Sint8 direction; /* pdLcdGetDirection's result, a PDD_LCD_DIRECTION_ */
    Sint8 port; /* 0 to 3 for ports a to d, whose pad the screen reads */
    Sint8 mode; /* screen.c's enum screen_mode */
    /* updates left in the mode. CHAO_GROUND counts up the updates the player
     * has stood still instead, and PUZZLE_FADE the fade's updates so far */
    Sint16 wait;
    /* updates left before the puzzle stops waiting for a button */
    Sint32 countdown;
    Uint16 frame; /* anim's frame showing */
    Uint16 timer; /* own updates the frame has shown */
    const struct vmu_frame *anim;
    Sint8 puzzles; /* puzzles solved and the one in play, up to 50 */
    Sint8 slides; /* slides in this puzzle, up to 99 */
    Sint8 blank; /* the tile drawn as the gap */
    Sint8 cursor; /* the cursor's cell, 0 to 8 by rows */
    /* while a puzzle is played, a count that rises each update. its bits
     * blink the cursor's box and the last tally mark. while the puzzle fades
     * out, 1 when it was solved, and 0 when B gave it up */
    Sint8 blink;
    /* each cell's tile, solved when each holds its own */
    Sint8 tiles[PUZZLE_CELLS];
    /* the next shuffle slides along the blank's row, not its column */
    Sint8 along_row;
    Sint16 look_wait; /* updates to the garden Chao's next look */
    Sint16 roam_wait; /* updates to the garden Chao's next target */
    struct vmu_point target; /* where the garden's Chao heads */
    struct vmu_point pos; /* where the garden's Chao is */
    Uint32 lcd; /* the memory unit's port */
    /* @hack type: held and pressed signed, which has vmu_init clear them
     * with the 0 in R13 that its other stores take (MOV.L R13,@(R0,R14) at
     * 0x8c118dba and 0x8c118dc0); as Uint32, SHC loads a 0 of their own,
     * vmu_init runs 2 bytes longer and 31 of its 52 words differ */
    Sint32 held; /* the buttons the pad holds */
    Sint32 pressed; /* the buttons that went down since the last update */
};

/* ask the screens for request on the next update alone. only a screen
 * showing the Chao that follows the player takes it */
void vmu_request(int request);
/* TRUE on the screen's own update */
Bool vmu_screen_due(struct vmu_screen *screen);
/* clear every screen and restart its mode, then probe each memory unit */
void vmu_init(void);
/* run one update of every screen, given main's game mode. while
 * frame_interval is 1, every other call returns at once */
void vmu_frame_update(enum game_mode game_mode);

#endif
