/* the actions 1ST_READ.BIN names in e_mot.PRS, E-102's motion file.
 * loader_read_character_motions reads the file to MOTION_AREA for him.
 * each is valid only while the file is loaded */
#ifndef PLAYER_E_MOT_H
#define PLAYER_E_MOT_H
#include <ninja.h>

/* the actions e102_actions names. a comment gives an action's rows where
 * its name doesn't give them all, by enum e102_action, or by number where his
 * code names no row */
extern NJS_ACTION e102_lift_action; /* ACTION_LIFT, ACTION_PUT_DOWN */
/* ACTION_HOLD_RUN, ACTION_HOLD_RUN_PACED */
extern NJS_ACTION e102_hold_run_action;
/* ACTION_HOLD_SKATE_TURN, ACTION_HOLD_TURN */
extern NJS_ACTION e102_hold_skate_turn_action;
extern NJS_ACTION e102_hold_walk_action;
extern NJS_ACTION e102_hold_jump_action;
extern NJS_ACTION e102_hold_fall_start_action;
extern NJS_ACTION e102_hold_fall_action;
extern NJS_ACTION e102_hold_hard_landing_action;
extern NJS_ACTION e102_hold_jump_start_action;
/* ACTION_HOLD_WATER_START, ACTION_HOLD_WATER_TO_SKATE */
extern NJS_ACTION e102_hold_water_start_action;
extern NJS_ACTION e102_hold_skate_to_water_action;
extern NJS_ACTION e102_hold_water_skate_action;
/* ACTION_HOLD_SKATE_START, ACTION_HOLD_SKATE_END */
extern NJS_ACTION e102_hold_skate_start_action;
extern NJS_ACTION e102_hold_skate_action;
extern NJS_ACTION e102_ledge_action;
extern NJS_ACTION dat_0cc29324; /* row 76 */
extern NJS_ACTION dat_0cc2b7c8; /* row 77 */
extern NJS_ACTION dat_0cc2e3ec; /* row 78 */
extern NJS_ACTION dat_0cc30570; /* row 79 */
extern NJS_ACTION dat_0cc32df4; /* row 80 */
extern NJS_ACTION dat_0cc35bb8; /* row 81 */
extern NJS_ACTION e102_hold_shake_start_action;
extern NJS_ACTION e102_hold_shake_action;
extern NJS_ACTION e102_hold_shake_end_action;

#endif
