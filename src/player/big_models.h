/* Big's model, the pieces of it and of his rods that code reaches by name,
 * seven of his actions, and the fishing texlist. big_textures.c defines
 * them all */
#ifndef PLAYER_BIG_MODELS_H
#define PLAYER_BIG_MODELS_H
#include <ninja.h>

extern NJS_OBJECT big_object;
extern NJS_OBJECT big_upper_arm_minus_z;
extern NJS_OBJECT big_forearm_minus_z;
extern NJS_OBJECT big_hand_minus_z;
extern NJS_OBJECT big_upper_arm_plus_z;
extern NJS_OBJECT big_forearm_plus_z;
extern NJS_OBJECT big_hand_plus_z;
extern NJS_OBJECT big_thigh_minus_z;
extern NJS_OBJECT big_shin_minus_z;
extern NJS_OBJECT big_thigh_plus_z;
extern NJS_OBJECT big_shin_plus_z;
extern NJS_OBJECT big_foot_minus_z;
extern NJS_OBJECT big_toes_minus_z;
extern NJS_OBJECT big_foot_plus_z;
extern NJS_OBJECT big_toes_plus_z;
extern NJS_OBJECT big_fingers_minus_z;
extern NJS_OBJECT big_fingers_plus_z;
extern NJS_OBJECT big_head_end;
extern NJS_OBJECT big_pupil_plus_z;
extern NJS_OBJECT big_pupil_minus_z;
/* @hack type: big_neck as an array, so that big_step stores its sibling at
 * a displacement from its address (MOV.L R2,@(48,R1) at 0x8c0eabfc and
 * 0x8c0eacd8). as one object, SHC loads the sibling's address from a pool
 * word instead, and 2 of big_step's 1388 words differ */
extern NJS_OBJECT big_neck[];
/* @hack type: big_belt_torso as an array, so that big_step reads its model
 * and child at displacements from its address (MOV.L @(4,R7),R7 at
 * 0x8c0eacb0, MOV.L @(44,R7),R7 at 0x8c0eacbe). as one object, SHC loads
 * each member's address from a pool word instead, and 21 of big_step's 1388
 * words differ */
extern NJS_OBJECT big_belt_torso[];
extern NJS_OBJECT big_torso;
extern NJS_POINT3 big_belt_torso_flat[];
extern NJS_POINT3 big_belt_torso_full[];
extern NJS_POINT3 big_belt_torso_points[];
extern NJS_POINT3 big_belt_piece_flat[];
extern NJS_POINT3 big_belt_piece_full[];
extern NJS_POINT3 big_belt_piece_points[];
/* @hack type: big_rod as an array, so that big_step stores its model and
 * child at displacements from its address (MOV.L R2,@(4,R1) at 0x8c0eace8,
 * MOV.L R2,@(44,R3) at 0x8c0ead4a). as one object, SHC loads each member's
 * address from a pool word instead, and 17 of big_step's 1388 words differ.
 * big_step's last literals share a pool with big_choose_mode's, so 1 of
 * big_choose_mode's 4304 words differs too */
extern NJS_OBJECT big_rod[];
extern NJS_MODEL big_power_rod_model;
extern NJS_OBJECT big_power_rod_reel;
extern NJS_OBJECT big_power_rod_object;
extern NJS_MODEL big_rod_model;
extern NJS_OBJECT big_rod_reel;
extern NJS_OBJECT big_mouth;
/* @hack type: big_head as an array, so that big_sway_destroy and
 * big_sway_step store its model at a displacement from its address
 * (MOV.L R1,@(4,R3) at 0x8c0eff58, MOV.L R2,@(4,R3) at 0x8c0f0286). as one
 * object, SHC loads the model's address from a pool word instead, and 4 of
 * big_sway_destroy's 5 words and 3 of big_sway_step's 305 differ */
extern NJS_OBJECT big_head[];
extern NJS_OBJECT big_head_blend;
extern NJS_MOTION big_mouth_shapes;
extern NJS_MODEL big_head_model;
extern NJS_MODEL big_head_sway_model;

extern NJS_ACTION big_stand_action;
extern NJS_ACTION big_land_action;
extern NJS_ACTION big_walk_action;
extern NJS_ACTION big_run_action;
extern NJS_ACTION big_jump_action;
extern NJS_ACTION big_fall_start_action;
extern NJS_ACTION big_fall_action;

/* texture_load_character loads it for Big, from the pvm FISHING. the lures
 * draw with it, as do fishing.c's target, hud and splash */
extern NJS_TEXLIST fishing_texlist;

#endif
