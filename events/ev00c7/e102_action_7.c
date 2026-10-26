/* this object holds, in order: E-102's ev00c7_e102_action_7; E-101 mkII's
 * texture list, model and three actions; the blue bird's texture list; a
 * model and its one action, which nothing reads; the gray bird's texture
 * list; and the pink bird's texture names. E-102's model lies in
 * 1ST_READ.BIN. the NJS_MDATA2 table of E-102's motion lies in the data
 * before this object. ev00c7_pink_bird_texlist, which names the pink
 * bird's textures, lies in the data after it */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00c7/e102_action_7.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 13
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 13
#define TEXT_22_LENGTH 14
#define TEXT_23_LENGTH 13
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 12
#define TEXT_26_LENGTH 7
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 7
#define TEXT_30_LENGTH 8
#define TEXT_31_LENGTH 8
#define TEXT_32_LENGTH 8
#define TEXT_33_LENGTH 9
#define TEXT_34_LENGTH 9
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 8
#define TEXT_37_LENGTH 8
#define TEXT_38_LENGTH 9
#define TEXT_39_LENGTH 9
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 7
#define TEXT_42_LENGTH 7
#define TEXT_43_LENGTH 7
#define TEXT_44_LENGTH 9
#define TEXT_45_LENGTH 9
#define TEXT_46_LENGTH 9
#include "ev00c7/e102_action_7/text.inc"

extern NJS_MDATA2 dat_0cbb5820[];
extern NJS_OBJECT e102_object;

