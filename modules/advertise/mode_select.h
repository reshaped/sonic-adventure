/* mode select, the first screen with a menu, over the game's title. its items
 * are adventure, trial, options and internet */
#ifndef ADVERTISE_MODE_SELECT_H
#define ADVERTISE_MODE_SELECT_H

#include "advertise/screen.h"
#include "hud/menu.h"
#include "system/task.h"

/* draw the window behind the items of MODE_MENU and of TRIAL_MENU */
void mode_menu_draw(struct menu_view *view);
void trial_menu_draw(struct menu_view *view);
/* mode_select_create starts the screen's task, with its back screen from
 * links, and returns it, or NULL. links->next is kept, but nothing reads it:
 * each item names its own screen. mode_select_destroy ends the task
 * mode_select_create last made, if it made one.
 *
 * as the screen opens, it raises menu_state.mode_visits, makes itself the
 * card screen's next and makes the card screen its back. MODE_MENU disables
 * trial while the trial has nothing to play, and internet while
 * internet_open is FALSE. a choice sets menu_state.mode. adventure goes on to
 * the wheel. trial asks TRIAL_MENU, then goes on to the wheel to list action
 * stages or mini games. options goes on to the options screen, and internet
 * closes the menus with MENU_INTERNET. B or X goes back, and sets
 * menu_state.mode to MENU_MODE_ADVENTURE. once the title's music has ended,
 * 480 steps with no move of the cursor close the menu. 20 steps after that,
 * the screen closes the menus with MENU_DEMO.
 *
 * mode_select_back_card needs the task live. it makes the card screen the
 * back screen */
struct task *mode_select_create(struct screen_links *links);
void mode_select_destroy(void);
void mode_select_back_card(void);

#endif
