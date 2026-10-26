/* the tap's water: its object, and its textures' names and texlist.
 * nothing in STG07.BIN or 1ST_READ.BIN points at the texlist */
#include <ninja.h>

#include "stg07/tap_water.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#include "stg07/tap_water/text.inc"

extern NJS_MODEL tap_water_model;

NJS_OBJECT tap_water_object = {
#include "stg07/tap_water/tap_water_object.inc"
};
/* an object's data starts on a multiple of 8. each item, from here to the
 * material, follows the one before with no gap. each starts off a multiple of
 * 8, so none starts an object. this file holds them all */
static NJS_TEXNAME tap_water_textures[4] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
    {TEXT_4, 0, 0},
};
static NJS_TEXLIST tap_water_texlist = {
#include "stg07/tap_water/tap_water_texlist.inc"
};
static NJS_MATERIAL dat_0ca4dbf4[1] = {
#include "stg07/tap_water/dat_0ca4dbf4.inc"
};
