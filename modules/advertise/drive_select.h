/* the drive screen, which picks the memory card the game saves to */
#ifndef ADVERTISE_DRIVE_SELECT_H
#define ADVERTISE_DRIVE_SELECT_H

#include "advertise/screen.h"
#include "system/task.h"

/* drive_select_create starts the screen's task, with its back and next
 * screens from links, and returns it, or NULL. drive_select_destroy ends the
 * task drive_select_create last made, if it made one.
 *
 * the screen clears save_disabled as it opens. a choice sets save_drive to
 * the drive chosen 8 steps later. the screen then goes on to next with the
 * card's save read, or with a new one made and written. for a card with no
 * save and fewer than 10 blocks free, it asks first whether to go on without
 * the card. if no card is ready by the end of the 8 steps, it goes on to next
 * with a new save made and written nowhere. B or X goes back. going on without
 * the card goes back too, with save_disabled set, save_drive and save_slot 0,
 * and menu_state.mode_visits raised.
 *
 * drive_select_back_options needs the task live. it makes the options screen
 * the back screen, and B and X then don't go back */
struct task *drive_select_create(struct screen_links *links);
void drive_select_destroy(void);
void drive_select_back_options(void);

#endif
