/* saves: a game's progress and records, as the memory card keeps them */
#ifndef SAVE_SAVE_H
#define SAVE_SAVE_H
#include <sg_xpt.h>

#include "player/player.h"
#include "progress/progress.h"

/* a save keeps a record of each action stage a character plays: Sonic's
 * ten, then Tails' five, Knuckles' five, Amy's three, E-102's five and
 * Big's four. each character's records come in stage order */
#define SAVE_STAGES 32
/* Big's four records come last, and keep no time */
#define SAVE_STAGE_TIMES 28
/* Big's four keep his catch instead: Emerald Coast, Twinkle Park, Ice Cap
 * and Hot Shelter */
#define SAVE_FISHING_STAGES (SAVE_STAGES - SAVE_STAGE_TIMES)

/* a save keeps the best three scores or times of each minigame, boss trial
 * and fishing stage */
#define SAVE_BEST 3
/* a save's cart times keep the best run's splits as its first two laps
 * ended */
#define SAVE_CART_SPLITS 2

/* the characters with a boss trial: Sonic, Tails, Knuckles, Amy, E-102
 * and Big */
#define SAVE_TRIALS 6

/* the characters a save keeps records for: all but Eggman and Tikal */
#define SAVE_CHARACTERS 6

/* the bytes of a save's emblem bits */
#define EMBLEM_BYTES 17

/* the saves the card's file holds */
#define SAVE_COUNT 3
/* a new save's stage, past every real one */
#define NO_STAGE 100

/* a bit of a save's flags: events play their voices with no messages */
#define SAVE_NO_MESSAGES 0x2

/* a time as a save keeps it. frames counts sixtieths of a second, but
 * hundredths in a cart time */
struct save_time {
    Sint8 minutes;
    Sint8 seconds;
    Sint8 frames;
};

/* a minigame's best scores, the best first */
struct save_scores {
    Sint32 score[SAVE_BEST];
};

/* a boss trial's best times, the best first */
struct save_times {
    struct save_time time[SAVE_BEST];
};

/* a character's times in an act of the cart: the best first, then the best
 * run's splits */
struct save_cart_times {
    struct save_time time[SAVE_BEST + SAVE_CART_SPLITS];
};

/* the biggest fish Big has caught in a stage: its weight, and its kind */
struct save_fish {
    Sint16 weight;
    Sint16 kind;
};

struct save {
    Uint32 checksum; /* a CRC-16 of the bytes after it, in the low half */
    Sint32 play_time; /* the steps played */
    Sint32 stage_scores[SAVE_STAGES]; /* the best score in each stage */
    /* the best time in each stage: the least, or the most left for E-102's */
    struct save_time stage_times[SAVE_STAGE_TIMES];
    /* Big's best fishing scores in each of his stages, in tens, and his
     * biggest fish there */
    Sint16 fishing_scores[SAVE_FISHING_STAGES][SAVE_BEST];
    struct save_fish biggest_fish[SAVE_FISHING_STAGES];
    Sint16 stage_rings[SAVE_STAGES]; /* the most rings in each stage */
    /* the best scores of Sky Chase's two acts, Ice Cap's board and Sand
     * Hill's board: an array each, of Sonic's and then Tails' */
    struct save_scores shooting1_scores[2];
    struct save_scores shooting2_scores[2];
    struct save_scores icecap_scores[2];
    struct save_scores sunaboard_scores[2];
    struct save_scores mole_scores; /* the Hedgehog Hammer's scores */
    /* each character's times in the cart's first act */
    struct save_cart_times cart_times[SAVE_CHARACTERS];
    struct save_times trial_times[SAVE_TRIALS];
    Sint8 emblems[EMBLEM_BYTES]; /* a bit for each emblem won */
    Sint8 flags; /* SAVE_NO_MESSAGES */
    Sint8 lives[SAVE_CHARACTERS];
    Uint8 character; /* the character playing as the save was last stored */
    Uint8 unk_259[3];
    Uint16 stage; /* stage_last_cleared as the save was last stored */
    Uint8 unk_25e[2];
    /* event_flags and events_played, a bit each, then the rest of the
     * progress in its own types */
    Sint8 event_flags[FLAG_COUNT / 8];
    Sint8 events_played[EVENT_COUNT / 8];
    Uint8 unk_2e0[CHARACTER_COUNT];
    struct story_record story_records[CHARACTER_COUNT];
    Sint8 stage_clears[CHARACTER_COUNT][STAGE_COUNT];
};

/* the stage of the last clear counted in stage_clears. save_store keeps it
 * in the save, and save_load restores it */
extern int stage_last_cleared;
/* the saves as the card's file holds them */
extern struct save saves[SAVE_COUNT];

/* set every save, or the one the game plays, to a new game with its
 * checksum, and set the rings to 0. a new game holds each record's
 * default, lets Sonic alone be played, and starts his story in Station
 * Square */
void save_reset_all(void);
void save_reset(void);
/* copy the progress into saves[save_slot] and give it a new checksum. it
 * writes nothing to the card. save_slot must be below SAVE_COUNT, and the
 * character playing below CHARACTER_COUNT */
void save_store(void);
/* as save_store, but storing only the play time, the character and
 * stage_last_cleared */
void save_update(void);
/* copy saves[save_slot] into the progress. it reads nothing from the card
 * and checks no checksum. the save's character must be below
 * CHARACTER_COUNT */
void save_load(void);
/* TRUE if each of the SAVE_COUNT saves from save on matches its checksum,
 * else FALSE */
Bool save_check_all(struct save *save);

#endif
