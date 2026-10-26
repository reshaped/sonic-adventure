/* the models the objects draw, alone or in trees, and their actions */
#ifndef OBJECT_MODELS_H
#define OBJECT_MODELS_H
#include <ninja.h>

/* the savepoint's gate. its child is a lamp at x 15, and that lamp's
 * sibling the other, at x -15 */
extern NJS_OBJECT savepoint_gate;
/* @hack type: the lamps as arrays, whose uses load each lamp's own address
 * to reach its model, as the image's do (MOV.L @(4,R4),R2 at 0x8c068924);
 * as single objects, savepoint_draw runs 32 bytes short and 124 of its and
 * lamp_draw's 175 words differ */
extern NJS_OBJECT savepoint_lamp_plus_x[];
extern NJS_OBJECT savepoint_lamp_minus_x[];

extern NJS_OBJECT iron_ball_object;
extern NJS_OBJECT iron_ball_link_object;
extern NJS_OBJECT iron_ball_joint_object;

extern NJS_MODEL capsule_base_model;
extern NJS_MODEL capsule_body_model;
extern NJS_MODEL capsule_button_model;

/* the target E-102 shoots down */
extern NJS_OBJECT target_object;

extern NJS_MODEL item_box_base_model;
extern NJS_MODEL item_box_glass_model;
extern NJS_MODEL item_box_top_model;
extern NJS_MODEL item_box_icon_model;

/* a level item's stand: its wave, its light, its floor and the wave's
 * action */
/* @hack type: the wave, the light and the action as arrays, whose uses load
 * each one's own address to reach its fields, as the image's do (MOV.L
 * @(4,R3),R2 at 0x8c06d348); as single objects, 352 of the 950 words of
 * level_item_step, level_item_draw_stand, level_item_draw_waves,
 * level_item_animate and level_item_take differ */
extern NJS_OBJECT level_item_wave_object[];
extern NJS_OBJECT level_item_light_object[];
extern NJS_OBJECT level_item_floor_object;
extern NJS_ACTION level_item_wave_action[];

extern NJS_OBJECT emerald_piece_object;

/* a jump panel. panel_end_object is panel 0, which only the set editor
 * shows */
extern NJS_OBJECT panel_object;
extern NJS_OBJECT panel_number_object;
extern NJS_OBJECT panel_end_object;

extern NJS_OBJECT item_box_air_object;
extern NJS_OBJECT item_box_air_panel;
extern NJS_OBJECT item_box_air_lower;

/* a plate of the toge, with the spike that rises out of it as its child */
extern NJS_OBJECT toge_object;

/* a switch. the plate is what a player stands on in the world, and the
 * button's materials 3 and 4 are its lamp */
extern NJS_MODEL switch_plate_model;
extern NJS_MATERIAL switch_button_materials[];
extern NJS_MODEL switch_button_model;
extern NJS_MODEL switch_base_model;

/* a rocket's launcher and the action that opens it, then the actions of a
 * rocket that flies along and of one that flies up */
/* @hack type: the launcher as an array, whose uses load its own address
 * to reach its model, as the image's do (MOV.L at 0x8c066cea); as a single
 * object, 16 of rocket_launcher_init_h's and rocket_launcher_init_v's 116
 * words differ */
extern NJS_OBJECT rocket_launcher_object[];
extern NJS_ACTION rocket_launcher_action;
extern NJS_ACTION rocket_h_action;
extern NJS_ACTION rocket_v_action;

/* a spring and spring b: the base, and the coil, whose child is the top */
extern NJS_MODEL spring_base_model;
extern NJS_OBJECT spring_coil_object;
extern NJS_OBJECT spring_top_object;
extern NJS_MODEL spring_b_base_model;
extern NJS_OBJECT spring_b_coil_object;
extern NJS_OBJECT spring_b_top_object;

extern NJS_MODEL spring_h_base_model;
extern NJS_MODEL spring_h_board_model;

/* the square a shadow is drawn with, and its model */
/* @hack type: the square as an array, whose address shadow_draw_at_height
 * loads to reach its model, as the image does (MOV.L @(4,R4),R4 at
 * 0x8c0973d2); as a single object, 2 of that function's 84 words differ.
 * its other uses pass only its address */
extern NJS_OBJECT shadow_object[];
extern NJS_MODEL shadow_model;

/* the hint box, whose model's material 10 is its lamp */
/* @hack type: the box as an array, whose use loads its own address to
 * reach its model, as the image's does (MOV.L at 0x8c0643a6); as a single
 * object, 5 of hint_copy_model's 57 words differ */
extern NJS_OBJECT hint_object[];

extern NJS_ACTION dash_panel_action;

extern NJS_OBJECT dash_ring_object;

extern NJS_OBJECT kusa_object;
extern NJS_OBJECT kusa_cut_object;
extern NJS_OBJECT kusa_leaf_object;

/* the balloon at the goal of Amy's stages */
extern NJS_MODEL balloon_model;
extern NJS_MODEL balloon_knot_model;
extern NJS_MODEL balloon_sign_model;
extern NJS_MODEL balloon_sign_frame_model;

extern NJS_MODEL ring_model;
extern NJS_OBJECT ring_object;

extern NJS_OBJECT ripple_object;
/* the stars that burst from a player who crashes */
extern NJS_OBJECT star_yellow_object;
extern NJS_OBJECT star_orange_object;

#endif
