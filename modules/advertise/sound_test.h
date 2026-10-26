/* the sound test, which the options screen goes on to. it plays the game's
 * music by kind, and the sounds of the effects' banks */
#ifndef ADVERTISE_SOUND_TEST_H
#define ADVERTISE_SOUND_TEST_H

#include "advertise/screen.h"
#include "system/task.h"

/* sound_test_create starts the screen's task, with its back screen from
 * links, and returns it, or NULL. links->next is kept, but nothing reads it.
 * sound_test_destroy ends the task sound_test_create last made, if it made
 * one.
 *
 * as the screen opens, it stops the music and voices and plays its own voice
 * line. once it has faded in, it asks SOUND_TEST_MENU for a kind, with the
 * cursor on the first. a kind of music lists its tracks, five rows at a time.
 * up and down move the cursor. A or Start plays the track under the cursor, and
 * B or X stops the music and asks the menu again. neither does anything until
 * the screen's textures are in. the effects load a bank's file and show three
 * numbers: the bank, a group in it and a sound in the group. left and right
 * pick a number, and up and down step it. a step zeroes the numbers to its
 * right, and a new bank loads its file. A or Start plays the sound, and B or X
 * loads the menus' bank, mobile1.mlt, and asks the menu again. back, or B or X
 * in the menu, goes back. the numbers and the one picked stay as they were from
 * one visit to the next */
struct task *sound_test_create(struct screen_links *links);
void sound_test_destroy(void);

#endif
