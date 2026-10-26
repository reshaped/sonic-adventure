/* the a-life's task, which each garden's stage task starts, and its
 * globals */
#ifndef AL_MAIN_ALIFE_H
#define AL_MAIN_ALIFE_H
#include <sg_xpt.h>

struct alife_entry;

/* SHC lays out bss in the order of each name's first declaration. so this
 * header declares every bss global alife.c defines, those only alife.c
 * uses too, in the order the image holds them. alife.c's three statics
 * follow them */

/* the fruit in the garden. each fruit's task adds 1 as it starts and takes
 * 1 as it ends. a tree grows no fruit while the count is 10 or more */
extern Sint16 alife_fruit_count;
/* the character player 0 plays, 0 to 5 in CHAO_PLAYERS's order, which
 * indexes a Chao's likes. Tikal and Eggman count as Sonic, 0 */
extern Sint32 alife_character;
/* the garden, an enum garden */
extern Sint32 alife_garden;
/* 8 bytes no code reads or writes */
extern Uint8 dat_0cb1e8cc[8];
/* each category's count of entries, and its list's first entry, the newest.
 * the rest of the a-life reads them through entry.h's alife_get_ functions */
extern Sint32 alife_chao_count;
extern Sint32 alife_animal_count;
extern Sint32 alife_place_count;
extern Sint32 alife_call_count;
extern struct alife_entry *alife_chao_list;
extern struct alife_entry *alife_animal_list;
extern struct alife_entry *alife_place_list;
extern struct alife_entry *alife_call_list;
/* the favorite marks the garden's Chao have left since the a-life's task
 * started. alife_mark_favorite leaves no mark past the 101st */
extern Uint16 alife_mark_count;

/* empty the four entry lists, then make the a-life's task. its first step
 * zeroes alife_character, alife_fruit_count and alife_mark_count, and sets
 * alife_garden. its third places player 0 at its entrance, clears the
 * carried Chao, sets alife_character and takes in the egg player 0 holds.
 * each step from the fourth finds the entries that touch */
void alife_create(void);

#endif
