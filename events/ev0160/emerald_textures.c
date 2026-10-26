/* the texture names and texlists of every emerald but the blue one.
 * pachacamac_action.c holds the blue emerald's texture names, and
 * ev0160_emerald_blue_texlist lies between that file's data and this one's */
#include <ninja.h>

#include "ev0160/emerald_textures.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 15
#define TEXT_5_LENGTH 15
#define TEXT_6_LENGTH 14
#include "ev0160/emerald_textures/text.inc"

static NJS_TEXNAME ev0160_emerald_green_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev0160_emerald_green_texlist = {
#include "ev0160/emerald_textures/ev0160_emerald_green_texlist.inc"
};
static NJS_TEXNAME ev0160_emerald_purple_textures[1] = {
    {TEXT_2, 0, 0},
};
NJS_TEXLIST ev0160_emerald_purple_texlist = {
#include "ev0160/emerald_textures/ev0160_emerald_purple_texlist.inc"
};
static NJS_TEXNAME ev0160_emerald_sky_textures[1] = {
    {TEXT_3, 0, 0},
};
NJS_TEXLIST ev0160_emerald_sky_texlist = {
#include "ev0160/emerald_textures/ev0160_emerald_sky_texlist.inc"
};
static NJS_TEXNAME ev0160_emerald_white_textures[1] = {
    {TEXT_4, 0, 0},
};
NJS_TEXLIST ev0160_emerald_white_texlist = {
#include "ev0160/emerald_textures/ev0160_emerald_white_texlist.inc"
};
static NJS_TEXNAME ev0160_emerald_yellow_textures[1] = {
    {TEXT_5, 0, 0},
};
NJS_TEXLIST ev0160_emerald_yellow_texlist = {
#include "ev0160/emerald_textures/ev0160_emerald_yellow_texlist.inc"
};
static NJS_TEXNAME ev0160_emerald_red_textures[1] = {
    {TEXT_6, 0, 0},
};
NJS_TEXLIST ev0160_emerald_red_texlist = {
#include "ev0160/emerald_textures/ev0160_emerald_red_texlist.inc"
};
