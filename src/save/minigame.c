/* minigames: the boards' best scores and the cart's best times */
#include <sg_xpt.h>

#include "minicart/times.h"
#include "player/player.h"
#include "save/card.h"
#include "save/minigame.h"
#include "save/save.h"
#include "stage/stage.h"

/* the save keeps the boards' scores for Sonic and for Tails */
#define BOARD_PLAYERS 2

/* the cart's acts. the save keeps the first's times, and cart_act_times
 * the others' */
#define CART_ACTS 6

/* a character's default times in an act of the cart: the best three, then
 * the best run's splits. CART_ACT_DEFAULTS holds a set for each character */
#define CART_DEFAULTS                                                          \
    {                                                                          \
        {                                                                      \
            {10, 0, 0}, {15, 0, 0}, {20, 0, 0}, {4, 0, 0}, {7, 0, 0}           \
        }                                                                      \
    }
#define CART_ACT_DEFAULTS                                                      \
    {CART_DEFAULTS, CART_DEFAULTS, CART_DEFAULTS,                              \
     CART_DEFAULTS, CART_DEFAULTS, CART_DEFAULTS}

static struct save_cart_times *cart_find_times(void);
static int minigame_character(void);
static int cart_get_act(void);

/* each character's times in the cart's acts after the first.
 * cart_reset_times resets only the save's, so these last until the game
 * restarts */
static struct save_cart_times cart_act_times[CART_ACTS - 1][SAVE_CHARACTERS] = {
    CART_ACT_DEFAULTS, CART_ACT_DEFAULTS, CART_ACT_DEFAULTS,
    CART_ACT_DEFAULTS, CART_ACT_DEFAULTS,
};

void board_reset_icecap_scores(struct save *save)
{
    static const struct save_scores defaults = {{9000, 6000, 3000}};
    struct save_scores *scores;
    int i;

    for (i = 0, scores = save->icecap_scores; i < BOARD_PLAYERS;
         i++, scores++) {
        *scores = defaults;
    }
}

void board_reset_sunaboard_scores(struct save *save)
{
    static const struct save_scores defaults = {{10000, 6000, 3000}};
    struct save_scores *scores;
    int i;

    for (i = 0, scores = save->sunaboard_scores; i < BOARD_PLAYERS;
         i++, scores++) {
        *scores = defaults;
    }
}

void cart_reset_times(struct save *save)
{
    static const struct save_cart_times defaults = CART_DEFAULTS;
    struct save_cart_times *times;
    int i;

    for (i = 0, times = save->cart_times; i < SAVE_CHARACTERS; i++, times++) {
        *times = defaults;
    }
}

int board_record_icecap_score(void)
{
    /* @hack redundant: register on count, which keeps SAVE_BEST in R13 (MOV
     * #3,R13 at 0x8c0a7ca4); as a plain local, 11 of 59 words differ, and
     * with SAVE_BEST in its place, 11 of 59 too */
    register int count = SAVE_BEST;
    /* @hack redundant: register on rank, which keeps it in R14 (MOV R13,R14
     * at 0x8c0a7cac); without it, 15 of 59 words differ */
    register int rank = count;
    int character = minigame_character();
    int points;
    Sint32 *scores;
    Sint32 *cursor;
    int i;

    if (character < BOARD_PLAYERS) {
        points = score;
        scores = saves[save_slot].icecap_scores[character].score;
        for (i = 0, cursor = scores; i < count; i++, cursor++) {
            if (points >= *cursor) {
                break;
            }
        }
        rank = i;
        if (rank < count) {
            for (i = count - 1, cursor = scores + count - 1; i > rank;
                 i--, cursor--) {
                *cursor = cursor[-1];
            }
            *cursor = points;
        }
    }
    return rank;
}

