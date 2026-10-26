/* the blue, green, purple, sky, white and yellow emeralds' texture names and
 * lists */
#include <ninja.h>

#include "ev00f7/emerald_textures.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 15
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 15
#define TEXT_6_LENGTH 15
#include "ev00f7/emerald_textures/text.inc"

static NJS_TEXNAME ev00f7_emerald_blue_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev00f7_emerald_blue_texlist = {
#include "ev00f7/emerald_textures/ev00f7_emerald_blue_texlist.inc"
};
static NJS_TEXNAME ev00f7_emerald_green_textures[1] = {
    {TEXT_2, 0, 0},
};
NJS_TEXLIST ev00f7_emerald_green_texlist = {
#include "ev00f7/emerald_textures/ev00f7_emerald_green_texlist.inc"
};
static NJS_TEXNAME ev00f7_emerald_purple_textures[1] = {
    {TEXT_3, 0, 0},
};
NJS_TEXLIST ev00f7_emerald_purple_texlist = {
#include "ev00f7/emerald_textures/ev00f7_emerald_purple_texlist.inc"
};
static NJS_TEXNAME ev00f7_emerald_sky_textures[1] = {
    {TEXT_4, 0, 0},
};
NJS_TEXLIST ev00f7_emerald_sky_texlist = {
#include "ev00f7/emerald_textures/ev00f7_emerald_sky_texlist.inc"
};
static NJS_TEXNAME ev00f7_emerald_white_textures[1] = {
    {TEXT_5, 0, 0},
};
NJS_TEXLIST ev00f7_emerald_white_texlist = {
#include "ev00f7/emerald_textures/ev00f7_emerald_white_texlist.inc"
};
static NJS_TEXNAME ev00f7_emerald_yellow_textures[1] = {
    {TEXT_6, 0, 0},
};
NJS_TEXLIST ev00f7_emerald_yellow_texlist = {
#include "ev00f7/emerald_textures/ev00f7_emerald_yellow_texlist.inc"
};
