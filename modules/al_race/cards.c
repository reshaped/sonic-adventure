/* the Chao the race loads from the memory cards, and writes back to them */
#include <sg_xpt.h>
#include <shinobi.h>

#include "al_main/card.h"
#include "al_race/cards.h"
#include "chao/chao.h"
#include "save/card.h"

static const Sint32 race_drives[CARD_DRIVES] = {0, 1, 2, 3, 4, 5, 6, 7};

int race_load_cards(struct race_cards *cards, struct chao *chao)
{
    /* @hack spelling: to, a cursor apart from chao, which the loop steps in
     * chao's place. it keeps cards in R13, i in R14, 8 in R9 and 1 in R8
     * (0x0cb93166 to 0x0cb9317a), as the image does. stepping chao itself,
     * each pair swaps and 20 of 76 words differ */
    struct chao *to = chao;
    int i;
    const Sint32 *drive = race_drives;

    cards->count = cards->remaining = cards->loaded = 0;
    for (i = 0; i < CARD_DRIVES; i++) {
        if (card_menu_drives[i].exec_marked == 1 &&
            card_menu_drives[i].chao.type != CHAO_NOTHING) {
            if (card_read_chao(*drive, to) == 1) {
                while (buStat(*drive) != BUD_STAT_READY) {
                }
                cards->loaded |= 1 << i;
                ++cards->count;
            }
        }
        ++drive;
        ++to;
    }
    cards->remaining = cards->loaded;
    return 1;
}

int race_cards_busy(struct race_cards *cards)
{
    int i;
    const Sint32 *drive = race_drives;

    for (i = 0; i < CARD_DRIVES; i++, drive++) {
        if (cards->loaded & (1 << i)) {
            if (buStat(*drive) == (Sint32)BUD_STAT_BUSY) {
                return 1;
            }
        }
    }
    return 0;
}

int race_card_has_chao(struct race_cards *cards, int drive)
{
    if (card_menu_drives[drive].chao.type != CHAO_NOTHING) {
        if (cards->loaded & (1 << drive)) {
            return 1;
        }
    }
    return 0;
}

int race_card_loaded(struct race_cards *cards, int drive)
{
    if (cards->loaded & (1 << drive)) {
        return 1;
    }
    return 0;
}

int race_cards_drop_absent(struct race_cards *cards)
{
    int i = 0;
    int bit;

    /* @hack spelling: the loop as a do-while, which SHC keeps rolled, as the
     * image does. as a for loop, SHC writes the body four times a pass, the
     * function runs 128 bytes long, and 18 of 31 words differ */
    do {
        bit = 1 << i;
        if (cards->remaining & bit) {
            if (card_menu_drives[i].present != 1) {
                cards->remaining &= ~bit;
            }
        }
    } while (++i < CARD_DRIVES);
    return 1;
}

int race_card_remains(struct race_cards *cards, int drive)
{
    if (cards->remaining & (1 << drive)) {
        return 1;
    }
    return 0;
}

int race_cards_changed(struct race_cards *cards)
{
    int i;

    for (i = 0; i < CARD_DRIVES; i++) {
        if (card_menu_drives[i].present != card_menu_drives[i].was_present) {
            return 1;
        }
    }
    return 0;
}

void race_card_save(struct race_cards *cards, int drive,
                    const struct chao *chao)
{
    card_write_chao(drive, chao);
}
