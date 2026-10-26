/* the trial's mini game list, which picks one of the mini games the trial
 * can play for menu_state.story. it shows the emblems and best three records
 * of the one under the cursor */
#ifndef ADVERTISE_MINI_GAME_SELECT_H
#define ADVERTISE_MINI_GAME_SELECT_H

#include "advertise/screen.h"
#include "hud/menu.h"
#include "system/task.h"

/* draw MINI_GAME_MENU's contents: the names of the story's character and of
 * the mini game under the cursor, with their labels. it needs the screen's
 * task live */
void mini_game_menu_draw(struct menu_view *view);
/* mini_game_select_create starts the screen's task, with its back and next
 * screens from links, and returns it, or NULL. mini_game_select_destroy ends
 * the task mini_game_select_create last made, if it made one.
 *
 * the screen sets menu_state.trial[0] to MENU_TRIAL_MINI_GAME as it opens.
 * A or Start opens MINI_GAME_MENU, which goes on to next or cancels, and B or
 * X goes back. as the screen closes, either way, it sets trial_choice and
 * menu_state's stage and act to the mini game under the cursor. it also sets
 * the menus' choice to MENU_TRIAL */
struct task *mini_game_select_create(struct screen_links *links);
void mini_game_select_destroy(void);

#endif
