/* the definitions 1ST_READ.BIN names in s_sbmot.PRS, Sonic's board motion
 * file. loader_read_stage_files reads the file to SONIC_BOARD_AREA for
 * STAGE_SUNABOARD, and icecap_act_start reads it there for Ice Cap. both
 * read it only while the character is Sonic or Tails. each is valid only
 * while the file is loaded */
#ifndef PLAYER_S_SBMOT_H
#define PLAYER_S_SBMOT_H
#include <ninja.h>

/* the board's model, and a texlist of one texture.
 * sonic_board_start gives the board that texlist on an odd play time */
extern NJS_OBJECT sonic_board_object;
extern NJS_TEXLIST sb_board1_texlist;

/* the actions sonic_actions names. each one's comment gives the rows that
 * name it, by enum sonic_action or by index where the enum has none */
extern NJS_ACTION dat_0cb24580; /* ACTION_BOARD */
extern NJS_ACTION dat_0cb3328c; /* ACTION_BOARD_BRAKE_PLUS */
extern NJS_ACTION dat_0cb31228; /* ACTION_BOARD_BRAKE_MINUS */
extern NJS_ACTION dat_0cb2980c; /* ACTION_BOARD_AIR */
extern NJS_ACTION dat_0cb27d88; /* ACTION_BOARD_JUMP */
extern NJS_ACTION dat_0cb35110; /* ACTION_BOARD_LEAN_PLUS */
extern NJS_ACTION dat_0cb36f84; /* ACTION_BOARD_LEAN_MINUS */
extern NJS_ACTION dat_0cb2f4b4; /* ACTION_BOARD_CRASH_PLUS */
extern NJS_ACTION dat_0cb2c750; /* ACTION_BOARD_CRASH_MINUS */
extern NJS_ACTION dat_0cb1e5b4; /* ACTION_BOARD_FALL_START */
extern NJS_ACTION dat_0cb20168; /* ACTION_BOARD_FALL */
extern NJS_ACTION dat_0cb2316c; /* ACTION_BOARD_HARD_LANDING */
extern NJS_ACTION dat_0cb1ad0c; /* ACTION_BOARD_SLOW */
extern NJS_ACTION dat_0cb1c930; /* ACTION_BOARD_STRAIGHTEN */
extern NJS_ACTION dat_0cb12974; /* row 116 */
extern NJS_ACTION dat_0cb19108; /* ACTION_TRICK_BIG */
extern NJS_ACTION dat_0cb46fd8; /* ACTION_TRICK, ACTION_TRICK_BACK */
extern NJS_ACTION dat_0cb099dc; /* ACTION_TRICK_MEDIUM */
extern NJS_ACTION dat_0cb39f30; /* row 121 */
extern NJS_ACTION dat_0cb3b694; /* row 122 */
extern NJS_ACTION dat_0cb0d880; /* ACTION_TRICK_SMALL */
extern NJS_ACTION dat_0cb26314; /* ACTION_BOARD_CROUCH */

/* the board's own actions, which sonic_board_actions names. each one's
 * comment gives the rows of sonic_actions in which the board plays it, by
 * enum sonic_action or by index where the enum has none */
extern NJS_ACTION dat_0cb38c24; /* ACTION_BOARD_BRAKE_PLUS */
extern NJS_ACTION dat_0cb38af0; /* ACTION_BOARD_BRAKE_MINUS */
extern NJS_ACTION dat_0cb38574; /* ACTION_BOARD_AIR */
extern NJS_ACTION dat_0cb38440; /* ACTION_BOARD_JUMP */
extern NJS_ACTION dat_0cb38e68; /* ACTION_BOARD_LEAN_PLUS */
extern NJS_ACTION dat_0cb390ac; /* ACTION_BOARD_LEAN_MINUS */
extern NJS_ACTION dat_0cb389bc; /* ACTION_BOARD_CRASH_PLUS */
extern NJS_ACTION dat_0cb38798; /* ACTION_BOARD_CRASH_MINUS */
extern NJS_ACTION dat_0cb381c8; /* ACTION_BOARD_FALL_START */
extern NJS_ACTION dat_0cb382fc; /* ACTION_BOARD_FALL */
extern NJS_ACTION dat_0cb37c70; /* row 116 */
extern NJS_ACTION dat_0cb38094; /* ACTION_TRICK_BIG */
extern NJS_ACTION dat_0cb47e34; /* ACTION_TRICK, ACTION_TRICK_BACK */
extern NJS_ACTION dat_0cb375b8; /* ACTION_TRICK_MEDIUM */
extern NJS_ACTION dat_0cb470fc; /* row 121 */
extern NJS_ACTION dat_0cb47230; /* row 122 */
extern NJS_ACTION dat_0cb3784c; /* ACTION_TRICK_SMALL */

#endif
