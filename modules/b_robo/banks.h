/* Zero's banks: EROBO.PB's textures and EROBO.NB's models and motions. the
 * arms and the actions point into them */
#ifndef B_ROBO_BANKS_H
#define B_ROBO_BANKS_H
#include <ninja.h>
#include <sg_xpt.h>

/* where the model bank was read, and the texture bank's texlist */
extern Uint8 *erobo_models;
extern NJS_TEXLIST *erobo_textures;
/* the arms' objects and Zero's actions, which erobo_load points into the
 * model bank */
extern NJS_OBJECT erobo_arm0_object;
extern NJS_OBJECT erobo_arm1_object;
extern NJS_ACTION erobo_beam_action;
extern NJS_ACTION erobo_wind_up_action;
extern NJS_ACTION erobo_throw_action;
extern NJS_ACTION erobo_throw_end_action;
extern NJS_ACTION erobo_hand_out_action;
extern NJS_ACTION erobo_hand_home_action;
extern NJS_ACTION erobo_hit_front_action;
extern NJS_ACTION erobo_hit_front_land_action;
extern NJS_ACTION erobo_hit_behind_action;
extern NJS_ACTION erobo_hit_behind_land_action;
extern NJS_ACTION erobo_turn_around_action;
extern NJS_ACTION erobo_leap_action;
extern NJS_ACTION erobo_leap_fall_action;
extern NJS_ACTION erobo_walk_action;
extern NJS_ACTION erobo_stand_action;
extern NJS_ACTION erobo_shocked_action;
extern NJS_ACTION erobo_knocked_down_action;
extern NJS_ACTION erobo_hurt_action;
extern NJS_ACTION erobo_get_up_action;
extern NJS_ACTION erobo_spin_action;

/* read both banks into the memory past the module's end, the textures
 * first. then point the arms and the actions into the models.
 * erobo_textures is NULL when pvb_load can't allocate its arrays */
void erobo_load(void);
/* release the textures, and set erobo_textures to NULL. the models hold
 * nothing to release */
void erobo_release(void);

#endif
