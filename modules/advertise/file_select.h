/* the file screen, which picks the save to play from the card's three and
 * shows the records of the one under the cursor */
#ifndef ADVERTISE_FILE_SELECT_H
#define ADVERTISE_FILE_SELECT_H

#include "advertise/screen.h"
#include "hud/menu.h"
#include "system/task.h"

/* file_select_create starts the screen's task, with its back and next
 * screens from links, and returns it, or NULL. file_select_destroy ends the
 * task file_select_create last made, if it made one.
 *
 * the screen loads each save the cursor moves to, as save_load does. A or
 * Start on a save opens FILE_MENU, which goes on to next, erases the save as
 * save_reset_write does, or cancels. A or Start with Y held mails the save
 * instead, as save_write_player_mail does, when save_isp_found finds the
 * browser's internet settings in the flash memory. B or X goes back, and so
 * does A or Start on the row below the saves. save_slot then returns to the
 * slot it held as the screen opened, but the progress keeps the save last
 * loaded */
struct task *file_select_create(struct screen_links *links);
void file_select_destroy(void);
/* draw FILE_ERASE_MENU's contents: the question, and the number of the file
 * to erase, menu_state.save's */
void file_erase_menu_draw(struct menu_view *view);
/* file_select_next_options needs the task live. it makes the options screen
 * the one FILE_MENU goes on to */
void file_select_next_options(void);

#endif