int board_record_sunaboard_score(void)
{
    /* @hack redundant: register on count, which keeps SAVE_BEST in R13 (MOV
     * #3,R13 at 0x8c0a7d44); as a plain local, 11 of 59 words differ */
    register int count = SAVE_BEST;
    /* @hack redundant: register on rank, which keeps it in R14 (MOV R13,R14
     * at 0x8c0a7d4c); without it, 15 of 59 words differ */
    register int rank = count;
    int character = minigame_character();
    int points;
    Sint32 *scores;
    Sint32 *cursor;
    int i;

    if (character < BOARD_PLAYERS) {
        points = score;
        scores = saves[save_slot].sunaboard_scores[character].score;
        for (i = 0, cursor = scores; i < count; i++, cursor++) {
            if (points > *cursor) {
                break;
            }
        }
        rank = i;
        if (rank < count) {
            for (i = count - 1, cursor = scores + count - 1; i > rank;
                 i--, cursor--) {
                *cursor = cursor[-1];
            }
            *cursor = points;
        }
    }
    return rank;
}

int cart_record_time(void)
{
    struct cart_time run;
    int run_value;
    struct save_cart_times *times;
    struct save_time *records;
    int rank;
    int i;
    struct save_time *cursor;
    Sint8 lap;
    struct cart_time split;

    run = cart_get_time();
    run_value = (((run.minutes * 60) + run.seconds) * 100) + run.hundredths;
    times = cart_find_times();
    records = times->time;
    for (i = 0, cursor = records; i < SAVE_BEST; i++, cursor++) {
        int value =
            (((cursor->minutes * 60) + cursor->seconds) * 100) + cursor->frames;
        if (run_value <= value) {
            break;
        }
    }
    rank = i;
    if (rank < SAVE_BEST) {
        for (i = SAVE_BEST - 1, cursor = records + SAVE_BEST - 1; i > rank;
             i--, cursor--) {
            *cursor = cursor[-1];
        }
        cursor->minutes = run.minutes;
        cursor->seconds = run.seconds;
        cursor->frames = run.hundredths;
        if (rank == 0) {
            for (lap = 0, cursor = times->time + SAVE_BEST;
                 lap < SAVE_CART_SPLITS; lap++, cursor++) {
                split = cart_get_split(lap);
                cursor->minutes = split.minutes;
                cursor->seconds = split.seconds;
                cursor->frames = split.hundredths;
            }
        }
    }
    return rank;
}

void cart_copy_times(struct save_cart_times *out)
{
    if (out != NULL) {
        struct save_cart_times *times = cart_find_times();

        *out = *times;
    }
}

/* the times of the character playing in the current act: the save's for
 * the cart's first act, else cart_act_times'. a character the save keeps
 * no times for takes Sonic's */
static struct save_cart_times *cart_find_times(void)
{
    int character = minigame_character();
    int act;
    struct save_cart_times *times;

    if (character >= SAVE_CHARACTERS) {
        character = 0;
    }
    act = cart_get_act();
    switch ((Uint8)act) {
    case 0:
        times = &saves[save_slot].cart_times[character];
        break;
    default:
        times = &cart_act_times[(Uint8)act - 1][character];
        break;
    }
    return times;
}

/* the index of the character playing among a save's records, or
 * SAVE_CHARACTERS for a character it keeps none for */
static int minigame_character(void)
{
    int character = get_character();

    switch (character) {
    case CHARACTER_SONIC:
        character = 0;
        break;
    case CHARACTER_TAILS:
        character = 1;
        break;
    case CHARACTER_KNUCKLES:
        character = 2;
        break;
    case CHARACTER_AMY:
        character = 3;
        break;
    case CHARACTER_E102:
        character = 4;
        break;
    case CHARACTER_BIG:
        character = 5;
        break;
    default:
        character = SAVE_CHARACTERS;
        break;
    }
    return character;
}

/* stage_get_stage_act() if its act is one of the cart's after the first,
 * else 0 */
static int cart_get_act(void)
{
    int stage_act = stage_get_stage_act();

    if (!((Uint8)stage_act > 0 && (Uint8)stage_act < CART_ACTS)) {
        stage_act = 0;
    }
    return stage_act;
}
