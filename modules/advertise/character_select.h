/* the wheel of characters: the screen that picks the story to play, or in
 * the trial the character to list stages or mini games for */
#ifndef ADVERTISE_CHARACTER_SELECT_H
#define ADVERTISE_CHARACTER_SELECT_H

#include "advertise/screen.h"
#include "system/task.h"

/* character_select_create starts the screen's task, with its back and next
 * screens from links, and returns it, or NULL. character_select_destroy ends
 * the task character_select_create last made, if it made one.
 * character_select_set_next needs the task live. it sets the next screen,
 * which the wheel never reads: it goes on by its own choice */
struct task *character_select_create(struct screen_links *links);
void character_select_destroy(void);
void character_select_set_next(int next);

#endif
