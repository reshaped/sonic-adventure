/* the actions 1ST_READ.BIN names in k_mot.PRS, Knuckles' motion file.
 * loader_read_character_motions reads the file to MOTION_AREA for him. each
 * is valid only while the file is loaded */
#ifndef PLAYER_K_MOT_H
#define PLAYER_K_MOT_H
#include <ninja.h>

/* the actions knuckles_actions names. a comment gives an action's rows
 * where its name doesn't give them all */
extern NJS_ACTION knuckles_drown_action;
extern NJS_ACTION knuckles_clear_action;
extern NJS_ACTION knuckles_clear_once_action;
/* ACTION_DIG_START, ACTION_DIG_FAIL */
extern NJS_ACTION knuckles_dig_start_action;
extern NJS_ACTION knuckles_dig_action;
extern NJS_ACTION knuckles_dig_up_action;
extern NJS_ACTION knuckles_dig_out_action;
extern NJS_ACTION knuckles_lift_action; /* ACTION_LIFT, ACTION_PUT_DOWN */
extern NJS_ACTION knuckles_hold_run_action;
extern NJS_ACTION knuckles_hold_stop_action;
extern NJS_ACTION knuckles_hold_turn_action;
extern NJS_ACTION knuckles_hold_stand_action;
extern NJS_ACTION knuckles_hold_walk_action;
extern NJS_ACTION knuckles_pull_action;
extern NJS_ACTION knuckles_hold_shake_start_action;
extern NJS_ACTION knuckles_hold_shake_action;
extern NJS_ACTION knuckles_hold_shake_end_action;
extern NJS_ACTION knuckles_hold_jump_action;
extern NJS_ACTION knuckles_hold_fall_start_action;
extern NJS_ACTION knuckles_hold_fall_action;
extern NJS_ACTION knuckles_hold_hard_landing_action;
extern NJS_ACTION knuckles_hang_go_action;
extern NJS_ACTION knuckles_hang_at_32_action;
extern NJS_ACTION knuckles_hang_at_0_action;
extern NJS_ACTION knuckles_ledge_action;
extern NJS_ACTION knuckles_rocket_along_action;
extern NJS_ACTION knuckles_rocket_up_action;

#endif
