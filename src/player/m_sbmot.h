/* the definitions 1ST_READ.BIN names in m_sbmot.PRS, Tails' board motion
 * file. loader_read_stage_files reads the file to TAILS_BOARD_AREA for
 * STAGE_SUNABOARD, and icecap_act_start reads it there for Ice Cap. both
 * read it only while the character is Sonic or Tails. each is valid only
 * while the file is loaded */
#ifndef PLAYER_M_SBMOT_H
#define PLAYER_M_SBMOT_H
#include <ninja.h>

/* the board's model, and a texlist of one texture, mtx_board0.
 * tails_board_start gives the board both */
extern NJS_OBJECT tails_board_object;
extern NJS_TEXLIST tails_board_texlist;

/* the actions tails_actions names. each one's comment gives the rows that
 * name it, by enum tails_action_index or by index where the enum has none */
/* ACTION_BOARD, ACTION_BOARD_BRAKE_A, ACTION_BOARD_BRAKE_B, ACTION_BOARD_SLOW,
 * ACTION_BOARD_SLOW_ONCE */
extern NJS_ACTION dat_0cae1c44;
extern NJS_ACTION dat_0cae63e0; /* ACTION_BOARD_FALL_START */
extern NJS_ACTION dat_0cae4a2c; /* ACTION_BOARD_JUMP */
extern NJS_ACTION dat_0caec15c; /* ACTION_BOARD_LEAN_RIGHT */
extern NJS_ACTION dat_0caea6b8; /* ACTION_BOARD_LEAN_LEFT */
extern NJS_ACTION dat_0caeec50; /* ACTION_BOARD_CRASH_A */
extern NJS_ACTION dat_0cae8c14; /* ACTION_BOARD_CRASH_B */
extern NJS_ACTION dat_0cadd73c; /* ACTION_BOARD_FALL, ACTION_BOARD_TRICK_FALL */
extern NJS_ACTION dat_0cae0210; /* ACTION_BOARD_HARD_LANDING */
extern NJS_ACTION dat_0caf883c; /* ACTION_BOARD_TRICK_1 */
extern NJS_ACTION dat_0cb02590; /* ACTION_BOARD_TRICK_2 */
extern NJS_ACTION dat_0caf25a4; /* ACTION_BOARD_TRICK_3, row 124, row 125 */
extern NJS_ACTION dat_0caf5dc8; /* ACTION_BOARD_TRICK_4 */
extern NJS_ACTION dat_0cae3658; /* ACTION_BOARD_CROUCH */

/* the board's own actions, which tails_board_actions names. each one's
 * comment gives the rows of tails_actions in which the board plays it, by
 * enum tails_action_index or by index where the enum has none */
/* ACTION_BOARD_BRAKE_A, ACTION_BOARD_BRAKE_B, ACTION_BOARD_TRICK_3, row 124,
 * row 125, ACTION_BOARD_TRICK_4 */
extern NJS_ACTION dat_0cb03844;
extern NJS_ACTION dat_0cb02cd0; /* ACTION_BOARD_FALL_START */
extern NJS_ACTION dat_0cb02b7c; /* ACTION_BOARD_JUMP */
extern NJS_ACTION dat_0cb035a0; /* ACTION_BOARD_LEAN_RIGHT */
extern NJS_ACTION dat_0cb02f14; /* ACTION_BOARD_LEAN_LEFT */
extern NJS_ACTION dat_0cb0335c; /* ACTION_BOARD_CRASH_A */
extern NJS_ACTION dat_0cb03138; /* ACTION_BOARD_CRASH_B */
extern NJS_ACTION dat_0cb02a38; /* ACTION_BOARD_TRICK_FALL */
extern NJS_ACTION dat_0cb039c8; /* ACTION_BOARD_TRICK_1 */
extern NJS_ACTION dat_0cb0456c; /* ACTION_BOARD_TRICK_2 */

#endif
