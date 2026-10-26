/* the options screen, which goes on to the sound test and the file screen,
 * and sets the events' messages and the speakers */
#ifndef ADVERTISE_OPTIONS_H
#define ADVERTISE_OPTIONS_H

#include "advertise/screen.h"
#include "system/task.h"

/* options_create starts the screen's task, with its back screen from links, and
 * returns it, or NULL. links->next is kept, but nothing reads it: each item
 * names its own screen. options_destroy ends the task options_create last made,
 * if it made one.
 *
 * the screen opens with the cursor on the sound test when it comes from mode
 * select, and where it was otherwise. once the screen's textures are in, A or
 * Start chooses the item under the cursor, and B or X goes back. the sound test
 * item goes on to its screen. the messages item asks MESSAGES_MENU, with the
 * cursor on the save's setting. either of its items sets the voice-only option
 * to match, and writes the saves to the card as save_write does. the speakers
 * item asks SPEAKERS_MENU, with the cursor on the console's sound mode. either
 * of its items sets the mode to match, as sound_config_set does. B and X in
 * either menu change nothing. the file item goes on to the drive screen, and
 * makes the options screen the drive screen's back and the file screen's next.
 * it also makes mode select the card screen's next, and the card screen mode
 * select's back. the back item goes back */
struct task *options_create(struct screen_links *links);
void options_destroy(void);

#endif
