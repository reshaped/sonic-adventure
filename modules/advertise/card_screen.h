/* the card screen, the menus' first from the top: the game's title, after a
 * notice when no memory card is ready. Start goes on, and the title's music
 * ending starts the demo */
#ifndef ADVERTISE_CARD_SCREEN_H
#define ADVERTISE_CARD_SCREEN_H

#include "advertise/screen.h"
#include "system/task.h"

/* card_screen_create starts the screen's task, with its back and next
 * screens from links, and returns it, or NULL. card_screen_destroy ends the
 * task card_screen_create last made, if it made one.
 *
 * while menu_state.mode_visits is 0 and menu_state.card_noticed isn't 1, the
 * screen opens by looking for a memory card. with one ready, it shows the
 * title. with none, it sets card_noticed and shows a notice before the
 * title, which A or Start closes. if still no card is ready then, the screen
 * sets save_disabled, and sets save_drive and save_slot to 0. it also makes
 * mode select its next, past the drive and file screens, and itself mode
 * select's back. with mode_visits nonzero or card_noticed 1, the screen opens
 * on the title, with mode select as its next and itself as mode select's
 * back. on the title, Start goes on to next once the textures are ready. the
 * end of the title's music closes the menus with MENU_DEMO.
 *
 * card_screen_next_mode needs the task live. it makes mode select the screen
 * Start goes on to */
struct task *card_screen_create(struct screen_links *links);
void card_screen_destroy(void);
void card_screen_next_mode(void);

#endif
