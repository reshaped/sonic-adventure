/* the Chao the race loads from the memory cards, and writes back to them */
#ifndef AL_RACE_CARDS_H
#define AL_RACE_CARDS_H
#include <sg_xpt.h>

#include "chao/chao.h"

/* the drives the race loaded Chao from. loaded and remaining are masks, with
 * bit 1 << drive for each drive */
struct race_cards {
    Uint8 unk_00[0x40];
    Uint8 loaded; /* the drives race_load_cards loaded */
    /* loaded, less each drive race_cards_drop_absent has found without a
     * card since the load */
    Uint8 remaining;
    Uint8 count; /* the drives loaded */
};

/* the functions below take a drive from 0 to CARD_DRIVES - 1, whose record
 * is card_menu_drives[drive] */

/* load the Chao of each drive whose record has exec_marked 1 and a Chao.
 * card_read_chao queues the read of the card's Chao into chao[drive]. where it
 * returns 1, race_load_cards waits until buStat(drive) is ready and counts
 * the drive loaded. chao must hold a record for each drive. a drive not
 * loaded keeps its record as it was: card_read_chao leaves the record alone
 * where it returns 0. loaded, remaining and count start over, and remaining
 * ends equal to loaded. returns 1 */
int race_load_cards(struct race_cards *cards, struct chao *chao);
/* 1 when buStat is busy for any loaded drive, else 0 */
int race_cards_busy(struct race_cards *cards);
/* 1 when drive is loaded and its record has a Chao, else 0 */
int race_card_has_chao(struct race_cards *cards, int drive);
/* 1 when drive is loaded, else 0 */
int race_card_loaded(struct race_cards *cards, int drive);
/* take out of remaining each drive whose record's present isn't 1, and
 * return 1 */
int race_cards_drop_absent(struct race_cards *cards);
/* 1 when drive is in remaining, else 0 */
int race_card_remains(struct race_cards *cards, int drive);
/* 1 when any drive's present differs from its was_present, else 0. a
 * difference is a card come or gone that card_drives_take_changes, which polls
 * the drives' cards, hasn't taken in yet. cards goes unread */
int race_cards_changed(struct race_cards *cards);
/* queue the write of chao to drive's card with card_write_chao, whose result
 * it drops. chao must stay valid until buStat(drive) is ready. on a card
 * with no executable file but room for one, card_write_chao saves SET_AREA_1
 * as that file. during the race, SET_AREA_1 holds AL_Race01's set file, not
 * samp_lcd.PRS's executable, so such a card gets the set file as its
 * executable. cards goes unread */
void race_card_save(struct race_cards *cards, int drive,
                    const struct chao *chao);

#endif
