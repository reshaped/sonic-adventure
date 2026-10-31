/* the actions 1ST_READ.BIN names in b_mot.PRS, Big's motion file.
 * loader_read_character_motions reads the file to MOTION_AREA for him.
 * each is valid only while the file is loaded */
#ifndef PLAYER_B_MOT_H
#define PLAYER_B_MOT_H
#include <ninja.h>

/* the actions big_actions names. a comment gives an action's rows where
 * its name doesn't give them all */
/* ACTION_PUSH_START, ACTION_GRIP_START */
extern NJS_ACTION big_push_start_action;
extern NJS_ACTION big_lift_action; /* ACTION_LIFT, ACTION_PUT_DOWN */
extern NJS_ACTION big_hold_run_action;
/* ACTION_HOLD_STOP, ACTION_HOLD_TURN, ACTION_HOLD_STAND, ACTION_HANG_ON */
extern NJS_ACTION big_hold_stand_action;
extern NJS_ACTION big_hold_walk_action;
extern NJS_ACTION big_pull_action;
extern NJS_ACTION big_shake_action;
extern NJS_ACTION big_grip_action;
extern NJS_ACTION big_hold_jump_action;
extern NJS_ACTION big_hold_fall_start_action;
extern NJS_ACTION big_hold_fall_action;
extern NJS_ACTION big_hold_land_action;
/* ACTION_THROW_AT, ACTION_AIR_THROW_AT */
extern NJS_ACTION big_throw_at_action;
extern NJS_ACTION big_throw_action; /* ACTION_THROW, ACTION_AIR_THROW */
extern NJS_ACTION big_ledge_action;
extern NJS_ACTION big_flung_action;
extern NJS_ACTION big_clear_action;
extern NJS_ACTION big_swing_action;
extern NJS_ACTION big_reel_action; /* ACTION_REEL, ACTION_REEL_HELD */
extern NJS_ACTION big_cast_start_action;
extern NJS_ACTION big_cast_action; /* ACTION_CAST, ACTION_CAST_SWING */
extern NJS_ACTION big_reel_down_action;
extern NJS_ACTION big_reel_down_right_action;
extern NJS_ACTION big_reel_down_left_action;
extern NJS_ACTION big_reel_right_action;
extern NJS_ACTION big_reel_left_action;
extern NJS_ACTION big_catch_small_action;
extern NJS_ACTION big_catch_medium_action;
extern NJS_ACTION big_catch_large_action;
extern NJS_ACTION big_snap_action;
extern NJS_ACTION big_snapped_action;
/* the action of big_actions' row 65, a row his code doesn't name */
extern NJS_ACTION dat_0cc61540;
extern NJS_ACTION big_float_cast_start_action;
/* ACTION_FLOAT_CAST, ACTION_FLOAT_REEL_HELD */
extern NJS_ACTION big_float_cast_action;
extern NJS_ACTION big_float_reel_down_action;
extern NJS_ACTION big_float_reel_down_right_action;
extern NJS_ACTION big_float_reel_down_left_action;
extern NJS_ACTION big_float_reel_right_action;
extern NJS_ACTION big_float_reel_left_action;
extern NJS_ACTION big_hold_small_action;
extern NJS_ACTION big_hold_small_walk_action;
extern NJS_ACTION big_hold_small_run_action;
/* ACTION_LIFT_SMALL, ACTION_PUT_DOWN_SMALL */
extern NJS_ACTION big_lift_small_action;
extern NJS_ACTION big_swim_start_action;
extern NJS_ACTION big_swim_action;
extern NJS_ACTION big_dive_start_action;
extern NJS_ACTION big_dive_action;
extern NJS_ACTION big_float_under_start_action;
extern NJS_ACTION big_float_under_action;
extern NJS_ACTION big_float_action;
extern NJS_ACTION big_hold_shake_start_action;
extern NJS_ACTION big_hold_shake_action;
extern NJS_ACTION big_hold_shake_end_action;
extern NJS_ACTION big_breathe_action;
extern NJS_ACTION big_drown_action;
extern NJS_ACTION big_down_land_action;

#endif
