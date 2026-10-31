/* the definitions 1ST_READ.BIN names in s_mot.PRS, Sonic's motion file.
 * loader_read_character_motions reads the file to MOTION_AREA for Sonic or
 * Tails. each is valid only while the file is loaded */
#ifndef PLAYER_S_MOT_H
#define PLAYER_S_MOT_H
#include <ninja.h>

/* the aura's model, on which aura_draw draws Sonic's action */
extern NJS_OBJECT sonic_aura_object;
/* the head of the aura's model. sonic_look_step turns it as it turns
 * player 0's head */
extern NJS_OBJECT sonic_aura_head;

/* the actions sonic_actions names. a comment gives an action's rows where
 * its name doesn't give them all, by enum sonic_action or by index where
 * the enum has none */
extern NJS_ACTION sonic_updraft_action;
extern NJS_ACTION sonic_lift_action; /* ACTION_LIFT, ACTION_PUT_DOWN */
extern NJS_ACTION sonic_hold_run_action;
extern NJS_ACTION sonic_hold_stop_action;
extern NJS_ACTION sonic_hold_turn_action;
extern NJS_ACTION sonic_hold_stand_action;
extern NJS_ACTION sonic_hold_walk_action;
extern NJS_ACTION sonic_hang_on_action;
extern NJS_ACTION sonic_pull_action;
extern NJS_ACTION sonic_shake_action;
extern NJS_ACTION sonic_hold_shake_start_action;
extern NJS_ACTION sonic_hold_shake_action;
extern NJS_ACTION sonic_hold_shake_end_action;
/* ACTION_GRIP_START, ACTION_ROCKET */
extern NJS_ACTION sonic_grip_start_action;
extern NJS_ACTION sonic_grip_action; /* ACTION_GRIP, row 78 */
extern NJS_ACTION sonic_hold_jump_action;
extern NJS_ACTION sonic_hold_fall_start_action;
extern NJS_ACTION sonic_hold_fall_action;
extern NJS_ACTION sonic_hold_hard_landing_action;
extern NJS_ACTION sonic_throw_at_action;
extern NJS_ACTION sonic_throw_action;
extern NJS_ACTION sonic_air_throw_at_action;
extern NJS_ACTION sonic_air_throw_action;
extern NJS_ACTION sonic_light_dash_action;
extern NJS_ACTION sonic_ride_action;
extern NJS_ACTION sonic_hang_go_action;
extern NJS_ACTION sonic_hang_halfway_action;
extern NJS_ACTION sonic_hang_action;
extern NJS_ACTION sonic_ledge_action;
extern NJS_ACTION sonic_flung_action;
extern NJS_ACTION sonic_glide_action;
extern NJS_ACTION sonic_panel_land_action;
extern NJS_ACTION sonic_panel_jump_action;
extern NJS_ACTION sonic_clear_action;
extern NJS_ACTION dat_0cc45d6c; /* row 76 */
extern NJS_ACTION sonic_rocket_along_action;
extern NJS_ACTION sonic_rocket_up_action;
extern NJS_ACTION sonic_caught_action;
extern NJS_ACTION sonic_breathe_action;
extern NJS_ACTION sonic_drown_action;

#endif
