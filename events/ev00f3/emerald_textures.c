/* the green, purple, sky and white emeralds' texture names and lists */
#include <ninja.h>

#include "ev00f3/emerald_textures.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 15
#include "ev00f3/emerald_textures/text.inc"

static NJS_TEXNAME ev00f3_green_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev00f3_green_emerald_texlist = {
#include "ev00f3/emerald_textures/ev00f3_green_emerald_texlist.inc"
};
static NJS_TEXNAME ev00f3_purple_emerald_textures[1] = {
    {TEXT_2, 0, 0},
};
NJS_TEXLIST ev00f3_purple_emerald_texlist = {
#include "ev00f3/emerald_textures/ev00f3_purple_emerald_texlist.inc"
};
static NJS_TEXNAME ev00f3_sky_emerald_textures[1] = {
    {TEXT_3, 0, 0},
};
NJS_TEXLIST ev00f3_sky_emerald_texlist = {
#include "ev00f3/emerald_textures/ev00f3_sky_emerald_texlist.inc"
};
static NJS_TEXNAME ev00f3_white_emerald_textures[1] = {
    {TEXT_4, 0, 0},
};
NJS_TEXLIST ev00f3_white_emerald_texlist = {
#include "ev00f3/emerald_textures/ev00f3_white_emerald_texlist.inc"
};
