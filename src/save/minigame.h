/* minigames: the boards' best scores and the cart's best times */
#ifndef SAVE_MINIGAME_H
#define SAVE_MINIGAME_H
#include <sg_xpt.h>

struct save;
struct save_cart_times;

/* a time as cart_time_of gives it: a sign, and the magnitude up to
 * 99:59.99 */
struct cart_time {
    Sint8 sign; /* 1, 0 or -1, as the time is above, at or below 0 */
    Sint8 minutes;
    Sint8 seconds;
    Sint8 hundredths;
};

/* set save's scores on a board, or its cart times, to their defaults. a
 * board keeps scores for Sonic and Tails, and the cart keeps times for the
 * six characters a save keeps records for */
void board_reset_icecap_scores(struct save *save);
void board_reset_sunaboard_scores(struct save *save);
void cart_reset_times(struct save *save);
/* put score, the current score, among the board's best scores for the
 * character playing, in the save the game plays. it returns the score's
 * place: 0 for the best, or SAVE_BEST for none. the save keeps only Sonic's
 * and Tails' scores, so any other character's place is SAVE_BEST. Ice Cap
 * puts a score ahead of those it ties, and Sand Hill after them */
int board_record_icecap_score(void);
int board_record_sunaboard_score(void);
/* put the run's time among the cart's best times for the character
 * playing in the current act, and return its place as the boards do. a
 * time goes ahead of those it ties, and a new best keeps the run's splits
 * too. the save the game plays keeps the first act's times, and the other
 * acts' are kept in memory alone. a character the save keeps no times for
 * takes Sonic's */
int cart_record_time(void);
/* copy the cart's times for the character playing in the current act to
 * out, unless out is NULL */
void cart_copy_times(struct save_cart_times *out);

#endif
