/* the menus' memory card access. the menus run the backup library in a work
 * area of their own. they look on the cards for the game's save and the
 * voice files */
#ifndef ADVERTISE_CARD_H
#define ADVERTISE_CARD_H

#include <sg_xpt.h>

/* what card_find_save finds in a drive: no card ready, a card without the
 * game's save, or a card with it */
enum card_find { CARD_NOT_READY, CARD_NO_SAVE, CARD_SAVE };

/* card_open waits for the game's card access to finish with card_finish.
 * it then starts the backup library on every drive, with its work area past
 * the module's end, and returns once the library is ready. card_complete and
 * card_progress are then the library's callbacks. card_close starts the
 * game's card access again with card_init */
void card_open(void);
void card_close(void);
/* look in drive for the game's save, SONICADV_SYS. a drive without a card
 * ready, or any other result from buIsExistFile, gives CARD_NOT_READY */
enum card_find card_find_save(Uint8 drive);
/* set menu_state.summary[0] to the number, 1 to 8, of a voice file the
 * cards hold, or to 0 when they hold none. the clock's second picks the
 * file: counting the files found from SONICADV_V02 on, and past SONICADV_V08
 * from SONICADV_V01 again, it takes the (second + 1)th */
void card_pick_summary(void);

#endif
