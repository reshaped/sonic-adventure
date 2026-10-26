/* the trial's action stage list, which picks one of the action stages the
 * trial can play for menu_state.story. it shows the emblems and records of the
 * one under the cursor */
#ifndef ADVERTISE_TRIAL_STAGE_SELECT_H
#define ADVERTISE_TRIAL_STAGE_SELECT_H

#include "advertise/screen.h"
#include "hud/menu.h"
#include "system/task.h"

/* trial_stage_select_create starts the screen's task, with its back and next
 * screens from links, and returns it, or NULL. trial_stage_select_destroy ends
 * the task trial_stage_select_create last made, if it made one.
 *
 * the screen sets menu_state.trial[0] to MENU_TRIAL_STAGE as it opens. A or
 * Start opens TRIAL_STAGE_MENU, which goes on to next or cancels, and B or X
 * goes back while textures_ready is TRUE. as the screen closes, either way, it
 * sets trial_choice and menu_state's stage and act to the stage under the
 * cursor. it also sets the menus' choice to MENU_TRIAL */
struct task *trial_stage_select_create(struct screen_links *links);
void trial_stage_select_destroy(void);
/* draw TRIAL_STAGE_MENU's contents: the names of the story's character and of
 * the stage under the cursor, with their labels. it needs the screen's task
 * live */
void trial_stage_menu_draw(struct menu_view *view);

#endif
