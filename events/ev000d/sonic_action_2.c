/* the action Sonic repeats while he waits for Tails, and its motion. the
 * texture name table and the texlist of the Tornado's model follow. the
 * event loads the texlist, ev000d_tornado_texlist, from EV_TR1_WITH_SONIC.
 * the file ends on dat_0cb86db4, the model's last child, a node that draws
 * nothing */
#include <ninja.h>

#include "ev000d/sonic_action_2.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 12
#define TEXT_15_LENGTH 14
#define TEXT_16_LENGTH 14
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 12
#define TEXT_19_LENGTH 12
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 10
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 8
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 11
#define TEXT_33_LENGTH 9
#define TEXT_34_LENGTH 10
#define TEXT_35_LENGTH 9
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 10
#define TEXT_39_LENGTH 10
#define TEXT_40_LENGTH 10
#define TEXT_41_LENGTH 8
#define TEXT_42_LENGTH 12
#define TEXT_43_LENGTH 12
#define TEXT_44_LENGTH 12
#define TEXT_45_LENGTH 13
#define TEXT_46_LENGTH 13
#include "ev000d/sonic_action_2/text.inc"

extern NJS_MDATA2 dat_0cb866e0[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev000d_sonic_motion_2 = {
#include "ev000d/sonic_action_2/ev000d_sonic_motion_2.inc"
};
NJS_ACTION ev000d_sonic_action_2 = {
#include "ev000d/sonic_action_2/ev000d_sonic_action_2.inc"
};
static NJS_TEXNAME ev000d_tornado_textures[46] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0},
};
NJS_TEXLIST ev000d_tornado_texlist = {
#include "ev000d/sonic_action_2/ev000d_tornado_texlist.inc"
};
static NJS_OBJECT dat_0cb86db4 = {
#include "ev000d/sonic_action_2/dat_0cb86db4.inc"
};