static NJS_MOTION ev00c7_e102_motion_7 = {
#include "ev00c7/e102_action_7/ev00c7_e102_motion_7.inc"
};
NJS_ACTION ev00c7_e102_action_7 = {
#include "ev00c7/e102_action_7/ev00c7_e102_action_7.inc"
};
static NJS_TEXNAME ev00c7_e101_textures[32] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
};
NJS_TEXLIST ev00c7_e101_texlist = {
#include "ev00c7/e102_action_7/ev00c7_e101_texlist.inc"
};
static NJS_OBJECT dat_0cbb5e8c = {
#include "ev00c7/e102_action_7/dat_0cbb5e8c.inc"
};
static NJS_OBJECT dat_0cbb5ec0 = {
#include "ev00c7/e102_action_7/dat_0cbb5ec0.inc"
};
static NJS_OBJECT dat_0cbb5ef4 = {
#include "ev00c7/e102_action_7/dat_0cbb5ef4.inc"
};
static NJS_OBJECT dat_0cbb5f28 = {
#include "ev00c7/e102_action_7/dat_0cbb5f28.inc"
};
static NJS_OBJECT dat_0cbb5f5c = {
#include "ev00c7/e102_action_7/dat_0cbb5f5c.inc"
};
static NJS_OBJECT dat_0cbb5f90 = {
#include "ev00c7/e102_action_7/dat_0cbb5f90.inc"
};
static NJS_OBJECT dat_0cbb5fc4 = {
#include "ev00c7/e102_action_7/dat_0cbb5fc4.inc"
};
static NJS_OBJECT dat_0cbb5ff8 = {
#include "ev00c7/e102_action_7/dat_0cbb5ff8.inc"
};
static NJS_MATERIAL dat_0cbb602c[1] = {
#include "ev00c7/e102_action_7/dat_0cbb602c.inc"
};
static Sint16 dat_0cbb6040[12] = {
#include "ev00c7/e102_action_7/dat_0cbb6040.inc"
};
static NJS_COLOR dat_0cbb6058[10] = {
#include "ev00c7/e102_action_7/dat_0cbb6058.inc"
};
static NJS_MESHSET dat_0cbb6080[1] = {
#include "ev00c7/e102_action_7/dat_0cbb6080.inc"
};
static NJS_POINT3 dat_0cbb6098[8] = {
#include "ev00c7/e102_action_7/dat_0cbb6098.inc"
};
static NJS_VECTOR dat_0cbb60f8[8] = {
#include "ev00c7/e102_action_7/dat_0cbb60f8.inc"
};
static NJS_MODEL dat_0cbb6158 = {
#include "ev00c7/e102_action_7/dat_0cbb6158.inc"
};
static NJS_OBJECT dat_0cbb6180 = {
#include "ev00c7/e102_action_7/dat_0cbb6180.inc"
};
static NJS_MATERIAL dat_0cbb61b4[1] = {
#include "ev00c7/e102_action_7/dat_0cbb61b4.inc"
};
static Sint16 dat_0cbb61c8[28] = {
#include "ev00c7/e102_action_7/dat_0cbb61c8.inc"
};
static NJS_COLOR dat_0cbb6200[24] = {
#include "ev00c7/e102_action_7/dat_0cbb6200.inc"
};
static NJS_MESHSET dat_0cbb6260[1] = {
#include "ev00c7/e102_action_7/dat_0cbb6260.inc"
};
static NJS_POINT3 dat_0cbb6278[14] = {
#include "ev00c7/e102_action_7/dat_0cbb6278.inc"
};
static NJS_VECTOR dat_0cbb6320[14] = {
#include "ev00c7/e102_action_7/dat_0cbb6320.inc"
};
static NJS_MODEL dat_0cbb63c8 = {
#include "ev00c7/e102_action_7/dat_0cbb63c8.inc"
};
static NJS_OBJECT dat_0cbb63f0 = {
#include "ev00c7/e102_action_7/dat_0cbb63f0.inc"
};
static NJS_MATERIAL dat_0cbb6424[5] = {
#include "ev00c7/e102_action_7/dat_0cbb6424.inc"
};
static Sint16 dat_0cbb6488[79] = {
#include "ev00c7/e102_action_7/dat_0cbb6488.inc"
};
static Sint16 dat_0cbb6526[45] = {
#include "ev00c7/e102_action_7/dat_0cbb6526.inc"
};
static Sint16 dat_0cbb6580[70] = {
#include "ev00c7/e102_action_7/dat_0cbb6580.inc"
};
static Sint16 dat_0cbb660c[14] = {
#include "ev00c7/e102_action_7/dat_0cbb660c.inc"
};
static Sint16 dat_0cbb6628[20] = {
#include "ev00c7/e102_action_7/dat_0cbb6628.inc"
};
static NJS_COLOR dat_0cbb6650[68] = {
#include "ev00c7/e102_action_7/dat_0cbb6650.inc"
};
static NJS_COLOR dat_0cbb6760[38] = {
#include "ev00c7/e102_action_7/dat_0cbb6760.inc"
};
static NJS_COLOR dat_0cbb67f8[60] = {
#include "ev00c7/e102_action_7/dat_0cbb67f8.inc"
};
static NJS_COLOR dat_0cbb68e8[12] = {
#include "ev00c7/e102_action_7/dat_0cbb68e8.inc"
};
static NJS_COLOR dat_0cbb6918[16] = {
#include "ev00c7/e102_action_7/dat_0cbb6918.inc"
};
static NJS_MESHSET dat_0cbb6958[5] = {
#include "ev00c7/e102_action_7/dat_0cbb6958.inc"
};
static NJS_POINT3 dat_0cbb69d0[66] = {
#include "ev00c7/e102_action_7/dat_0cbb69d0.inc"
};
static NJS_VECTOR dat_0cbb6ce8[66] = {
#include "ev00c7/e102_action_7/dat_0cbb6ce8.inc"
};
static NJS_MODEL dat_0cbb7000 = {
#include "ev00c7/e102_action_7/dat_0cbb7000.inc"
};
static NJS_OBJECT dat_0cbb7028 = {
#include "ev00c7/e102_action_7/dat_0cbb7028.inc"
};
static NJS_MATERIAL dat_0cbb705c[2] = {
#include "ev00c7/e102_action_7/dat_0cbb705c.inc"
};
static Sint16 dat_0cbb7084[38] = {
#include "ev00c7/e102_action_7/dat_0cbb7084.inc"
};
static Sint16 dat_0cbb70d0[16] = {
#include "ev00c7/e102_action_7/dat_0cbb70d0.inc"
};
static NJS_COLOR dat_0cbb70f0[36] = {
#include "ev00c7/e102_action_7/dat_0cbb70f0.inc"
};
static NJS_COLOR dat_0cbb7180[12] = {
#include "ev00c7/e102_action_7/dat_0cbb7180.inc"
};
static NJS_MESHSET dat_0cbb71b0[2] = {
#include "ev00c7/e102_action_7/dat_0cbb71b0.inc"
};
static NJS_POINT3 dat_0cbb71e0[24] = {
#include "ev00c7/e102_action_7/dat_0cbb71e0.inc"
};
static NJS_VECTOR dat_0cbb7300[24] = {
#include "ev00c7/e102_action_7/dat_0cbb7300.inc"
};
static NJS_MODEL dat_0cbb7420 = {
#include "ev00c7/e102_action_7/dat_0cbb7420.inc"
};
static NJS_OBJECT dat_0cbb7448 = {
#include "ev00c7/e102_action_7/dat_0cbb7448.inc"
};
static NJS_MATERIAL dat_0cbb747c[2] = {
#include "ev00c7/e102_action_7/dat_0cbb747c.inc"
};
static Sint16 dat_0cbb74a4[20] = {
#include "ev00c7/e102_action_7/dat_0cbb74a4.inc"
};
static Sint16 dat_0cbb74cc[6] = {
#include "ev00c7/e102_action_7/dat_0cbb74cc.inc"
};
static NJS_COLOR dat_0cbb74d8[16] = {
#include "ev00c7/e102_action_7/dat_0cbb74d8.inc"
};
static NJS_COLOR dat_0cbb7518[4] = {
#include "ev00c7/e102_action_7/dat_0cbb7518.inc"
};
static NJS_MESHSET dat_0cbb7528[2] = {
#include "ev00c7/e102_action_7/dat_0cbb7528.inc"
};
static NJS_POINT3 dat_0cbb7558[8] = {
#include "ev00c7/e102_action_7/dat_0cbb7558.inc"
};
static NJS_VECTOR dat_0cbb75b8[8] = {
#include "ev00c7/e102_action_7/dat_0cbb75b8.inc"
};
static NJS_MODEL dat_0cbb7618 = {
#include "ev00c7/e102_action_7/dat_0cbb7618.inc"
};
static NJS_OBJECT dat_0cbb7640 = {
#include "ev00c7/e102_action_7/dat_0cbb7640.inc"
};
static NJS_MATERIAL dat_0cbb7674[3] = {
#include "ev00c7/e102_action_7/dat_0cbb7674.inc"
};
static Sint16 dat_0cbb76b0[10] = {
#include "ev00c7/e102_action_7/dat_0cbb76b0.inc"
};
static Sint16 dat_0cbb76c4[11] = {
#include "ev00c7/e102_action_7/dat_0cbb76c4.inc"
};
static Sint16 dat_0cbb76da[5] = {
#include "ev00c7/e102_action_7/dat_0cbb76da.inc"
};
static NJS_COLOR dat_0cbb76e4[8] = {
#include "ev00c7/e102_action_7/dat_0cbb76e4.inc"
};
static NJS_COLOR dat_0cbb7704[10] = {
#include "ev00c7/e102_action_7/dat_0cbb7704.inc"
};
static NJS_COLOR dat_0cbb772c[4] = {
#include "ev00c7/e102_action_7/dat_0cbb772c.inc"
};
static NJS_MESHSET dat_0cbb773c[3] = {
#include "ev00c7/e102_action_7/dat_0cbb773c.inc"
};
static NJS_POINT3 dat_0cbb7784[9] = {
#include "ev00c7/e102_action_7/dat_0cbb7784.inc"
};
static NJS_VECTOR dat_0cbb77f0[9] = {
#include "ev00c7/e102_action_7/dat_0cbb77f0.inc"
};
static NJS_MODEL dat_0cbb785c = {
#include "ev00c7/e102_action_7/dat_0cbb785c.inc"
};
static NJS_OBJECT dat_0cbb7884 = {
#include "ev00c7/e102_action_7/dat_0cbb7884.inc"
};
static NJS_OBJECT dat_0cbb78b8 = {
#include "ev00c7/e102_action_7/dat_0cbb78b8.inc"
};
static NJS_OBJECT dat_0cbb78ec = {
#include "ev00c7/e102_action_7/dat_0cbb78ec.inc"
};
static NJS_OBJECT dat_0cbb7920 = {
#include "ev00c7/e102_action_7/dat_0cbb7920.inc"
};
static NJS_OBJECT dat_0cbb7954 = {
#include "ev00c7/e102_action_7/dat_0cbb7954.inc"
};
static NJS_OBJECT dat_0cbb7988 = {
#include "ev00c7/e102_action_7/dat_0cbb7988.inc"
};
static NJS_MATERIAL dat_0cbb79bc[1] = {
#include "ev00c7/e102_action_7/dat_0cbb79bc.inc"
};
static Sint16 dat_0cbb79d0[12] = {
#include "ev00c7/e102_action_7/dat_0cbb79d0.inc"
};
static NJS_COLOR dat_0cbb79e8[10] = {
#include "ev00c7/e102_action_7/dat_0cbb79e8.inc"
};
static NJS_MESHSET dat_0cbb7a10[1] = {
#include "ev00c7/e102_action_7/dat_0cbb7a10.inc"
};
static NJS_POINT3 dat_0cbb7a28[8] = {
#include "ev00c7/e102_action_7/dat_0cbb7a28.inc"
};
static NJS_VECTOR dat_0cbb7a88[8] = {
#include "ev00c7/e102_action_7/dat_0cbb7a88.inc"
};
static NJS_MODEL dat_0cbb7ae8 = {
#include "ev00c7/e102_action_7/dat_0cbb7ae8.inc"
};
static NJS_OBJECT dat_0cbb7b10 = {
#include "ev00c7/e102_action_7/dat_0cbb7b10.inc"
};
static NJS_MATERIAL dat_0cbb7b44[1] = {
#include "ev00c7/e102_action_7/dat_0cbb7b44.inc"
};
static Sint16 dat_0cbb7b58[28] = {
#include "ev00c7/e102_action_7/dat_0cbb7b58.inc"
};
static NJS_COLOR dat_0cbb7b90[24] = {
#include "ev00c7/e102_action_7/dat_0cbb7b90.inc"
};
static NJS_MESHSET dat_0cbb7bf0[1] = {
#include "ev00c7/e102_action_7/dat_0cbb7bf0.inc"
};
static NJS_POINT3 dat_0cbb7c08[14] = {
#include "ev00c7/e102_action_7/dat_0cbb7c08.inc"
};
static NJS_VECTOR dat_0cbb7cb0[14] = {
#include "ev00c7/e102_action_7/dat_0cbb7cb0.inc"
};
static NJS_MODEL dat_0cbb7d58 = {
#include "ev00c7/e102_action_7/dat_0cbb7d58.inc"
};
static NJS_OBJECT dat_0cbb7d80 = {
#include "ev00c7/e102_action_7/dat_0cbb7d80.inc"
};
static NJS_MATERIAL dat_0cbb7db4[5] = {
#include "ev00c7/e102_action_7/dat_0cbb7db4.inc"
};
static Sint16 dat_0cbb7e18[76] = {
#include "ev00c7/e102_action_7/dat_0cbb7e18.inc"
};
static Sint16 dat_0cbb7eb0[48] = {
#include "ev00c7/e102_action_7/dat_0cbb7eb0.inc"
};
static Sint16 dat_0cbb7f10[70] = {
#include "ev00c7/e102_action_7/dat_0cbb7f10.inc"
};
static Sint16 dat_0cbb7f9c[14] = {
#include "ev00c7/e102_action_7/dat_0cbb7f9c.inc"
};
static Sint16 dat_0cbb7fb8[22] = {
#include "ev00c7/e102_action_7/dat_0cbb7fb8.inc"
};
static NJS_COLOR dat_0cbb7fe4[66] = {
#include "ev00c7/e102_action_7/dat_0cbb7fe4.inc"
};
static NJS_COLOR dat_0cbb80ec[40] = {
#include "ev00c7/e102_action_7/dat_0cbb80ec.inc"
};
static NJS_COLOR dat_0cbb818c[60] = {
#include "ev00c7/e102_action_7/dat_0cbb818c.inc"
};
static NJS_COLOR dat_0cbb827c[12] = {
#include "ev00c7/e102_action_7/dat_0cbb827c.inc"
};
static NJS_COLOR dat_0cbb82ac[18] = {
#include "ev00c7/e102_action_7/dat_0cbb82ac.inc"
};
static NJS_MESHSET dat_0cbb82f4[5] = {
#include "ev00c7/e102_action_7/dat_0cbb82f4.inc"
};
static NJS_POINT3 dat_0cbb836c[66] = {
#include "ev00c7/e102_action_7/dat_0cbb836c.inc"
};
static NJS_VECTOR dat_0cbb8684[66] = {
#include "ev00c7/e102_action_7/dat_0cbb8684.inc"
};
static NJS_MODEL dat_0cbb899c = {
#include "ev00c7/e102_action_7/dat_0cbb899c.inc"
};
static NJS_OBJECT dat_0cbb89c4 = {
#include "ev00c7/e102_action_7/dat_0cbb89c4.inc"
};
static NJS_MATERIAL dat_0cbb89f8[2] = {
#include "ev00c7/e102_action_7/dat_0cbb89f8.inc"
};
static Sint16 dat_0cbb8a20[20] = {
#include "ev00c7/e102_action_7/dat_0cbb8a20.inc"
};
static Sint16 dat_0cbb8a48[6] = {
#include "ev00c7/e102_action_7/dat_0cbb8a48.inc"
};
static NJS_COLOR dat_0cbb8a54[16] = {
#include "ev00c7/e102_action_7/dat_0cbb8a54.inc"
};
static NJS_COLOR dat_0cbb8a94[4] = {
#include "ev00c7/e102_action_7/dat_0cbb8a94.inc"
};
static NJS_MESHSET dat_0cbb8aa4[2] = {
#include "ev00c7/e102_action_7/dat_0cbb8aa4.inc"
};
static NJS_POINT3 dat_0cbb8ad4[8] = {
#include "ev00c7/e102_action_7/dat_0cbb8ad4.inc"
};
static NJS_VECTOR dat_0cbb8b34[8] = {
#include "ev00c7/e102_action_7/dat_0cbb8b34.inc"
};
static NJS_MODEL dat_0cbb8b94 = {
#include "ev00c7/e102_action_7/dat_0cbb8b94.inc"
};
static NJS_OBJECT dat_0cbb8bbc = {
#include "ev00c7/e102_action_7/dat_0cbb8bbc.inc"
};
static NJS_MATERIAL dat_0cbb8bf0[2] = {
#include "ev00c7/e102_action_7/dat_0cbb8bf0.inc"
};
static Sint16 dat_0cbb8c18[38] = {
#include "ev00c7/e102_action_7/dat_0cbb8c18.inc"
};
static Sint16 dat_0cbb8c64[16] = {
#include "ev00c7/e102_action_7/dat_0cbb8c64.inc"
};
static NJS_COLOR dat_0cbb8c84[36] = {
#include "ev00c7/e102_action_7/dat_0cbb8c84.inc"
};
static NJS_COLOR dat_0cbb8d14[12] = {
#include "ev00c7/e102_action_7/dat_0cbb8d14.inc"
};
static NJS_MESHSET dat_0cbb8d44[2] = {
#include "ev00c7/e102_action_7/dat_0cbb8d44.inc"
};
static NJS_POINT3 dat_0cbb8d74[24] = {
#include "ev00c7/e102_action_7/dat_0cbb8d74.inc"
};
static NJS_VECTOR dat_0cbb8e94[24] = {
#include "ev00c7/e102_action_7/dat_0cbb8e94.inc"
};
static NJS_MODEL dat_0cbb8fb4 = {
#include "ev00c7/e102_action_7/dat_0cbb8fb4.inc"
};
static NJS_OBJECT dat_0cbb8fdc = {
#include "ev00c7/e102_action_7/dat_0cbb8fdc.inc"
};
static NJS_MATERIAL dat_0cbb9010[3] = {
#include "ev00c7/e102_action_7/dat_0cbb9010.inc"
};
static Sint16 dat_0cbb904c[10] = {
#include "ev00c7/e102_action_7/dat_0cbb904c.inc"
};
static Sint16 dat_0cbb9060[11] = {
#include "ev00c7/e102_action_7/dat_0cbb9060.inc"
};
static Sint16 dat_0cbb9076[5] = {
#include "ev00c7/e102_action_7/dat_0cbb9076.inc"
};
static NJS_COLOR dat_0cbb9080[8] = {
#include "ev00c7/e102_action_7/dat_0cbb9080.inc"
};
static NJS_COLOR dat_0cbb90a0[10] = {
#include "ev00c7/e102_action_7/dat_0cbb90a0.inc"
};
static NJS_COLOR dat_0cbb90c8[4] = {
#include "ev00c7/e102_action_7/dat_0cbb90c8.inc"
};
static NJS_MESHSET dat_0cbb90d8[3] = {
#include "ev00c7/e102_action_7/dat_0cbb90d8.inc"
};
static NJS_POINT3 dat_0cbb9120[9] = {
#include "ev00c7/e102_action_7/dat_0cbb9120.inc"
};
static NJS_VECTOR dat_0cbb918c[9] = {
#include "ev00c7/e102_action_7/dat_0cbb918c.inc"
};
static NJS_MODEL dat_0cbb91f8 = {
#include "ev00c7/e102_action_7/dat_0cbb91f8.inc"
};
static NJS_OBJECT dat_0cbb9220 = {
#include "ev00c7/e102_action_7/dat_0cbb9220.inc"
};
static NJS_OBJECT dat_0cbb9254 = {
#include "ev00c7/e102_action_7/dat_0cbb9254.inc"
};
static NJS_OBJECT dat_0cbb9288 = {
#include "ev00c7/e102_action_7/dat_0cbb9288.inc"
};
static NJS_OBJECT dat_0cbb92bc = {
#include "ev00c7/e102_action_7/dat_0cbb92bc.inc"
};
static NJS_OBJECT dat_0cbb92f0 = {
#include "ev00c7/e102_action_7/dat_0cbb92f0.inc"
};
static NJS_OBJECT dat_0cbb9324 = {
#include "ev00c7/e102_action_7/dat_0cbb9324.inc"
};
static NJS_MATERIAL dat_0cbb9358[2] = {
#include "ev00c7/e102_action_7/dat_0cbb9358.inc"
};
static Sint16 dat_0cbb9380[30] = {
#include "ev00c7/e102_action_7/dat_0cbb9380.inc"
};
static Sint16 dat_0cbb93bc[14] = {
#include "ev00c7/e102_action_7/dat_0cbb93bc.inc"
};
static NJS_COLOR dat_0cbb93d8[26] = {
#include "ev00c7/e102_action_7/dat_0cbb93d8.inc"
};
static NJS_COLOR dat_0cbb9440[12] = {
#include "ev00c7/e102_action_7/dat_0cbb9440.inc"
};
static NJS_MESHSET dat_0cbb9470[2] = {
#include "ev00c7/e102_action_7/dat_0cbb9470.inc"
};
static NJS_POINT3 dat_0cbb94a0[16] = {
#include "ev00c7/e102_action_7/dat_0cbb94a0.inc"
};
static NJS_VECTOR dat_0cbb9560[16] = {
#include "ev00c7/e102_action_7/dat_0cbb9560.inc"
};
static NJS_MODEL dat_0cbb9620 = {
#include "ev00c7/e102_action_7/dat_0cbb9620.inc"
};
static NJS_OBJECT dat_0cbb9648 = {
#include "ev00c7/e102_action_7/dat_0cbb9648.inc"
};
static NJS_MATERIAL dat_0cbb967c[2] = {
#include "ev00c7/e102_action_7/dat_0cbb967c.inc"
};
static Sint16 dat_0cbb96a4[19] = {
#include "ev00c7/e102_action_7/dat_0cbb96a4.inc"
};
static Sint16 dat_0cbb96ca[13] = {
#include "ev00c7/e102_action_7/dat_0cbb96ca.inc"
};
static NJS_COLOR dat_0cbb96e4[18] = {
#include "ev00c7/e102_action_7/dat_0cbb96e4.inc"
};
static NJS_COLOR dat_0cbb972c[10] = {
#include "ev00c7/e102_action_7/dat_0cbb972c.inc"
};
static NJS_MESHSET dat_0cbb9754[2] = {
#include "ev00c7/e102_action_7/dat_0cbb9754.inc"
};
static NJS_POINT3 dat_0cbb9784[16] = {
#include "ev00c7/e102_action_7/dat_0cbb9784.inc"
};
static NJS_VECTOR dat_0cbb9844[16] = {
#include "ev00c7/e102_action_7/dat_0cbb9844.inc"
};
static NJS_MODEL dat_0cbb9904 = {
#include "ev00c7/e102_action_7/dat_0cbb9904.inc"
};
static NJS_OBJECT dat_0cbb992c = {
#include "ev00c7/e102_action_7/dat_0cbb992c.inc"
};
static NJS_MATERIAL dat_0cbb9960[2] = {
#include "ev00c7/e102_action_7/dat_0cbb9960.inc"
};
static Sint16 dat_0cbb9988[19] = {
#include "ev00c7/e102_action_7/dat_0cbb9988.inc"
};
static Sint16 dat_0cbb99ae[15] = {
#include "ev00c7/e102_action_7/dat_0cbb99ae.inc"
};
static NJS_COLOR dat_0cbb99cc[18] = {
#include "ev00c7/e102_action_7/dat_0cbb99cc.inc"
};
static NJS_COLOR dat_0cbb9a14[12] = {
#include "ev00c7/e102_action_7/dat_0cbb9a14.inc"
};
static NJS_MESHSET dat_0cbb9a44[2] = {
#include "ev00c7/e102_action_7/dat_0cbb9a44.inc"
};
static NJS_POINT3 dat_0cbb9a74[16] = {
#include "ev00c7/e102_action_7/dat_0cbb9a74.inc"
};
static NJS_VECTOR dat_0cbb9b34[16] = {
#include "ev00c7/e102_action_7/dat_0cbb9b34.inc"
};
static NJS_MODEL dat_0cbb9bf4 = {
#include "ev00c7/e102_action_7/dat_0cbb9bf4.inc"
};
static NJS_OBJECT dat_0cbb9c1c = {
#include "ev00c7/e102_action_7/dat_0cbb9c1c.inc"
};
static NJS_MATERIAL dat_0cbb9c50[2] = {
#include "ev00c7/e102_action_7/dat_0cbb9c50.inc"
};
static Sint16 dat_0cbb9c78[12] = {
#include "ev00c7/e102_action_7/dat_0cbb9c78.inc"
};
static Sint16 dat_0cbb9c90[14] = {
#include "ev00c7/e102_action_7/dat_0cbb9c90.inc"
};
static NJS_COLOR dat_0cbb9cac[10] = {
#include "ev00c7/e102_action_7/dat_0cbb9cac.inc"
};
static NJS_COLOR dat_0cbb9cd4[12] = {
#include "ev00c7/e102_action_7/dat_0cbb9cd4.inc"
};
static NJS_MESHSET dat_0cbb9d04[2] = {
#include "ev00c7/e102_action_7/dat_0cbb9d04.inc"
};
static NJS_POINT3 dat_0cbb9d34[10] = {
#include "ev00c7/e102_action_7/dat_0cbb9d34.inc"
};
static NJS_VECTOR dat_0cbb9dac[10] = {
#include "ev00c7/e102_action_7/dat_0cbb9dac.inc"
};
static NJS_MODEL dat_0cbb9e24 = {
#include "ev00c7/e102_action_7/dat_0cbb9e24.inc"
};
static NJS_OBJECT dat_0cbb9e4c = {
#include "ev00c7/e102_action_7/dat_0cbb9e4c.inc"
};
static NJS_MATERIAL dat_0cbb9e80[1] = {
#include "ev00c7/e102_action_7/dat_0cbb9e80.inc"
};
static Sint16 dat_0cbb9e94[154] = {
#include "ev00c7/e102_action_7/dat_0cbb9e94.inc"
};
static NJS_COLOR dat_0cbb9fc8[134] = {
#include "ev00c7/e102_action_7/dat_0cbb9fc8.inc"
};
static NJS_MESHSET dat_0cbba1e0[1] = {
#include "ev00c7/e102_action_7/dat_0cbba1e0.inc"
};
static NJS_POINT3 dat_0cbba1f8[49] = {
#include "ev00c7/e102_action_7/dat_0cbba1f8.inc"
};
static NJS_VECTOR dat_0cbba444[49] = {
#include "ev00c7/e102_action_7/dat_0cbba444.inc"
};
static NJS_MODEL dat_0cbba690 = {
#include "ev00c7/e102_action_7/dat_0cbba690.inc"
};
static NJS_OBJECT dat_0cbba6b8 = {
#include "ev00c7/e102_action_7/dat_0cbba6b8.inc"
};
static NJS_MATERIAL dat_0cbba6ec[1] = {
#include "ev00c7/e102_action_7/dat_0cbba6ec.inc"
};
static Sint16 dat_0cbba700[22] = {
#include "ev00c7/e102_action_7/dat_0cbba700.inc"
};
static NJS_COLOR dat_0cbba72c[18] = {
#include "ev00c7/e102_action_7/dat_0cbba72c.inc"
};
static NJS_MESHSET dat_0cbba774[1] = {
#include "ev00c7/e102_action_7/dat_0cbba774.inc"
};
static NJS_POINT3 dat_0cbba78c[8] = {
#include "ev00c7/e102_action_7/dat_0cbba78c.inc"
};
static NJS_VECTOR dat_0cbba7ec[8] = {
#include "ev00c7/e102_action_7/dat_0cbba7ec.inc"
};
static NJS_MODEL dat_0cbba84c = {
#include "ev00c7/e102_action_7/dat_0cbba84c.inc"
};
static NJS_OBJECT dat_0cbba874 = {
#include "ev00c7/e102_action_7/dat_0cbba874.inc"
};
static NJS_MATERIAL dat_0cbba8a8[2] = {
#include "ev00c7/e102_action_7/dat_0cbba8a8.inc"
};
static Sint16 dat_0cbba8d0[11] = {
#include "ev00c7/e102_action_7/dat_0cbba8d0.inc"
};
static Sint16 dat_0cbba8e6[5] = {
#include "ev00c7/e102_action_7/dat_0cbba8e6.inc"
};
static NJS_COLOR dat_0cbba8f0[10] = {
#include "ev00c7/e102_action_7/dat_0cbba8f0.inc"
};
static NJS_COLOR dat_0cbba918[4] = {
#include "ev00c7/e102_action_7/dat_0cbba918.inc"
};
static NJS_MESHSET dat_0cbba928[2] = {
#include "ev00c7/e102_action_7/dat_0cbba928.inc"
};
static NJS_POINT3 dat_0cbba958[8] = {
#include "ev00c7/e102_action_7/dat_0cbba958.inc"
};
static NJS_VECTOR dat_0cbba9b8[8] = {
#include "ev00c7/e102_action_7/dat_0cbba9b8.inc"
};
static NJS_MODEL dat_0cbbaa18 = {
#include "ev00c7/e102_action_7/dat_0cbbaa18.inc"
};
static NJS_OBJECT dat_0cbbaa40 = {
#include "ev00c7/e102_action_7/dat_0cbbaa40.inc"
};
static NJS_MATERIAL dat_0cbbaa74[2] = {
#include "ev00c7/e102_action_7/dat_0cbbaa74.inc"
};
static Sint16 dat_0cbbaa9c[10] = {
#include "ev00c7/e102_action_7/dat_0cbbaa9c.inc"
};
static Sint16 dat_0cbbaab0[14] = {
#include "ev00c7/e102_action_7/dat_0cbbaab0.inc"
};
static NJS_COLOR dat_0cbbaacc[8] = {
#include "ev00c7/e102_action_7/dat_0cbbaacc.inc"
};
static NJS_COLOR dat_0cbbaaec[12] = {
#include "ev00c7/e102_action_7/dat_0cbbaaec.inc"
};
static NJS_MESHSET dat_0cbbab1c[2] = {
#include "ev00c7/e102_action_7/dat_0cbbab1c.inc"
};
static NJS_POINT3 dat_0cbbab4c[8] = {
#include "ev00c7/e102_action_7/dat_0cbbab4c.inc"
};
static NJS_VECTOR dat_0cbbabac[8] = {
#include "ev00c7/e102_action_7/dat_0cbbabac.inc"
};
static NJS_MODEL dat_0cbbac0c = {
#include "ev00c7/e102_action_7/dat_0cbbac0c.inc"
};
static NJS_OBJECT dat_0cbbac34 = {
#include "ev00c7/e102_action_7/dat_0cbbac34.inc"
};
static NJS_MATERIAL dat_0cbbac68[1] = {
#include "ev00c7/e102_action_7/dat_0cbbac68.inc"
};
static Sint16 dat_0cbbac7c[12] = {
#include "ev00c7/e102_action_7/dat_0cbbac7c.inc"
};
static NJS_COLOR dat_0cbbac94[10] = {
#include "ev00c7/e102_action_7/dat_0cbbac94.inc"
};
static NJS_MESHSET dat_0cbbacbc[1] = {
#include "ev00c7/e102_action_7/dat_0cbbacbc.inc"
};
static NJS_POINT3 dat_0cbbacd4[8] = {
#include "ev00c7/e102_action_7/dat_0cbbacd4.inc"
};
static NJS_VECTOR dat_0cbbad34[8] = {
#include "ev00c7/e102_action_7/dat_0cbbad34.inc"
};
static NJS_MODEL dat_0cbbad94 = {
#include "ev00c7/e102_action_7/dat_0cbbad94.inc"
};
static NJS_OBJECT dat_0cbbadbc = {
#include "ev00c7/e102_action_7/dat_0cbbadbc.inc"
};
static NJS_MATERIAL dat_0cbbadf0[1] = {
#include "ev00c7/e102_action_7/dat_0cbbadf0.inc"
};
static Sint16 dat_0cbbae04[22] = {
#include "ev00c7/e102_action_7/dat_0cbbae04.inc"
};
static NJS_COLOR dat_0cbbae30[18] = {
#include "ev00c7/e102_action_7/dat_0cbbae30.inc"
};
static NJS_MESHSET dat_0cbbae78[1] = {
#include "ev00c7/e102_action_7/dat_0cbbae78.inc"
};
static NJS_POINT3 dat_0cbbae90[8] = {
#include "ev00c7/e102_action_7/dat_0cbbae90.inc"
};
static NJS_VECTOR dat_0cbbaef0[8] = {
#include "ev00c7/e102_action_7/dat_0cbbaef0.inc"
};
static NJS_MODEL dat_0cbbaf50 = {
#include "ev00c7/e102_action_7/dat_0cbbaf50.inc"
};
static NJS_OBJECT dat_0cbbaf78 = {
#include "ev00c7/e102_action_7/dat_0cbbaf78.inc"
};
static NJS_MATERIAL dat_0cbbafac[2] = {
#include "ev00c7/e102_action_7/dat_0cbbafac.inc"
};
static Sint16 dat_0cbbafd4[14] = {
#include "ev00c7/e102_action_7/dat_0cbbafd4.inc"
};
static Sint16 dat_0cbbaff0[6] = {
#include "ev00c7/e102_action_7/dat_0cbbaff0.inc"
};
static NJS_COLOR dat_0cbbaffc[12] = {
#include "ev00c7/e102_action_7/dat_0cbbaffc.inc"
};
static NJS_COLOR dat_0cbbb02c[4] = {
#include "ev00c7/e102_action_7/dat_0cbbb02c.inc"
};
static NJS_MESHSET dat_0cbbb03c[2] = {
#include "ev00c7/e102_action_7/dat_0cbbb03c.inc"
};
static NJS_POINT3 dat_0cbbb06c[8] = {
#include "ev00c7/e102_action_7/dat_0cbbb06c.inc"
};
static NJS_VECTOR dat_0cbbb0cc[8] = {
#include "ev00c7/e102_action_7/dat_0cbbb0cc.inc"
};
static NJS_MODEL dat_0cbbb12c = {
#include "ev00c7/e102_action_7/dat_0cbbb12c.inc"
};
static NJS_OBJECT dat_0cbbb154 = {
#include "ev00c7/e102_action_7/dat_0cbbb154.inc"
};
static NJS_MATERIAL dat_0cbbb188[2] = {
#include "ev00c7/e102_action_7/dat_0cbbb188.inc"
};
static Sint16 dat_0cbbb1b0[10] = {
#include "ev00c7/e102_action_7/dat_0cbbb1b0.inc"
};
static Sint16 dat_0cbbb1c4[14] = {
#include "ev00c7/e102_action_7/dat_0cbbb1c4.inc"
};
static NJS_COLOR dat_0cbbb1e0[8] = {
#include "ev00c7/e102_action_7/dat_0cbbb1e0.inc"
};
static NJS_COLOR dat_0cbbb200[12] = {
#include "ev00c7/e102_action_7/dat_0cbbb200.inc"
};
static NJS_MESHSET dat_0cbbb230[2] = {
#include "ev00c7/e102_action_7/dat_0cbbb230.inc"
};
static NJS_POINT3 dat_0cbbb260[8] = {
#include "ev00c7/e102_action_7/dat_0cbbb260.inc"
};
static NJS_VECTOR dat_0cbbb2c0[8] = {
#include "ev00c7/e102_action_7/dat_0cbbb2c0.inc"
};
static NJS_MODEL dat_0cbbb320 = {
#include "ev00c7/e102_action_7/dat_0cbbb320.inc"
};
static NJS_OBJECT dat_0cbbb348 = {
#include "ev00c7/e102_action_7/dat_0cbbb348.inc"
};
static NJS_MATERIAL dat_0cbbb37c[1] = {
#include "ev00c7/e102_action_7/dat_0cbbb37c.inc"
};
static Sint16 dat_0cbbb390[12] = {
#include "ev00c7/e102_action_7/dat_0cbbb390.inc"
};
static NJS_COLOR dat_0cbbb3a8[10] = {
#include "ev00c7/e102_action_7/dat_0cbbb3a8.inc"
};
static NJS_MESHSET dat_0cbbb3d0[1] = {
#include "ev00c7/e102_action_7/dat_0cbbb3d0.inc"
};
static NJS_POINT3 dat_0cbbb3e8[8] = {
#include "ev00c7/e102_action_7/dat_0cbbb3e8.inc"
};
static NJS_VECTOR dat_0cbbb448[8] = {
#include "ev00c7/e102_action_7/dat_0cbbb448.inc"
};
static NJS_MODEL dat_0cbbb4a8 = {
#include "ev00c7/e102_action_7/dat_0cbbb4a8.inc"
};
static NJS_OBJECT dat_0cbbb4d0 = {
#include "ev00c7/e102_action_7/dat_0cbbb4d0.inc"
};
static NJS_MATERIAL dat_0cbbb504[2] = {
#include "ev00c7/e102_action_7/dat_0cbbb504.inc"
};
static Sint16 dat_0cbbb52c[7] = {
#include "ev00c7/e102_action_7/dat_0cbbb52c.inc"
};
static Sint16 dat_0cbbb53a[15] = {
#include "ev00c7/e102_action_7/dat_0cbbb53a.inc"
};
static NJS_COLOR dat_0cbbb558[6] = {
#include "ev00c7/e102_action_7/dat_0cbbb558.inc"
};
static NJS_COLOR dat_0cbbb570[12] = {
#include "ev00c7/e102_action_7/dat_0cbbb570.inc"
};
static NJS_MESHSET dat_0cbbb5a0[2] = {
#include "ev00c7/e102_action_7/dat_0cbbb5a0.inc"
};
static NJS_POINT3 dat_0cbbb5d0[8] = {
#include "ev00c7/e102_action_7/dat_0cbbb5d0.inc"
};
static NJS_VECTOR dat_0cbbb630[8] = {
#include "ev00c7/e102_action_7/dat_0cbbb630.inc"
};
static NJS_MODEL dat_0cbbb690 = {
#include "ev00c7/e102_action_7/dat_0cbbb690.inc"
};
static NJS_OBJECT dat_0cbbb6b8 = {
#include "ev00c7/e102_action_7/dat_0cbbb6b8.inc"
};
static NJS_MATERIAL dat_0cbbb6ec[1] = {
#include "ev00c7/e102_action_7/dat_0cbbb6ec.inc"
};
static Sint16 dat_0cbbb700[86] = {
#include "ev00c7/e102_action_7/dat_0cbbb700.inc"
};
static NJS_COLOR dat_0cbbb7ac[76] = {
#include "ev00c7/e102_action_7/dat_0cbbb7ac.inc"
};
static NJS_MESHSET dat_0cbbb8dc[1] = {
#include "ev00c7/e102_action_7/dat_0cbbb8dc.inc"
};
static NJS_POINT3 dat_0cbbb8f4[32] = {
#include "ev00c7/e102_action_7/dat_0cbbb8f4.inc"
};
static NJS_VECTOR dat_0cbbba74[32] = {
#include "ev00c7/e102_action_7/dat_0cbbba74.inc"
};
static NJS_MODEL dat_0cbbbbf4 = {
#include "ev00c7/e102_action_7/dat_0cbbbbf4.inc"
};
static NJS_OBJECT dat_0cbbbc1c = {
#include "ev00c7/e102_action_7/dat_0cbbbc1c.inc"
};
static NJS_MATERIAL dat_0cbbbc50[2] = {
#include "ev00c7/e102_action_7/dat_0cbbbc50.inc"
};
static Sint16 dat_0cbbbc78[25] = {
#include "ev00c7/e102_action_7/dat_0cbbbc78.inc"
};
static Sint16 dat_0cbbbcaa[13] = {
#include "ev00c7/e102_action_7/dat_0cbbbcaa.inc"
};
static NJS_COLOR dat_0cbbbcc4[20] = {
#include "ev00c7/e102_action_7/dat_0cbbbcc4.inc"
};
static NJS_COLOR dat_0cbbbd14[10] = {
#include "ev00c7/e102_action_7/dat_0cbbbd14.inc"
};
static NJS_MESHSET dat_0cbbbd3c[2] = {
#include "ev00c7/e102_action_7/dat_0cbbbd3c.inc"
};
static NJS_POINT3 dat_0cbbbd6c[10] = {
#include "ev00c7/e102_action_7/dat_0cbbbd6c.inc"
};
static NJS_VECTOR dat_0cbbbde4[10] = {
#include "ev00c7/e102_action_7/dat_0cbbbde4.inc"
};
static NJS_MODEL dat_0cbbbe5c = {
#include "ev00c7/e102_action_7/dat_0cbbbe5c.inc"
};
static NJS_OBJECT dat_0cbbbe84 = {
#include "ev00c7/e102_action_7/dat_0cbbbe84.inc"
};
static NJS_MATERIAL dat_0cbbbeb8[2] = {
#include "ev00c7/e102_action_7/dat_0cbbbeb8.inc"
};
static Sint16 dat_0cbbbee0[24] = {
#include "ev00c7/e102_action_7/dat_0cbbbee0.inc"
};
static Sint16 dat_0cbbbf10[16] = {
#include "ev00c7/e102_action_7/dat_0cbbbf10.inc"
};
static NJS_COLOR dat_0cbbbf30[22] = {
#include "ev00c7/e102_action_7/dat_0cbbbf30.inc"
};
static NJS_COLOR dat_0cbbbf88[13] = {
#include "ev00c7/e102_action_7/dat_0cbbbf88.inc"
};
static NJS_MESHSET dat_0cbbbfbc[2] = {
#include "ev00c7/e102_action_7/dat_0cbbbfbc.inc"
};
static NJS_POINT3 dat_0cbbbfec[18] = {
#include "ev00c7/e102_action_7/dat_0cbbbfec.inc"
};
static NJS_VECTOR dat_0cbbc0c4[18] = {
#include "ev00c7/e102_action_7/dat_0cbbc0c4.inc"
};
static NJS_MODEL dat_0cbbc19c = {
#include "ev00c7/e102_action_7/dat_0cbbc19c.inc"
};
static NJS_OBJECT dat_0cbbc1c4 = {
#include "ev00c7/e102_action_7/dat_0cbbc1c4.inc"
};
static NJS_OBJECT dat_0cbbc1f8 = {
#include "ev00c7/e102_action_7/dat_0cbbc1f8.inc"
};
static NJS_MATERIAL dat_0cbbc22c[1] = {
#include "ev00c7/e102_action_7/dat_0cbbc22c.inc"
};
static Sint16 dat_0cbbc240[8] = {
#include "ev00c7/e102_action_7/dat_0cbbc240.inc"
};
static NJS_COLOR dat_0cbbc250[6] = {
#include "ev00c7/e102_action_7/dat_0cbbc250.inc"
};
static NJS_MESHSET dat_0cbbc268[1] = {
#include "ev00c7/e102_action_7/dat_0cbbc268.inc"
};
static NJS_POINT3 dat_0cbbc280[4] = {
#include "ev00c7/e102_action_7/dat_0cbbc280.inc"
};
static NJS_VECTOR dat_0cbbc2b0[4] = {
#include "ev00c7/e102_action_7/dat_0cbbc2b0.inc"
};
static NJS_MODEL dat_0cbbc2e0 = {
#include "ev00c7/e102_action_7/dat_0cbbc2e0.inc"
};
static NJS_OBJECT dat_0cbbc308 = {
#include "ev00c7/e102_action_7/dat_0cbbc308.inc"
};
static NJS_MATERIAL dat_0cbbc33c[2] = {
#include "ev00c7/e102_action_7/dat_0cbbc33c.inc"
};
static Sint16 dat_0cbbc364[25] = {
#include "ev00c7/e102_action_7/dat_0cbbc364.inc"
};
static Sint16 dat_0cbbc396[11] = {
#include "ev00c7/e102_action_7/dat_0cbbc396.inc"
};
static NJS_COLOR dat_0cbbc3ac[22] = {
#include "ev00c7/e102_action_7/dat_0cbbc3ac.inc"
};
static NJS_COLOR dat_0cbbc404[8] = {
#include "ev00c7/e102_action_7/dat_0cbbc404.inc"
};
static NJS_MESHSET dat_0cbbc424[2] = {
#include "ev00c7/e102_action_7/dat_0cbbc424.inc"
};
static NJS_POINT3 dat_0cbbc454[12] = {
#include "ev00c7/e102_action_7/dat_0cbbc454.inc"
};
static NJS_VECTOR dat_0cbbc4e4[12] = {
#include "ev00c7/e102_action_7/dat_0cbbc4e4.inc"
};
static NJS_MODEL dat_0cbbc574 = {
#include "ev00c7/e102_action_7/dat_0cbbc574.inc"
};
static NJS_OBJECT dat_0cbbc59c = {
#include "ev00c7/e102_action_7/dat_0cbbc59c.inc"
};
static NJS_MATERIAL dat_0cbbc5d0[2] = {
#include "ev00c7/e102_action_7/dat_0cbbc5d0.inc"
};
static Sint16 dat_0cbbc5f8[15] = {
#include "ev00c7/e102_action_7/dat_0cbbc5f8.inc"
};
static Sint16 dat_0cbbc616[7] = {
#include "ev00c7/e102_action_7/dat_0cbbc616.inc"
};
static NJS_COLOR dat_0cbbc624[14] = {
#include "ev00c7/e102_action_7/dat_0cbbc624.inc"
};
static NJS_COLOR dat_0cbbc65c[6] = {
#include "ev00c7/e102_action_7/dat_0cbbc65c.inc"
};
static NJS_MESHSET dat_0cbbc674[2] = {
#include "ev00c7/e102_action_7/dat_0cbbc674.inc"
};
static NJS_POINT3 dat_0cbbc6a4[12] = {
#include "ev00c7/e102_action_7/dat_0cbbc6a4.inc"
};
static NJS_VECTOR dat_0cbbc734[12] = {
#include "ev00c7/e102_action_7/dat_0cbbc734.inc"
};
static NJS_MODEL dat_0cbbc7c4 = {
#include "ev00c7/e102_action_7/dat_0cbbc7c4.inc"
};
static NJS_OBJECT dat_0cbbc7ec = {
#include "ev00c7/e102_action_7/dat_0cbbc7ec.inc"
};
static NJS_MATERIAL dat_0cbbc820[2] = {
#include "ev00c7/e102_action_7/dat_0cbbc820.inc"
};
static Sint16 dat_0cbbc848[15] = {
#include "ev00c7/e102_action_7/dat_0cbbc848.inc"
};
static Sint16 dat_0cbbc866[11] = {
#include "ev00c7/e102_action_7/dat_0cbbc866.inc"
};
static NJS_COLOR dat_0cbbc87c[14] = {
#include "ev00c7/e102_action_7/dat_0cbbc87c.inc"
};
static NJS_COLOR dat_0cbbc8b4[8] = {
#include "ev00c7/e102_action_7/dat_0cbbc8b4.inc"
};
static NJS_MESHSET dat_0cbbc8d4[2] = {
#include "ev00c7/e102_action_7/dat_0cbbc8d4.inc"
};
static NJS_POINT3 dat_0cbbc904[12] = {
#include "ev00c7/e102_action_7/dat_0cbbc904.inc"
};
static NJS_VECTOR dat_0cbbc994[12] = {
#include "ev00c7/e102_action_7/dat_0cbbc994.inc"
};
static NJS_MODEL dat_0cbbca24 = {
#include "ev00c7/e102_action_7/dat_0cbbca24.inc"
};
static NJS_OBJECT dat_0cbbca4c = {
#include "ev00c7/e102_action_7/dat_0cbbca4c.inc"
};
static NJS_MATERIAL dat_0cbbca80[6] = {
#include "ev00c7/e102_action_7/dat_0cbbca80.inc"
};
static Sint16 dat_0cbbcaf8[81] = {
#include "ev00c7/e102_action_7/dat_0cbbcaf8.inc"
};
static Sint16 dat_0cbbcb9a[17] = {
#include "ev00c7/e102_action_7/dat_0cbbcb9a.inc"
};
static Sint16 dat_0cbbcbbc[16] = {
#include "ev00c7/e102_action_7/dat_0cbbcbbc.inc"
};
static Sint16 dat_0cbbcbdc[40] = {
#include "ev00c7/e102_action_7/dat_0cbbcbdc.inc"
};
static Sint16 dat_0cbbcc2c[11] = {
#include "ev00c7/e102_action_7/dat_0cbbcc2c.inc"
};
static Sint16 dat_0cbbcc42[11] = {
#include "ev00c7/e102_action_7/dat_0cbbcc42.inc"
};
static NJS_COLOR dat_0cbbcc58[70] = {
#include "ev00c7/e102_action_7/dat_0cbbcc58.inc"
};
static NJS_COLOR dat_0cbbcd70[14] = {
#include "ev00c7/e102_action_7/dat_0cbbcd70.inc"
};
static NJS_COLOR dat_0cbbcda8[14] = {
#include "ev00c7/e102_action_7/dat_0cbbcda8.inc"
};
static NJS_COLOR dat_0cbbcde0[35] = {
#include "ev00c7/e102_action_7/dat_0cbbcde0.inc"
};
static NJS_COLOR dat_0cbbce6c[9] = {
#include "ev00c7/e102_action_7/dat_0cbbce6c.inc"
};
static NJS_COLOR dat_0cbbce90[9] = {
#include "ev00c7/e102_action_7/dat_0cbbce90.inc"
};
static NJS_MESHSET dat_0cbbceb4[6] = {
#include "ev00c7/e102_action_7/dat_0cbbceb4.inc"
};
static NJS_POINT3 dat_0cbbcf44[60] = {
#include "ev00c7/e102_action_7/dat_0cbbcf44.inc"
};
static NJS_VECTOR dat_0cbbd214[60] = {
#include "ev00c7/e102_action_7/dat_0cbbd214.inc"
};
static NJS_MODEL dat_0cbbd4e4 = {
#include "ev00c7/e102_action_7/dat_0cbbd4e4.inc"
};
static NJS_OBJECT dat_0cbbd50c = {
#include "ev00c7/e102_action_7/dat_0cbbd50c.inc"
};
static NJS_OBJECT dat_0cbbd540 = {
#include "ev00c7/e102_action_7/dat_0cbbd540.inc"
};
static NJS_OBJECT dat_0cbbd574 = {
#include "ev00c7/e102_action_7/dat_0cbbd574.inc"
};
static NJS_OBJECT dat_0cbbd5a8 = {
#include "ev00c7/e102_action_7/dat_0cbbd5a8.inc"
};
static NJS_MATERIAL dat_0cbbd5dc[6] = {
#include "ev00c7/e102_action_7/dat_0cbbd5dc.inc"
};
static Sint16 dat_0cbbd654[35] = {
#include "ev00c7/e102_action_7/dat_0cbbd654.inc"
};
static Sint16 dat_0cbbd69a[124] = {
#include "ev00c7/e102_action_7/dat_0cbbd69a.inc"
};
static Sint16 dat_0cbbd792[25] = {
#include "ev00c7/e102_action_7/dat_0cbbd792.inc"
};
static Sint16 dat_0cbbd7c4[10] = {
#include "ev00c7/e102_action_7/dat_0cbbd7c4.inc"
};
static Sint16 dat_0cbbd7d8[18] = {
#include "ev00c7/e102_action_7/dat_0cbbd7d8.inc"
};
static Sint16 dat_0cbbd7fc[14] = {
#include "ev00c7/e102_action_7/dat_0cbbd7fc.inc"
};
static NJS_COLOR dat_0cbbd818[30] = {
#include "ev00c7/e102_action_7/dat_0cbbd818.inc"
};
static NJS_COLOR dat_0cbbd890[110] = {
#include "ev00c7/e102_action_7/dat_0cbbd890.inc"
};
static NJS_COLOR dat_0cbbda48[20] = {
#include "ev00c7/e102_action_7/dat_0cbbda48.inc"
};
static NJS_COLOR dat_0cbbda98[8] = {
#include "ev00c7/e102_action_7/dat_0cbbda98.inc"
};
static NJS_COLOR dat_0cbbdab8[16] = {
#include "ev00c7/e102_action_7/dat_0cbbdab8.inc"
};
static NJS_COLOR dat_0cbbdaf8[12] = {
#include "ev00c7/e102_action_7/dat_0cbbdaf8.inc"
};
static NJS_MESHSET dat_0cbbdb28[6] = {
#include "ev00c7/e102_action_7/dat_0cbbdb28.inc"
};
static NJS_POINT3 dat_0cbbdbb8[72] = {
#include "ev00c7/e102_action_7/dat_0cbbdbb8.inc"
};
static NJS_VECTOR dat_0cbbdf18[72] = {
#include "ev00c7/e102_action_7/dat_0cbbdf18.inc"
};
static NJS_MODEL dat_0cbbe278 = {
#include "ev00c7/e102_action_7/dat_0cbbe278.inc"
};
static NJS_OBJECT dat_0cbbe2a0 = {
#include "ev00c7/e102_action_7/dat_0cbbe2a0.inc"
};
static NJS_OBJECT dat_0cbbe2d4 = {
#include "ev00c7/e102_action_7/dat_0cbbe2d4.inc"
};
static NJS_OBJECT dat_0cbbe308 = {
#include "ev00c7/e102_action_7/dat_0cbbe308.inc"
};
static NJS_OBJECT dat_0cbbe33c = {
#include "ev00c7/e102_action_7/dat_0cbbe33c.inc"
};
static NJS_OBJECT dat_0cbbe370 = {
#include "ev00c7/e102_action_7/dat_0cbbe370.inc"
};
static NJS_OBJECT dat_0cbbe3a4 = {
#include "ev00c7/e102_action_7/dat_0cbbe3a4.inc"
};
static NJS_OBJECT dat_0cbbe3d8 = {
#include "ev00c7/e102_action_7/dat_0cbbe3d8.inc"
};
static NJS_OBJECT dat_0cbbe40c = {
#include "ev00c7/e102_action_7/dat_0cbbe40c.inc"
};
static NJS_OBJECT dat_0cbbe440 = {
#include "ev00c7/e102_action_7/dat_0cbbe440.inc"
};
static NJS_MKEY_F dat_0cbbe474[59] = {
#include "ev00c7/e102_action_7/dat_0cbbe474.inc"
};
static NJS_MKEY_A dat_0cbbe824[57] = {
#include "ev00c7/e102_action_7/dat_0cbbe824.inc"
};
static NJS_MKEY_A dat_0cbbebb4[58] = {
#include "ev00c7/e102_action_7/dat_0cbbebb4.inc"
};
static NJS_MKEY_A dat_0cbbef54[59] = {
#include "ev00c7/e102_action_7/dat_0cbbef54.inc"
};
static NJS_MKEY_A dat_0cbbf304[59] = {
#include "ev00c7/e102_action_7/dat_0cbbf304.inc"
};
static NJS_MKEY_A dat_0cbbf6b4[59] = {
#include "ev00c7/e102_action_7/dat_0cbbf6b4.inc"
};
static NJS_MKEY_A dat_0cbbfa64[59] = {
#include "ev00c7/e102_action_7/dat_0cbbfa64.inc"
};
static NJS_MKEY_A dat_0cbbfe14[59] = {
#include "ev00c7/e102_action_7/dat_0cbbfe14.inc"
};
static NJS_MKEY_A dat_0cbc01c4[56] = {
#include "ev00c7/e102_action_7/dat_0cbc01c4.inc"
};
static NJS_MKEY_A dat_0cbc0544[58] = {
#include "ev00c7/e102_action_7/dat_0cbc0544.inc"
};
static NJS_MKEY_A dat_0cbc08e4[58] = {
#include "ev00c7/e102_action_7/dat_0cbc08e4.inc"
};
static NJS_MKEY_A dat_0cbc0c84[55] = {
#include "ev00c7/e102_action_7/dat_0cbc0c84.inc"
};
static NJS_MDATA2 dat_0cbc0ff4[65] = {
#include "ev00c7/e102_action_7/dat_0cbc0ff4.inc"
};
static NJS_MOTION ev00c7_e101_motion_0 = {
#include "ev00c7/e102_action_7/ev00c7_e101_motion_0.inc"
};
NJS_ACTION ev00c7_e101_action_0 = {
#include "ev00c7/e102_action_7/ev00c7_e101_action_0.inc"
};
static NJS_MKEY_F dat_0cbc1418[2] = {
#include "ev00c7/e102_action_7/dat_0cbc1418.inc"
};
static NJS_MKEY_A dat_0cbc1438[28] = {
#include "ev00c7/e102_action_7/dat_0cbc1438.inc"
};
static NJS_MKEY_A dat_0cbc15f8[29] = {
#include "ev00c7/e102_action_7/dat_0cbc15f8.inc"
};
static NJS_MKEY_A dat_0cbc17c8[29] = {
#include "ev00c7/e102_action_7/dat_0cbc17c8.inc"
};
static NJS_MKEY_A dat_0cbc1998[29] = {
#include "ev00c7/e102_action_7/dat_0cbc1998.inc"
};
static NJS_MKEY_A dat_0cbc1b68[29] = {
#include "ev00c7/e102_action_7/dat_0cbc1b68.inc"
};
static NJS_MKEY_A dat_0cbc1d38[29] = {
#include "ev00c7/e102_action_7/dat_0cbc1d38.inc"
};
static NJS_MKEY_A dat_0cbc1f08[29] = {
#include "ev00c7/e102_action_7/dat_0cbc1f08.inc"
};
static NJS_MKEY_A dat_0cbc20d8[29] = {
#include "ev00c7/e102_action_7/dat_0cbc20d8.inc"
};
static NJS_MKEY_A dat_0cbc22a8[29] = {
#include "ev00c7/e102_action_7/dat_0cbc22a8.inc"
};
static NJS_MDATA2 dat_0cbc2478[65] = {
#include "ev00c7/e102_action_7/dat_0cbc2478.inc"
};
static NJS_MOTION ev00c7_e101_motion_1 = {
#include "ev00c7/e102_action_7/ev00c7_e101_motion_1.inc"
};
NJS_ACTION ev00c7_e101_action_1 = {
#include "ev00c7/e102_action_7/ev00c7_e101_action_1.inc"
};
static NJS_MKEY_F dat_0cbc289c[39] = {
#include "ev00c7/e102_action_7/dat_0cbc289c.inc"
};
static NJS_MKEY_A dat_0cbc2b0c[64] = {
#include "ev00c7/e102_action_7/dat_0cbc2b0c.inc"
};
static NJS_MKEY_A dat_0cbc2f0c[83] = {
#include "ev00c7/e102_action_7/dat_0cbc2f0c.inc"
};
static NJS_MKEY_A dat_0cbc343c[78] = {
#include "ev00c7/e102_action_7/dat_0cbc343c.inc"
};
static NJS_MKEY_A dat_0cbc391c[75] = {
#include "ev00c7/e102_action_7/dat_0cbc391c.inc"
};
static NJS_MKEY_A dat_0cbc3dcc[88] = {
#include "ev00c7/e102_action_7/dat_0cbc3dcc.inc"
};
static NJS_MKEY_A dat_0cbc434c[87] = {
#include "ev00c7/e102_action_7/dat_0cbc434c.inc"
};
static NJS_MKEY_A dat_0cbc48bc[80] = {
#include "ev00c7/e102_action_7/dat_0cbc48bc.inc"
};
static NJS_MKEY_A dat_0cbc4dbc[88] = {
#include "ev00c7/e102_action_7/dat_0cbc4dbc.inc"
};
static NJS_MKEY_A dat_0cbc533c[88] = {
#include "ev00c7/e102_action_7/dat_0cbc533c.inc"
};
static NJS_MDATA2 dat_0cbc58bc[65] = {
#include "ev00c7/e102_action_7/dat_0cbc58bc.inc"
};
static NJS_MOTION ev00c7_e101_motion_2 = {
#include "ev00c7/e102_action_7/ev00c7_e101_motion_2.inc"
};
NJS_ACTION ev00c7_e101_action_2 = {
#include "ev00c7/e102_action_7/ev00c7_e101_action_2.inc"
};
static NJS_TEXNAME ev00c7_blue_bird_textures[12] = {
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
};
NJS_TEXLIST ev00c7_blue_bird_texlist = {
#include "ev00c7/e102_action_7/ev00c7_blue_bird_texlist.inc"
};
static NJS_MATERIAL dat_0cbc5d78[1] = {
#include "ev00c7/e102_action_7/dat_0cbc5d78.inc"
};
static Sint16 dat_0cbc5d8c[6] = {
#include "ev00c7/e102_action_7/dat_0cbc5d8c.inc"
};
static NJS_COLOR dat_0cbc5d98[4] = {
#include "ev00c7/e102_action_7/dat_0cbc5d98.inc"
};
static NJS_MESHSET dat_0cbc5da8[1] = {
#include "ev00c7/e102_action_7/dat_0cbc5da8.inc"
};
static NJS_POINT3 dat_0cbc5dc0[4] = {
#include "ev00c7/e102_action_7/dat_0cbc5dc0.inc"
};
static NJS_VECTOR dat_0cbc5df0[4] = {
#include "ev00c7/e102_action_7/dat_0cbc5df0.inc"
};
static NJS_MODEL dat_0cbc5e20 = {
#include "ev00c7/e102_action_7/dat_0cbc5e20.inc"
};
static NJS_OBJECT dat_0cbc5e48 = {
#include "ev00c7/e102_action_7/dat_0cbc5e48.inc"
};
static NJS_MATERIAL dat_0cbc5e7c[1] = {
#include "ev00c7/e102_action_7/dat_0cbc5e7c.inc"
};
static Sint16 dat_0cbc5e90[12] = {
#include "ev00c7/e102_action_7/dat_0cbc5e90.inc"
};
static NJS_COLOR dat_0cbc5ea8[9] = {
#include "ev00c7/e102_action_7/dat_0cbc5ea8.inc"
};
static NJS_MESHSET dat_0cbc5ecc[1] = {
#include "ev00c7/e102_action_7/dat_0cbc5ecc.inc"
};
static NJS_POINT3 dat_0cbc5ee4[7] = {
#include "ev00c7/e102_action_7/dat_0cbc5ee4.inc"
};
static NJS_VECTOR dat_0cbc5f38[7] = {
#include "ev00c7/e102_action_7/dat_0cbc5f38.inc"
};
static NJS_MODEL dat_0cbc5f8c = {
#include "ev00c7/e102_action_7/dat_0cbc5f8c.inc"
};
static NJS_OBJECT dat_0cbc5fb4 = {
#include "ev00c7/e102_action_7/dat_0cbc5fb4.inc"
};
static NJS_OBJECT dat_0cbc5fe8 = {
#include "ev00c7/e102_action_7/dat_0cbc5fe8.inc"
};
static NJS_OBJECT dat_0cbc601c = {
#include "ev00c7/e102_action_7/dat_0cbc601c.inc"
};
static NJS_OBJECT dat_0cbc6050 = {
#include "ev00c7/e102_action_7/dat_0cbc6050.inc"
};
static NJS_OBJECT dat_0cbc6084 = {
#include "ev00c7/e102_action_7/dat_0cbc6084.inc"
};
static NJS_OBJECT dat_0cbc60b8 = {
#include "ev00c7/e102_action_7/dat_0cbc60b8.inc"
};
static NJS_OBJECT dat_0cbc60ec = {
#include "ev00c7/e102_action_7/dat_0cbc60ec.inc"
};
static NJS_MATERIAL dat_0cbc6120[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6120.inc"
};
static Sint16 dat_0cbc6134[6] = {
#include "ev00c7/e102_action_7/dat_0cbc6134.inc"
};
static NJS_COLOR dat_0cbc6140[4] = {
#include "ev00c7/e102_action_7/dat_0cbc6140.inc"
};
static NJS_MESHSET dat_0cbc6150[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6150.inc"
};
static NJS_POINT3 dat_0cbc6168[4] = {
#include "ev00c7/e102_action_7/dat_0cbc6168.inc"
};
static NJS_VECTOR dat_0cbc6198[4] = {
#include "ev00c7/e102_action_7/dat_0cbc6198.inc"
};
static NJS_MODEL dat_0cbc61c8 = {
#include "ev00c7/e102_action_7/dat_0cbc61c8.inc"
};
static NJS_OBJECT dat_0cbc61f0 = {
#include "ev00c7/e102_action_7/dat_0cbc61f0.inc"
};
static NJS_MATERIAL dat_0cbc6224[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6224.inc"
};
static Sint16 dat_0cbc6238[12] = {
#include "ev00c7/e102_action_7/dat_0cbc6238.inc"
};
static NJS_COLOR dat_0cbc6250[9] = {
#include "ev00c7/e102_action_7/dat_0cbc6250.inc"
};
static NJS_MESHSET dat_0cbc6274[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6274.inc"
};
static NJS_POINT3 dat_0cbc628c[7] = {
#include "ev00c7/e102_action_7/dat_0cbc628c.inc"
};
static NJS_VECTOR dat_0cbc62e0[7] = {
#include "ev00c7/e102_action_7/dat_0cbc62e0.inc"
};
static NJS_MODEL dat_0cbc6334 = {
#include "ev00c7/e102_action_7/dat_0cbc6334.inc"
};
static NJS_OBJECT dat_0cbc635c = {
#include "ev00c7/e102_action_7/dat_0cbc635c.inc"
};
static NJS_OBJECT dat_0cbc6390 = {
#include "ev00c7/e102_action_7/dat_0cbc6390.inc"
};
static NJS_OBJECT dat_0cbc63c4 = {
#include "ev00c7/e102_action_7/dat_0cbc63c4.inc"
};
static NJS_OBJECT dat_0cbc63f8 = {
#include "ev00c7/e102_action_7/dat_0cbc63f8.inc"
};
static NJS_OBJECT dat_0cbc642c = {
#include "ev00c7/e102_action_7/dat_0cbc642c.inc"
};
static NJS_OBJECT dat_0cbc6460 = {
#include "ev00c7/e102_action_7/dat_0cbc6460.inc"
};
static NJS_OBJECT dat_0cbc6494 = {
#include "ev00c7/e102_action_7/dat_0cbc6494.inc"
};
static NJS_OBJECT dat_0cbc64c8 = {
#include "ev00c7/e102_action_7/dat_0cbc64c8.inc"
};
static NJS_OBJECT dat_0cbc64fc = {
#include "ev00c7/e102_action_7/dat_0cbc64fc.inc"
};
static NJS_OBJECT dat_0cbc6530 = {
#include "ev00c7/e102_action_7/dat_0cbc6530.inc"
};
static NJS_OBJECT dat_0cbc6564 = {
#include "ev00c7/e102_action_7/dat_0cbc6564.inc"
};
static NJS_OBJECT dat_0cbc6598 = {
#include "ev00c7/e102_action_7/dat_0cbc6598.inc"
};
static NJS_MATERIAL dat_0cbc65cc[1] = {
#include "ev00c7/e102_action_7/dat_0cbc65cc.inc"
};
static Sint16 dat_0cbc65e0[18] = {
#include "ev00c7/e102_action_7/dat_0cbc65e0.inc"
};
static NJS_COLOR dat_0cbc6604[15] = {
#include "ev00c7/e102_action_7/dat_0cbc6604.inc"
};
static NJS_MESHSET dat_0cbc6640[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6640.inc"
};
static NJS_POINT3 dat_0cbc6658[7] = {
#include "ev00c7/e102_action_7/dat_0cbc6658.inc"
};
static NJS_VECTOR dat_0cbc66ac[7] = {
#include "ev00c7/e102_action_7/dat_0cbc66ac.inc"
};
static NJS_MODEL dat_0cbc6700 = {
#include "ev00c7/e102_action_7/dat_0cbc6700.inc"
};
static NJS_OBJECT dat_0cbc6728 = {
#include "ev00c7/e102_action_7/dat_0cbc6728.inc"
};
static NJS_OBJECT dat_0cbc675c = {
#include "ev00c7/e102_action_7/dat_0cbc675c.inc"
};
static NJS_OBJECT dat_0cbc6790 = {
#include "ev00c7/e102_action_7/dat_0cbc6790.inc"
};
static NJS_OBJECT dat_0cbc67c4 = {
#include "ev00c7/e102_action_7/dat_0cbc67c4.inc"
};
static NJS_MATERIAL dat_0cbc67f8[1] = {
#include "ev00c7/e102_action_7/dat_0cbc67f8.inc"
};
static Sint16 dat_0cbc680c[10] = {
#include "ev00c7/e102_action_7/dat_0cbc680c.inc"
};
static NJS_COLOR dat_0cbc6820[8] = {
#include "ev00c7/e102_action_7/dat_0cbc6820.inc"
};
static NJS_MESHSET dat_0cbc6840[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6840.inc"
};
static NJS_POINT3 dat_0cbc6858[8] = {
#include "ev00c7/e102_action_7/dat_0cbc6858.inc"
};
static NJS_VECTOR dat_0cbc68b8[8] = {
#include "ev00c7/e102_action_7/dat_0cbc68b8.inc"
};
static NJS_MODEL dat_0cbc6918 = {
#include "ev00c7/e102_action_7/dat_0cbc6918.inc"
};
static NJS_OBJECT dat_0cbc6940 = {
#include "ev00c7/e102_action_7/dat_0cbc6940.inc"
};
static NJS_MATERIAL dat_0cbc6974[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6974.inc"
};
static Sint16 dat_0cbc6988[10] = {
#include "ev00c7/e102_action_7/dat_0cbc6988.inc"
};
static NJS_COLOR dat_0cbc699c[8] = {
#include "ev00c7/e102_action_7/dat_0cbc699c.inc"
};
static NJS_MESHSET dat_0cbc69bc[1] = {
#include "ev00c7/e102_action_7/dat_0cbc69bc.inc"
};
static NJS_POINT3 dat_0cbc69d4[8] = {
#include "ev00c7/e102_action_7/dat_0cbc69d4.inc"
};
static NJS_VECTOR dat_0cbc6a34[8] = {
#include "ev00c7/e102_action_7/dat_0cbc6a34.inc"
};
static NJS_MODEL dat_0cbc6a94 = {
#include "ev00c7/e102_action_7/dat_0cbc6a94.inc"
};
static NJS_OBJECT dat_0cbc6abc = {
#include "ev00c7/e102_action_7/dat_0cbc6abc.inc"
};
static NJS_MATERIAL dat_0cbc6af0[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6af0.inc"
};
static Sint16 dat_0cbc6b04[16] = {
#include "ev00c7/e102_action_7/dat_0cbc6b04.inc"
};
static NJS_COLOR dat_0cbc6b24[12] = {
#include "ev00c7/e102_action_7/dat_0cbc6b24.inc"
};
static NJS_MESHSET dat_0cbc6b54[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6b54.inc"
};
static NJS_POINT3 dat_0cbc6b6c[12] = {
#include "ev00c7/e102_action_7/dat_0cbc6b6c.inc"
};
static NJS_VECTOR dat_0cbc6bfc[12] = {
#include "ev00c7/e102_action_7/dat_0cbc6bfc.inc"
};
static NJS_MODEL dat_0cbc6c8c = {
#include "ev00c7/e102_action_7/dat_0cbc6c8c.inc"
};
static NJS_OBJECT dat_0cbc6cb4 = {
#include "ev00c7/e102_action_7/dat_0cbc6cb4.inc"
};
static NJS_MATERIAL dat_0cbc6ce8[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6ce8.inc"
};
static Sint16 dat_0cbc6cfc[16] = {
#include "ev00c7/e102_action_7/dat_0cbc6cfc.inc"
};
static NJS_COLOR dat_0cbc6d1c[12] = {
#include "ev00c7/e102_action_7/dat_0cbc6d1c.inc"
};
static NJS_MESHSET dat_0cbc6d4c[1] = {
#include "ev00c7/e102_action_7/dat_0cbc6d4c.inc"
};
static NJS_POINT3 dat_0cbc6d64[12] = {
#include "ev00c7/e102_action_7/dat_0cbc6d64.inc"
};
static NJS_VECTOR dat_0cbc6df4[12] = {
#include "ev00c7/e102_action_7/dat_0cbc6df4.inc"
};
static NJS_MODEL dat_0cbc6e84 = {
#include "ev00c7/e102_action_7/dat_0cbc6e84.inc"
};
static NJS_OBJECT dat_0cbc6eac = {
#include "ev00c7/e102_action_7/dat_0cbc6eac.inc"
};
static NJS_MATERIAL dat_0cbc6ee0[4] = {
#include "ev00c7/e102_action_7/dat_0cbc6ee0.inc"
};
static Sint16 dat_0cbc6f30[239] = {
#include "ev00c7/e102_action_7/dat_0cbc6f30.inc"
};
static Sint16 dat_0cbc710e[22] = {
#include "ev00c7/e102_action_7/dat_0cbc710e.inc"
};
static Sint16 dat_0cbc713a[21] = {
#include "ev00c7/e102_action_7/dat_0cbc713a.inc"
};
static Sint16 dat_0cbc7164[22] = {
#include "ev00c7/e102_action_7/dat_0cbc7164.inc"
};
static NJS_COLOR dat_0cbc7190[209] = {
#include "ev00c7/e102_action_7/dat_0cbc7190.inc"
};
static NJS_COLOR dat_0cbc74d4[20] = {
#include "ev00c7/e102_action_7/dat_0cbc74d4.inc"
};
static NJS_COLOR dat_0cbc7524[16] = {
#include "ev00c7/e102_action_7/dat_0cbc7524.inc"
};
static NJS_COLOR dat_0cbc7564[18] = {
#include "ev00c7/e102_action_7/dat_0cbc7564.inc"
};
static NJS_MESHSET dat_0cbc75ac[4] = {
#include "ev00c7/e102_action_7/dat_0cbc75ac.inc"
};
static NJS_POINT3 dat_0cbc760c[127] = {
#include "ev00c7/e102_action_7/dat_0cbc760c.inc"
};
static NJS_VECTOR dat_0cbc7c00[127] = {
#include "ev00c7/e102_action_7/dat_0cbc7c00.inc"
};
static NJS_MODEL dat_0cbc81f4 = {
#include "ev00c7/e102_action_7/dat_0cbc81f4.inc"
};
static NJS_OBJECT dat_0cbc821c = {
#include "ev00c7/e102_action_7/dat_0cbc821c.inc"
};
static NJS_OBJECT dat_0cbc8250 = {
#include "ev00c7/e102_action_7/dat_0cbc8250.inc"
};
static NJS_OBJECT dat_0cbc8284 = {
#include "ev00c7/e102_action_7/dat_0cbc8284.inc"
};
static NJS_OBJECT dat_0cbc82b8 = {
#include "ev00c7/e102_action_7/dat_0cbc82b8.inc"
};
static NJS_MATERIAL dat_0cbc82ec[1] = {
#include "ev00c7/e102_action_7/dat_0cbc82ec.inc"
};
static Sint16 dat_0cbc8300[8] = {
#include "ev00c7/e102_action_7/dat_0cbc8300.inc"
};
static NJS_COLOR dat_0cbc8310[6] = {
#include "ev00c7/e102_action_7/dat_0cbc8310.inc"
};
static NJS_MESHSET dat_0cbc8328[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8328.inc"
};
static NJS_POINT3 dat_0cbc8340[6] = {
#include "ev00c7/e102_action_7/dat_0cbc8340.inc"
};
static NJS_VECTOR dat_0cbc8388[6] = {
#include "ev00c7/e102_action_7/dat_0cbc8388.inc"
};
static NJS_MODEL dat_0cbc83d0 = {
#include "ev00c7/e102_action_7/dat_0cbc83d0.inc"
};
static NJS_OBJECT dat_0cbc83f8 = {
#include "ev00c7/e102_action_7/dat_0cbc83f8.inc"
};
static NJS_MATERIAL dat_0cbc842c[1] = {
#include "ev00c7/e102_action_7/dat_0cbc842c.inc"
};
static Sint16 dat_0cbc8440[22] = {
#include "ev00c7/e102_action_7/dat_0cbc8440.inc"
};
static NJS_COLOR dat_0cbc846c[16] = {
#include "ev00c7/e102_action_7/dat_0cbc846c.inc"
};
static NJS_MESHSET dat_0cbc84ac[1] = {
#include "ev00c7/e102_action_7/dat_0cbc84ac.inc"
};
static NJS_POINT3 dat_0cbc84c4[5] = {
#include "ev00c7/e102_action_7/dat_0cbc84c4.inc"
};
static NJS_VECTOR dat_0cbc8500[5] = {
#include "ev00c7/e102_action_7/dat_0cbc8500.inc"
};
static NJS_MODEL dat_0cbc853c = {
#include "ev00c7/e102_action_7/dat_0cbc853c.inc"
};
static NJS_OBJECT dat_0cbc8564 = {
#include "ev00c7/e102_action_7/dat_0cbc8564.inc"
};
static NJS_MATERIAL dat_0cbc8598[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8598.inc"
};
static Sint16 dat_0cbc85ac[30] = {
#include "ev00c7/e102_action_7/dat_0cbc85ac.inc"
};
static NJS_COLOR dat_0cbc85e8[28] = {
#include "ev00c7/e102_action_7/dat_0cbc85e8.inc"
};
static NJS_MESHSET dat_0cbc8658[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8658.inc"
};
static NJS_POINT3 dat_0cbc8670[18] = {
#include "ev00c7/e102_action_7/dat_0cbc8670.inc"
};
static NJS_VECTOR dat_0cbc8748[18] = {
#include "ev00c7/e102_action_7/dat_0cbc8748.inc"
};
static NJS_MODEL dat_0cbc8820 = {
#include "ev00c7/e102_action_7/dat_0cbc8820.inc"
};
static NJS_OBJECT dat_0cbc8848 = {
#include "ev00c7/e102_action_7/dat_0cbc8848.inc"
};
static NJS_MATERIAL dat_0cbc887c[1] = {
#include "ev00c7/e102_action_7/dat_0cbc887c.inc"
};
static Sint16 dat_0cbc8890[20] = {
#include "ev00c7/e102_action_7/dat_0cbc8890.inc"
};
static NJS_COLOR dat_0cbc88b8[16] = {
#include "ev00c7/e102_action_7/dat_0cbc88b8.inc"
};
static NJS_MESHSET dat_0cbc88f8[1] = {
#include "ev00c7/e102_action_7/dat_0cbc88f8.inc"
};
static NJS_POINT3 dat_0cbc8910[12] = {
#include "ev00c7/e102_action_7/dat_0cbc8910.inc"
};
static NJS_VECTOR dat_0cbc89a0[12] = {
#include "ev00c7/e102_action_7/dat_0cbc89a0.inc"
};
static NJS_MODEL dat_0cbc8a30 = {
#include "ev00c7/e102_action_7/dat_0cbc8a30.inc"
};
static NJS_OBJECT dat_0cbc8a58 = {
#include "ev00c7/e102_action_7/dat_0cbc8a58.inc"
};
static NJS_MATERIAL dat_0cbc8a8c[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8a8c.inc"
};
static Sint16 dat_0cbc8aa0[10] = {
#include "ev00c7/e102_action_7/dat_0cbc8aa0.inc"
};
static NJS_COLOR dat_0cbc8ab4[8] = {
#include "ev00c7/e102_action_7/dat_0cbc8ab4.inc"
};
static NJS_MESHSET dat_0cbc8ad4[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8ad4.inc"
};
static NJS_POINT3 dat_0cbc8aec[5] = {
#include "ev00c7/e102_action_7/dat_0cbc8aec.inc"
};
static NJS_VECTOR dat_0cbc8b28[5] = {
#include "ev00c7/e102_action_7/dat_0cbc8b28.inc"
};
static NJS_MODEL dat_0cbc8b64 = {
#include "ev00c7/e102_action_7/dat_0cbc8b64.inc"
};
static NJS_OBJECT dat_0cbc8b8c = {
#include "ev00c7/e102_action_7/dat_0cbc8b8c.inc"
};
static NJS_MATERIAL dat_0cbc8bc0[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8bc0.inc"
};
static Sint16 dat_0cbc8bd4[10] = {
#include "ev00c7/e102_action_7/dat_0cbc8bd4.inc"
};
static NJS_COLOR dat_0cbc8be8[8] = {
#include "ev00c7/e102_action_7/dat_0cbc8be8.inc"
};
static NJS_MESHSET dat_0cbc8c08[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8c08.inc"
};
static NJS_POINT3 dat_0cbc8c20[5] = {
#include "ev00c7/e102_action_7/dat_0cbc8c20.inc"
};
static NJS_VECTOR dat_0cbc8c5c[5] = {
#include "ev00c7/e102_action_7/dat_0cbc8c5c.inc"
};
static NJS_MODEL dat_0cbc8c98 = {
#include "ev00c7/e102_action_7/dat_0cbc8c98.inc"
};
static NJS_OBJECT dat_0cbc8cc0 = {
#include "ev00c7/e102_action_7/dat_0cbc8cc0.inc"
};
static NJS_MATERIAL dat_0cbc8cf4[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8cf4.inc"
};
static Sint16 dat_0cbc8d08[24] = {
#include "ev00c7/e102_action_7/dat_0cbc8d08.inc"
};
static NJS_COLOR dat_0cbc8d38[20] = {
#include "ev00c7/e102_action_7/dat_0cbc8d38.inc"
};
static NJS_MESHSET dat_0cbc8d88[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8d88.inc"
};
static NJS_POINT3 dat_0cbc8da0[12] = {
#include "ev00c7/e102_action_7/dat_0cbc8da0.inc"
};
static NJS_VECTOR dat_0cbc8e30[12] = {
#include "ev00c7/e102_action_7/dat_0cbc8e30.inc"
};
static NJS_MODEL dat_0cbc8ec0 = {
#include "ev00c7/e102_action_7/dat_0cbc8ec0.inc"
};
static NJS_OBJECT dat_0cbc8ee8 = {
#include "ev00c7/e102_action_7/dat_0cbc8ee8.inc"
};
static NJS_MATERIAL dat_0cbc8f1c[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8f1c.inc"
};
static Sint16 dat_0cbc8f30[36] = {
#include "ev00c7/e102_action_7/dat_0cbc8f30.inc"
};
static NJS_COLOR dat_0cbc8f78[30] = {
#include "ev00c7/e102_action_7/dat_0cbc8f78.inc"
};
static NJS_MESHSET dat_0cbc8ff0[1] = {
#include "ev00c7/e102_action_7/dat_0cbc8ff0.inc"
};
static NJS_POINT3 dat_0cbc9008[12] = {
#include "ev00c7/e102_action_7/dat_0cbc9008.inc"
};
static NJS_VECTOR dat_0cbc9098[12] = {
#include "ev00c7/e102_action_7/dat_0cbc9098.inc"
};
static NJS_MODEL dat_0cbc9128 = {
#include "ev00c7/e102_action_7/dat_0cbc9128.inc"
};
static NJS_OBJECT dat_0cbc9150 = {
#include "ev00c7/e102_action_7/dat_0cbc9150.inc"
};
static NJS_OBJECT dat_0cbc9184 = {
#include "ev00c7/e102_action_7/dat_0cbc9184.inc"
};
static NJS_OBJECT dat_0cbc91b8 = {
#include "ev00c7/e102_action_7/dat_0cbc91b8.inc"
};
static NJS_OBJECT dat_0cbc91ec = {
#include "ev00c7/e102_action_7/dat_0cbc91ec.inc"
};
static NJS_MATERIAL dat_0cbc9220[1] = {
#include "ev00c7/e102_action_7/dat_0cbc9220.inc"
};
static Sint16 dat_0cbc9234[48] = {
#include "ev00c7/e102_action_7/dat_0cbc9234.inc"
};
static NJS_COLOR dat_0cbc9294[38] = {
#include "ev00c7/e102_action_7/dat_0cbc9294.inc"
};
static NJS_MESHSET dat_0cbc932c[1] = {
#include "ev00c7/e102_action_7/dat_0cbc932c.inc"
};
static NJS_POINT3 dat_0cbc9344[12] = {
#include "ev00c7/e102_action_7/dat_0cbc9344.inc"
};
static NJS_VECTOR dat_0cbc93d4[12] = {
#include "ev00c7/e102_action_7/dat_0cbc93d4.inc"
};
static NJS_MODEL dat_0cbc9464 = {
#include "ev00c7/e102_action_7/dat_0cbc9464.inc"
};
static NJS_OBJECT dat_0cbc948c = {
#include "ev00c7/e102_action_7/dat_0cbc948c.inc"
};
static NJS_OBJECT dat_0cbc94c0 = {
#include "ev00c7/e102_action_7/dat_0cbc94c0.inc"
};
static NJS_OBJECT dat_0cbc94f4 = {
#include "ev00c7/e102_action_7/dat_0cbc94f4.inc"
};
static NJS_OBJECT dat_0cbc9528 = {
#include "ev00c7/e102_action_7/dat_0cbc9528.inc"
};
static NJS_MATERIAL dat_0cbc955c[1] = {
#include "ev00c7/e102_action_7/dat_0cbc955c.inc"
};
static Sint16 dat_0cbc9570[48] = {
#include "ev00c7/e102_action_7/dat_0cbc9570.inc"
};
static NJS_COLOR dat_0cbc95d0[38] = {
#include "ev00c7/e102_action_7/dat_0cbc95d0.inc"
};
static NJS_MESHSET dat_0cbc9668[1] = {
#include "ev00c7/e102_action_7/dat_0cbc9668.inc"
};
static NJS_POINT3 dat_0cbc9680[12] = {
#include "ev00c7/e102_action_7/dat_0cbc9680.inc"
};
static NJS_VECTOR dat_0cbc9710[12] = {
#include "ev00c7/e102_action_7/dat_0cbc9710.inc"
};
static NJS_MODEL dat_0cbc97a0 = {
#include "ev00c7/e102_action_7/dat_0cbc97a0.inc"
};
static NJS_OBJECT dat_0cbc97c8 = {
#include "ev00c7/e102_action_7/dat_0cbc97c8.inc"
};
static NJS_OBJECT dat_0cbc97fc = {
#include "ev00c7/e102_action_7/dat_0cbc97fc.inc"
};
static NJS_OBJECT dat_0cbc9830 = {
#include "ev00c7/e102_action_7/dat_0cbc9830.inc"
};
static NJS_OBJECT dat_0cbc9864 = {
#include "ev00c7/e102_action_7/dat_0cbc9864.inc"
};
static NJS_MATERIAL dat_0cbc9898[2] = {
#include "ev00c7/e102_action_7/dat_0cbc9898.inc"
};
static Sint16 dat_0cbc98c0[54] = {
#include "ev00c7/e102_action_7/dat_0cbc98c0.inc"
};
static Sint16 dat_0cbc992c[22] = {
#include "ev00c7/e102_action_7/dat_0cbc992c.inc"
};
static NJS_COLOR dat_0cbc9958[48] = {
#include "ev00c7/e102_action_7/dat_0cbc9958.inc"
};
static NJS_COLOR dat_0cbc9a18[18] = {
#include "ev00c7/e102_action_7/dat_0cbc9a18.inc"
};
static NJS_MESHSET dat_0cbc9a60[2] = {
#include "ev00c7/e102_action_7/dat_0cbc9a60.inc"
};
static NJS_POINT3 dat_0cbc9a90[26] = {
#include "ev00c7/e102_action_7/dat_0cbc9a90.inc"
};
static NJS_VECTOR dat_0cbc9bc8[26] = {
#include "ev00c7/e102_action_7/dat_0cbc9bc8.inc"
};
static NJS_MODEL dat_0cbc9d00 = {
#include "ev00c7/e102_action_7/dat_0cbc9d00.inc"
};
static NJS_OBJECT dat_0cbc9d28 = {
#include "ev00c7/e102_action_7/dat_0cbc9d28.inc"
};
static NJS_OBJECT dat_0cbc9d5c = {
#include "ev00c7/e102_action_7/dat_0cbc9d5c.inc"
};
static NJS_OBJECT dat_0cbc9d90 = {
#include "ev00c7/e102_action_7/dat_0cbc9d90.inc"
};
static NJS_OBJECT dat_0cbc9dc4 = {
#include "ev00c7/e102_action_7/dat_0cbc9dc4.inc"
};
static NJS_OBJECT dat_0cbc9df8 = {
#include "ev00c7/e102_action_7/dat_0cbc9df8.inc"
};
static NJS_MKEY_F dat_0cbc9e2c[2] = {
#include "ev00c7/e102_action_7/dat_0cbc9e2c.inc"
};
static NJS_MKEY_F dat_0cbc9e4c[30] = {
#include "ev00c7/e102_action_7/dat_0cbc9e4c.inc"
};
static NJS_MKEY_F dat_0cbca02c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca02c.inc"
};
static NJS_MKEY_A dat_0cbca04c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca04c.inc"
};
static NJS_MKEY_F dat_0cbca06c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca06c.inc"
};
static NJS_MKEY_F dat_0cbca08c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca08c.inc"
};
static NJS_MKEY_F dat_0cbca0ac[2] = {
#include "ev00c7/e102_action_7/dat_0cbca0ac.inc"
};
static NJS_MKEY_F dat_0cbca0cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbca0cc.inc"
};
static NJS_MKEY_F dat_0cbca0ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbca0ec.inc"
};
static NJS_MKEY_A dat_0cbca10c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca10c.inc"
};
static NJS_MKEY_F dat_0cbca12c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca12c.inc"
};
static NJS_MKEY_A dat_0cbca14c[30] = {
#include "ev00c7/e102_action_7/dat_0cbca14c.inc"
};
static NJS_MKEY_F dat_0cbca32c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca32c.inc"
};
static NJS_MKEY_F dat_0cbca34c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca34c.inc"
};
static NJS_MKEY_F dat_0cbca36c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca36c.inc"
};
static NJS_MKEY_F dat_0cbca38c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca38c.inc"
};
static NJS_MKEY_F dat_0cbca3ac[2] = {
#include "ev00c7/e102_action_7/dat_0cbca3ac.inc"
};
static NJS_MKEY_A dat_0cbca3cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbca3cc.inc"
};
static NJS_MKEY_F dat_0cbca3ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbca3ec.inc"
};
static NJS_MKEY_A dat_0cbca40c[30] = {
#include "ev00c7/e102_action_7/dat_0cbca40c.inc"
};
static NJS_MKEY_F dat_0cbca5ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbca5ec.inc"
};
static NJS_MKEY_F dat_0cbca60c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca60c.inc"
};
static NJS_MKEY_F dat_0cbca62c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca62c.inc"
};
static NJS_MKEY_F dat_0cbca64c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca64c.inc"
};
static NJS_MKEY_F dat_0cbca66c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca66c.inc"
};
static NJS_MKEY_A dat_0cbca68c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca68c.inc"
};
static NJS_MKEY_F dat_0cbca6ac[2] = {
#include "ev00c7/e102_action_7/dat_0cbca6ac.inc"
};
static NJS_MKEY_A dat_0cbca6cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbca6cc.inc"
};
static NJS_MKEY_F dat_0cbca6ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbca6ec.inc"
};
static NJS_MKEY_F dat_0cbca70c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca70c.inc"
};
static NJS_MKEY_F dat_0cbca72c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca72c.inc"
};
static NJS_MKEY_F dat_0cbca74c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca74c.inc"
};
static NJS_MKEY_F dat_0cbca76c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca76c.inc"
};
static NJS_MKEY_F dat_0cbca78c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca78c.inc"
};
static NJS_MKEY_F dat_0cbca7ac[2] = {
#include "ev00c7/e102_action_7/dat_0cbca7ac.inc"
};
static NJS_MKEY_F dat_0cbca7cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbca7cc.inc"
};
static NJS_MKEY_A dat_0cbca7ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbca7ec.inc"
};
static NJS_MKEY_F dat_0cbca80c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca80c.inc"
};
static NJS_MKEY_F dat_0cbca82c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca82c.inc"
};
static NJS_MKEY_A dat_0cbca84c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca84c.inc"
};
static NJS_MKEY_F dat_0cbca86c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca86c.inc"
};
static NJS_MKEY_A dat_0cbca88c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca88c.inc"
};
static NJS_MKEY_F dat_0cbca8ac[2] = {
#include "ev00c7/e102_action_7/dat_0cbca8ac.inc"
};
static NJS_MKEY_A dat_0cbca8cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbca8cc.inc"
};
static NJS_MKEY_F dat_0cbca8ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbca8ec.inc"
};
static NJS_MKEY_A dat_0cbca90c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca90c.inc"
};
static NJS_MKEY_F dat_0cbca92c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca92c.inc"
};
static NJS_MKEY_F dat_0cbca94c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca94c.inc"
};
static NJS_MKEY_F dat_0cbca96c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca96c.inc"
};
static NJS_MKEY_F dat_0cbca98c[2] = {
#include "ev00c7/e102_action_7/dat_0cbca98c.inc"
};
static NJS_MKEY_F dat_0cbca9ac[2] = {
#include "ev00c7/e102_action_7/dat_0cbca9ac.inc"
};
static NJS_MKEY_A dat_0cbca9cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbca9cc.inc"
};
static NJS_MKEY_F dat_0cbca9ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbca9ec.inc"
};
static NJS_MKEY_F dat_0cbcaa0c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcaa0c.inc"
};
static NJS_MKEY_A dat_0cbcaa2c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcaa2c.inc"
};
static NJS_MKEY_F dat_0cbcaa4c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcaa4c.inc"
};
static NJS_MKEY_F dat_0cbcaa6c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcaa6c.inc"
};
static NJS_MKEY_A dat_0cbcaa8c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcaa8c.inc"
};
static NJS_MKEY_F dat_0cbcaaac[2] = {
#include "ev00c7/e102_action_7/dat_0cbcaaac.inc"
};
static NJS_MKEY_F dat_0cbcaacc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcaacc.inc"
};
static NJS_MKEY_A dat_0cbcaaec[2] = {
#include "ev00c7/e102_action_7/dat_0cbcaaec.inc"
};
static NJS_MKEY_F dat_0cbcab0c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcab0c.inc"
};
static NJS_MKEY_F dat_0cbcab2c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcab2c.inc"
};
static NJS_MKEY_A dat_0cbcab4c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcab4c.inc"
};
static NJS_MKEY_F dat_0cbcab6c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcab6c.inc"
};
static NJS_MKEY_A dat_0cbcab8c[30] = {
#include "ev00c7/e102_action_7/dat_0cbcab8c.inc"
};
static NJS_MKEY_F dat_0cbcad6c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcad6c.inc"
};
static NJS_MKEY_F dat_0cbcad8c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcad8c.inc"
};
static NJS_MKEY_F dat_0cbcadac[2] = {
#include "ev00c7/e102_action_7/dat_0cbcadac.inc"
};
static NJS_MKEY_F dat_0cbcadcc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcadcc.inc"
};
static NJS_MKEY_F dat_0cbcadec[30] = {
#include "ev00c7/e102_action_7/dat_0cbcadec.inc"
};
static NJS_MKEY_A dat_0cbcafcc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcafcc.inc"
};
static NJS_MKEY_F dat_0cbcafec[2] = {
#include "ev00c7/e102_action_7/dat_0cbcafec.inc"
};
static NJS_MKEY_F dat_0cbcb00c[30] = {
#include "ev00c7/e102_action_7/dat_0cbcb00c.inc"
};
static NJS_MKEY_A dat_0cbcb1ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb1ec.inc"
};
static NJS_MKEY_F dat_0cbcb20c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb20c.inc"
};
static NJS_MKEY_F dat_0cbcb22c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb22c.inc"
};
static NJS_MKEY_A dat_0cbcb24c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb24c.inc"
};
static NJS_MKEY_F dat_0cbcb26c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb26c.inc"
};
static NJS_MKEY_F dat_0cbcb28c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb28c.inc"
};
static NJS_MKEY_A dat_0cbcb2ac[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb2ac.inc"
};
static NJS_MKEY_F dat_0cbcb2cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb2cc.inc"
};
static NJS_MKEY_F dat_0cbcb2ec[30] = {
#include "ev00c7/e102_action_7/dat_0cbcb2ec.inc"
};
static NJS_MKEY_A dat_0cbcb4cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb4cc.inc"
};
static NJS_MKEY_F dat_0cbcb4ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb4ec.inc"
};
static NJS_MKEY_F dat_0cbcb50c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb50c.inc"
};
static NJS_MKEY_A dat_0cbcb52c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb52c.inc"
};
static NJS_MKEY_F dat_0cbcb54c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb54c.inc"
};
static NJS_MKEY_A dat_0cbcb56c[30] = {
#include "ev00c7/e102_action_7/dat_0cbcb56c.inc"
};
static NJS_MKEY_F dat_0cbcb74c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb74c.inc"
};
static NJS_MKEY_F dat_0cbcb76c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb76c.inc"
};
static NJS_MKEY_F dat_0cbcb78c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb78c.inc"
};
static NJS_MKEY_A dat_0cbcb7ac[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb7ac.inc"
};
static NJS_MKEY_F dat_0cbcb7cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb7cc.inc"
};
static NJS_MKEY_A dat_0cbcb7ec[30] = {
#include "ev00c7/e102_action_7/dat_0cbcb7ec.inc"
};
static NJS_MKEY_F dat_0cbcb9cc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb9cc.inc"
};
static NJS_MKEY_F dat_0cbcb9ec[2] = {
#include "ev00c7/e102_action_7/dat_0cbcb9ec.inc"
};
static NJS_MKEY_F dat_0cbcba0c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcba0c.inc"
};
static NJS_MKEY_F dat_0cbcba2c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcba2c.inc"
};
static NJS_MKEY_F dat_0cbcba4c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcba4c.inc"
};
static NJS_MKEY_F dat_0cbcba6c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcba6c.inc"
};
static NJS_MKEY_F dat_0cbcba8c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcba8c.inc"
};
static NJS_MKEY_A dat_0cbcbaac[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbaac.inc"
};
static NJS_MKEY_F dat_0cbcbacc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbacc.inc"
};
static NJS_MKEY_A dat_0cbcbaec[30] = {
#include "ev00c7/e102_action_7/dat_0cbcbaec.inc"
};
static NJS_MKEY_F dat_0cbcbccc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbccc.inc"
};
static NJS_MKEY_F dat_0cbcbcec[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbcec.inc"
};
static NJS_MKEY_F dat_0cbcbd0c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbd0c.inc"
};
static NJS_MKEY_A dat_0cbcbd2c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbd2c.inc"
};
static NJS_MKEY_F dat_0cbcbd4c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbd4c.inc"
};
static NJS_MKEY_A dat_0cbcbd6c[30] = {
#include "ev00c7/e102_action_7/dat_0cbcbd6c.inc"
};
static NJS_MKEY_F dat_0cbcbf4c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbf4c.inc"
};
static NJS_MKEY_F dat_0cbcbf6c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbf6c.inc"
};
static NJS_MKEY_F dat_0cbcbf8c[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbf8c.inc"
};
static NJS_MKEY_F dat_0cbcbfac[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbfac.inc"
};
static NJS_MKEY_F dat_0cbcbfcc[2] = {
#include "ev00c7/e102_action_7/dat_0cbcbfcc.inc"
};
static NJS_MDATA3 dat_0cbcbfec[57] = {
#include "ev00c7/e102_action_7/dat_0cbcbfec.inc"
};
static NJS_MOTION dat_0cbcc544 = {
#include "ev00c7/e102_action_7/dat_0cbcc544.inc"
};
static NJS_ACTION dat_0cbcc550 = {
#include "ev00c7/e102_action_7/dat_0cbcc550.inc"
};
static NJS_TEXNAME ev00c7_gray_bird_textures[7] = {
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_36, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_42, 0, 0}, {TEXT_44, 0, 0}, {TEXT_45, 0, 0},
};
NJS_TEXLIST ev00c7_gray_bird_texlist = {
#include "ev00c7/e102_action_7/ev00c7_gray_bird_texlist.inc"
};
static NJS_TEXNAME ev00c7_pink_bird_textures[7] = {
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_36, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_42, 0, 0}, {TEXT_44, 0, 0}, {TEXT_46, 0, 0},
};
