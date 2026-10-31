/* EV008C's action for event_eggmobile_object and its motion, then the Egg
 * Carrier's 21 texture names. their texlist, ev008c_egg_carrier_texlist,
 * starts the data after this file */
#include <ninja.h>

#include "ev008c/eggmobile_action.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 11
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 13
#include "ev008c/eggmobile_action/text.inc"

extern NJS_MDATA2 dat_0cb89d10[];
extern NJS_OBJECT event_eggmobile_object;

static NJS_MOTION ev008c_eggmobile_motion = {
#include "ev008c/eggmobile_action/ev008c_eggmobile_motion.inc"
};
NJS_ACTION ev008c_eggmobile_action = {
#include "ev008c/eggmobile_action/ev008c_eggmobile_action.inc"
};
NJS_TEXNAME ev008c_egg_carrier_textures[21] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0},
};
