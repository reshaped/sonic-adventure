/* the models and textures in AL_MAIN.PRS that the gardens and the race
 * use. AL_MAIN.PRS must stay loaded while they are in use */
#ifndef AL_MAIN_MODELS_H
#define AL_MAIN_MODELS_H
#include <ninja.h>
#include <sg_xpt.h>

extern NJS_TEXLIST alife_objects_texlist;
/* the tree sways through the action's frames 0 to 8 */
extern NJS_ACTION garden_tree_action;
/* a fruit on its branch at three stages of its growth, and then ripe */
extern NJS_MODEL tree_fruit_stage_0;
extern NJS_MODEL tree_fruit_stage_1;
extern NJS_MODEL tree_fruit_stage_2;
extern NJS_MODEL tree_fruit_ripe;
/* a fruit off its tree, and its scale, by its kind. kind 0 has no model */
extern NJS_MODEL *alife_fruit_models[11];
extern Float alife_fruit_scales[11];

extern NJS_TEXLIST alife_chao_texlist;
/* the race's ball: a unit sphere in alife_chao_texlist's texture 212 */
extern NJS_OBJECT alife_ball_object;
extern NJS_SPRITE alife_favorite_sprite;
extern NJS_MODEL dat_0ca50650;
extern NJS_OBJECT alife_flower_object;
extern NJS_OBJECT alife_white_flower_object;
extern NJS_MODEL alife_trumpet_model;
extern NJS_MODEL alife_drum_model;
/* the model at node 4 of the Chao's pose while it plays alife_drum_model */
extern NJS_MODEL dat_0ca4cc08;
/* the cocoon, which alife_chao_show and alife_chao_draw draw while the
 * Chao's flags hold CHAO_IN_COCOON */
extern NJS_OBJECT alife_cocoon_object;

/* the egg, and the eggs a record's CHAO_GOLD, CHAO_PAL and CHAO_BEGG pick
 * in that order, each with the bottom and top its shell breaks into */
extern NJS_OBJECT alife_egg_object;
extern NJS_OBJECT alife_egg_bottom_object;
extern NJS_OBJECT alife_egg_top_object;
extern NJS_OBJECT alife_gold_egg_object;
extern NJS_OBJECT alife_gold_egg_bottom_object;
extern NJS_OBJECT alife_gold_egg_top_object;
extern NJS_OBJECT alife_pal_egg_object;
extern NJS_OBJECT alife_pal_egg_bottom_object;
extern NJS_OBJECT alife_pal_egg_top_object;
extern NJS_OBJECT alife_begg_egg_object;
extern NJS_OBJECT alife_begg_egg_bottom_object;
extern NJS_OBJECT alife_begg_egg_top_object;

/* the font, texture 0, and the window, texture 1 */
extern NJS_TEXLIST alife_window_texlist;

/* the card menu's drive and panel, which the race's menu draws too */
extern NJS_OBJECT card_drive_object;
extern NJS_OBJECT card_panel_object;

#endif
