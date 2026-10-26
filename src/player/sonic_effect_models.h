/* Sonic's fidget action, and the models of his dash, his ball and the ice.
 * the ice is the block ice_create puts a player in */
#ifndef PLAYER_SONIC_EFFECT_MODELS_H
#define PLAYER_SONIC_EFFECT_MODELS_H
#include <ninja.h>

extern NJS_ACTION sonic_fidget_action;
extern NJS_OBJECT sonic_dash_object;
extern NJS_OBJECT sonic_ball_object;
/* the ice's texlist. Ice Cap's and Chaos 6's texture tables load it */
extern NJS_TEXLIST sonic_ice_texlist;
extern NJS_OBJECT sonic_ice_object;

#endif
