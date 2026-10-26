/* downloads: a card's event file. it adds items and a timed mission to the
 * acts it names, and textures, a sound bank and a model for the items */
#ifndef OBJECT_DOWNLOAD_H
#define OBJECT_DOWNLOAD_H

/* the hex digits '0' to 'F' by value, with no null after them. a card file
 * spells its number in its name with them */
extern char card_hex_digits[16];

/* forget the mission: its progress, its limit, and the items it needs */
void download_reset(void);
/* at an act's start, make a new task for the downloads. then read the
 * card's newest event file again, and add what it holds to the act. the
 * mission's progress carries over from the last act while the file's
 * mission stays the same, and is forgotten otherwise. with the task pool or
 * the heap out of room, it reads through a null task once the file puts an
 * item in the act */
void download_start(void);
/* at an act's end, destroy the downloads' task and the items under it */
void download_end(void);

#endif
