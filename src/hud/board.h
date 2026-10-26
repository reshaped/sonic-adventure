/* the board stages' hud: a score under its own label, where the time
 * would be */
#ifndef HUD_BOARD_H
#define HUD_BOARD_H
#include <ninja.h>
#include <sg_xpt.h>

/* the score's label, which sboard's and stg08's texture tables load */
extern NJS_TEXLIST board_score_texlist;

/* draw score in five digits, leading zeros and all. a score past 99999
 * shows as 99999 */
void hud_draw_board_score(int score);
/* draw the score's and the rings' labels, in place of the time's and the
 * rings' */
void hud_draw_board_labels(void);
/* TRUE in the board stages: STAGE_SUNABOARD, and STAGE_ICECAP's act 2 in
 * GAME_MODE_TRIAL */
Bool board_active(void);
/* TRUE in STAGE_CART, whose minicart module draws a hud of its own */
Bool cart_active(void);

#endif
