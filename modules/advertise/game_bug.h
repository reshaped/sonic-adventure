/* the GAME BUG screen, which prints its name. mode select makes it the
 * wheel's next screen, but the wheel never goes on to its next. nothing else
 * opens it */
#ifndef ADVERTISE_GAME_BUG_H
#define ADVERTISE_GAME_BUG_H

#include "advertise/screen.h"
#include "system/task.h"

/* game_bug_create starts the screen's task, with its back and next screens
 * from links, and returns it, or NULL. game_bug_destroy ends the task
 * game_bug_create last made, if it made one.
 *
 * once the textures are ready, A or Start goes on to next 10 steps later. B
 * or X goes back. as the screen closes, either way, it sets the menus' choice
 * to MENU_ADVENTURE */
struct task *game_bug_create(struct screen_links *links);
void game_bug_destroy(void);

#endif
