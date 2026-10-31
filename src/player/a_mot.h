/* the actions 1ST_READ.BIN names in a_mot.PRS, Amy's motion file.
 * loader_read_character_motions reads the file to MOTION_AREA for her.
 * each is valid only while the file is loaded */
#ifndef PLAYER_A_MOT_H
#define PLAYER_A_MOT_H
#include <ninja.h>

/* the actions amy_actions names. a comment gives an action's rows where its
 * name doesn't give them all, by enum amy_action_index, or by number where
 * her code names no row */
extern NJS_ACTION amy_hurt_action;
extern NJS_ACTION amy_hurt_land_action;
extern NJS_ACTION amy_down_action;
extern NJS_ACTION amy_down_land_action;
/* ACTION_HANG_ON_SLOW, ACTION_HANG_ON */
extern NJS_ACTION amy_hang_on_slow_action;
extern NJS_ACTION amy_handle_action;
extern NJS_ACTION dat_0cc3cb8c; /* row 34 */
extern NJS_ACTION amy_balance_start_action;
extern NJS_ACTION amy_balance_action;
extern NJS_ACTION amy_drown_action;
extern NJS_ACTION amy_take_drum_action;
/* ACTION_RAISE_DRUM, ACTION_SET_DRUM */
extern NJS_ACTION amy_raise_drum_action;
extern NJS_ACTION dat_0cc18708; /* row 40 */
extern NJS_ACTION amy_lift_action; /* ACTION_LIFT, ACTION_PUT_DOWN */
extern NJS_ACTION amy_hold_run_action;
extern NJS_ACTION amy_hold_stop_action;
extern NJS_ACTION amy_hold_turn_action;
extern NJS_ACTION amy_hold_stand_action;
extern NJS_ACTION amy_hold_walk_action;
extern NJS_ACTION amy_pull_action;
extern NJS_ACTION amy_shake_action;
extern NJS_ACTION amy_hold_shake_action;
extern NJS_ACTION dat_0cc1beb0; /* row 59 */
extern NJS_ACTION amy_hold_shake_end_action;
extern NJS_ACTION amy_grip_start_action; /* ACTION_GRIP_START, ACTION_ROCKET */
extern NJS_ACTION amy_grip_action; /* ACTION_GRIP, row 78 */
extern NJS_ACTION amy_hold_jump_action;
extern NJS_ACTION amy_hold_fall_start_action;
extern NJS_ACTION amy_hold_fall_action;
extern NJS_ACTION amy_hold_land_fast_action;
/* ACTION_THROW_AT, ACTION_AIR_THROW_AT */
extern NJS_ACTION amy_throw_at_action;
extern NJS_ACTION amy_throw_action; /* ACTION_THROW, ACTION_AIR_THROW */
extern NJS_ACTION amy_ledge_action;
extern NJS_ACTION amy_caught_action; /* ACTION_CAUGHT, ACTION_DIZZY */
extern NJS_ACTION amy_rocket_along_action;
extern NJS_ACTION amy_rocket_up_action;
extern NJS_ACTION amy_hammer_start_action;
extern NJS_ACTION amy_hammer_action;
extern NJS_ACTION amy_hammer_end_action;
extern NJS_ACTION amy_spin_start_action;
extern NJS_ACTION amy_spin_wind_up_action;
extern NJS_ACTION amy_spin_action;
extern NJS_ACTION amy_dizzy_walk_action;
extern NJS_ACTION amy_hammer_jump_action;
extern NJS_ACTION amy_hammer_leap_action;
extern NJS_ACTION amy_hammer_leap_2_action;
extern NJS_ACTION amy_hammer_leap_3_action;
extern NJS_ACTION amy_hammer_leap_4_action;
/* ACTION_HAMMER_JUMP_LAND, ACTION_AIR_HAMMER_LAND */
extern NJS_ACTION amy_hammer_jump_land_action;
extern NJS_ACTION amy_air_hammer_action;

/* the drum's own actions, which amy_draw_drum plays */
extern NJS_ACTION amy_drum_carry_action;
extern NJS_ACTION amy_drum_take_action; /* as she takes it or sets it down */

#endif
