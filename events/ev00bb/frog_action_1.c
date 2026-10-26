/* ev00bb_frog_action_1 and its motion, the texlists of the frogs of E-103,
 * E-104 and E-105, and the objects of E-103's and E-104's. nothing reads
 * two more texlists, dat_0cbb89e0 and dat_0cbbaf90 */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00bb/frog_action_1.h"

#define TEXT_1_LENGTH 5
#define TEXT_2_LENGTH 5
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 5
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 5
#define TEXT_7_LENGTH 5
#define TEXT_8_LENGTH 5
#define TEXT_9_LENGTH 5
#define TEXT_10_LENGTH 5
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 5
#define TEXT_13_LENGTH 5
#define TEXT_14_LENGTH 5
#define TEXT_15_LENGTH 5
#define TEXT_16_LENGTH 5
#define TEXT_17_LENGTH 5
#define TEXT_18_LENGTH 5
#include "ev00bb/frog_action_1/text.inc"

extern NJS_OBJECT dat_0cbb5230;
extern NJS_MDATA2 dat_0cbb8768[];

static NJS_MOTION ev00bb_frog_motion_1 = {
#include "ev00bb/frog_action_1/ev00bb_frog_motion_1.inc"
};
NJS_ACTION ev00bb_frog_action_1 = {
#include "ev00bb/frog_action_1/ev00bb_frog_action_1.inc"
};
static NJS_TEXNAME ev00bb_frog_1_textures[7] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0},
};
NJS_TEXLIST ev00bb_frog_1_texlist = {
#include "ev00bb/frog_action_1/ev00bb_frog_1_texlist.inc"
};
static NJS_TEXNAME ev00bb_frog_2_textures[7] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev00bb_frog_2_texlist = {
#include "ev00bb/frog_action_1/ev00bb_frog_2_texlist.inc"
};
static NJS_TEXNAME ev00bb_frog_3_textures[8] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
    {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
};
NJS_TEXLIST ev00bb_frog_3_texlist = {
#include "ev00bb/frog_action_1/ev00bb_frog_3_texlist.inc"
};
static NJS_TEXNAME dat_0cbb898c[7] = {
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0},
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
};
static NJS_TEXLIST dat_0cbb89e0 = {
#include "ev00bb/frog_action_1/dat_0cbb89e0.inc"
};
static NJS_OBJECT dat_0cbb89e8 = {
#include "ev00bb/frog_action_1/dat_0cbb89e8.inc"
};
static NJS_OBJECT dat_0cbb8a1c = {
#include "ev00bb/frog_action_1/dat_0cbb8a1c.inc"
};
static NJS_OBJECT dat_0cbb8a50 = {
#include "ev00bb/frog_action_1/dat_0cbb8a50.inc"
};
static NJS_OBJECT dat_0cbb8a84 = {
#include "ev00bb/frog_action_1/dat_0cbb8a84.inc"
};
static NJS_MATERIAL dat_0cbb8ab8[1] = {
#include "ev00bb/frog_action_1/dat_0cbb8ab8.inc"
};
static Sint16 dat_0cbb8acc[18] = {
#include "ev00bb/frog_action_1/dat_0cbb8acc.inc"
};
static NJS_COLOR dat_0cbb8af0[18] = {
#include "ev00bb/frog_action_1/dat_0cbb8af0.inc"
};
static NJS_MESHSET dat_0cbb8b38[1] = {
#include "ev00bb/frog_action_1/dat_0cbb8b38.inc"
};
static NJS_POINT3 dat_0cbb8b50[5] = {
#include "ev00bb/frog_action_1/dat_0cbb8b50.inc"
};
static NJS_VECTOR dat_0cbb8b8c[5] = {
#include "ev00bb/frog_action_1/dat_0cbb8b8c.inc"
};
static NJS_MODEL dat_0cbb8bc8 = {
#include "ev00bb/frog_action_1/dat_0cbb8bc8.inc"
};
static NJS_OBJECT dat_0cbb8bf0 = {
#include "ev00bb/frog_action_1/dat_0cbb8bf0.inc"
};
static NJS_MATERIAL dat_0cbb8c24[1] = {
#include "ev00bb/frog_action_1/dat_0cbb8c24.inc"
};
static Sint16 dat_0cbb8c38[30] = {
#include "ev00bb/frog_action_1/dat_0cbb8c38.inc"
};
static Sint16 dat_0cbb8c74[12] = {
#include "ev00bb/frog_action_1/dat_0cbb8c74.inc"
};
static NJS_COLOR dat_0cbb8c8c[30] = {
#include "ev00bb/frog_action_1/dat_0cbb8c8c.inc"
};
static NJS_COLOR dat_0cbb8d04[12] = {
#include "ev00bb/frog_action_1/dat_0cbb8d04.inc"
};
static NJS_MESHSET dat_0cbb8d34[2] = {
#include "ev00bb/frog_action_1/dat_0cbb8d34.inc"
};
static NJS_POINT3 dat_0cbb8d64[10] = {
#include "ev00bb/frog_action_1/dat_0cbb8d64.inc"
};
static NJS_VECTOR dat_0cbb8ddc[10] = {
#include "ev00bb/frog_action_1/dat_0cbb8ddc.inc"
};
static NJS_MODEL dat_0cbb8e54 = {
#include "ev00bb/frog_action_1/dat_0cbb8e54.inc"
};
static NJS_OBJECT dat_0cbb8e7c = {
#include "ev00bb/frog_action_1/dat_0cbb8e7c.inc"
};
static NJS_OBJECT dat_0cbb8eb0 = {
#include "ev00bb/frog_action_1/dat_0cbb8eb0.inc"
};
static NJS_OBJECT dat_0cbb8ee4 = {
#include "ev00bb/frog_action_1/dat_0cbb8ee4.inc"
};
static NJS_OBJECT dat_0cbb8f18 = {
#include "ev00bb/frog_action_1/dat_0cbb8f18.inc"
};
static NJS_OBJECT dat_0cbb8f4c = {
#include "ev00bb/frog_action_1/dat_0cbb8f4c.inc"
};
static NJS_MATERIAL dat_0cbb8f80[1] = {
#include "ev00bb/frog_action_1/dat_0cbb8f80.inc"
};
static Sint16 dat_0cbb8f94[18] = {
#include "ev00bb/frog_action_1/dat_0cbb8f94.inc"
};
static NJS_COLOR dat_0cbb8fb8[18] = {
#include "ev00bb/frog_action_1/dat_0cbb8fb8.inc"
};
static NJS_MESHSET dat_0cbb9000[1] = {
#include "ev00bb/frog_action_1/dat_0cbb9000.inc"
};
static NJS_POINT3 dat_0cbb9018[5] = {
#include "ev00bb/frog_action_1/dat_0cbb9018.inc"
};
static NJS_VECTOR dat_0cbb9054[5] = {
#include "ev00bb/frog_action_1/dat_0cbb9054.inc"
};
static NJS_MODEL dat_0cbb9090 = {
#include "ev00bb/frog_action_1/dat_0cbb9090.inc"
};
static NJS_OBJECT dat_0cbb90b8 = {
#include "ev00bb/frog_action_1/dat_0cbb90b8.inc"
};
static NJS_MATERIAL dat_0cbb90ec[1] = {
#include "ev00bb/frog_action_1/dat_0cbb90ec.inc"
};
static Sint16 dat_0cbb9100[30] = {
#include "ev00bb/frog_action_1/dat_0cbb9100.inc"
};
static Sint16 dat_0cbb913c[12] = {
#include "ev00bb/frog_action_1/dat_0cbb913c.inc"
};
static NJS_COLOR dat_0cbb9154[30] = {
#include "ev00bb/frog_action_1/dat_0cbb9154.inc"
};
static NJS_COLOR dat_0cbb91cc[12] = {
#include "ev00bb/frog_action_1/dat_0cbb91cc.inc"
};
static NJS_MESHSET dat_0cbb91fc[2] = {
#include "ev00bb/frog_action_1/dat_0cbb91fc.inc"
};
static NJS_POINT3 dat_0cbb922c[10] = {
#include "ev00bb/frog_action_1/dat_0cbb922c.inc"
};
static NJS_VECTOR dat_0cbb92a4[10] = {
#include "ev00bb/frog_action_1/dat_0cbb92a4.inc"
};
static NJS_MODEL dat_0cbb931c = {
#include "ev00bb/frog_action_1/dat_0cbb931c.inc"
};
static NJS_OBJECT dat_0cbb9344 = {
#include "ev00bb/frog_action_1/dat_0cbb9344.inc"
};
static NJS_OBJECT dat_0cbb9378 = {
#include "ev00bb/frog_action_1/dat_0cbb9378.inc"
};
static NJS_OBJECT dat_0cbb93ac = {
#include "ev00bb/frog_action_1/dat_0cbb93ac.inc"
};
static NJS_OBJECT dat_0cbb93e0 = {
#include "ev00bb/frog_action_1/dat_0cbb93e0.inc"
};
static NJS_OBJECT dat_0cbb9414 = {
#include "ev00bb/frog_action_1/dat_0cbb9414.inc"
};
static NJS_MATERIAL dat_0cbb9448[1] = {
#include "ev00bb/frog_action_1/dat_0cbb9448.inc"
};
static Sint16 dat_0cbb945c[24] = {
#include "ev00bb/frog_action_1/dat_0cbb945c.inc"
};
static NJS_COLOR dat_0cbb948c[24] = {
#include "ev00bb/frog_action_1/dat_0cbb948c.inc"
};
static NJS_MESHSET dat_0cbb94ec[1] = {
#include "ev00bb/frog_action_1/dat_0cbb94ec.inc"
};
static NJS_POINT3 dat_0cbb9504[6] = {
#include "ev00bb/frog_action_1/dat_0cbb9504.inc"
};
static NJS_VECTOR dat_0cbb954c[6] = {
#include "ev00bb/frog_action_1/dat_0cbb954c.inc"
};
static NJS_MODEL dat_0cbb9594 = {
#include "ev00bb/frog_action_1/dat_0cbb9594.inc"
};
static NJS_OBJECT dat_0cbb95bc = {
#include "ev00bb/frog_action_1/dat_0cbb95bc.inc"
};
static NJS_MATERIAL dat_0cbb95f0[1] = {
#include "ev00bb/frog_action_1/dat_0cbb95f0.inc"
};
static Sint16 dat_0cbb9604[24] = {
#include "ev00bb/frog_action_1/dat_0cbb9604.inc"
};
static NJS_COLOR dat_0cbb9634[24] = {
#include "ev00bb/frog_action_1/dat_0cbb9634.inc"
};
static NJS_MESHSET dat_0cbb9694[1] = {
#include "ev00bb/frog_action_1/dat_0cbb9694.inc"
};
static NJS_POINT3 dat_0cbb96ac[6] = {
#include "ev00bb/frog_action_1/dat_0cbb96ac.inc"
};
static NJS_VECTOR dat_0cbb96f4[6] = {
#include "ev00bb/frog_action_1/dat_0cbb96f4.inc"
};
static NJS_MODEL dat_0cbb973c = {
#include "ev00bb/frog_action_1/dat_0cbb973c.inc"
};
static NJS_OBJECT dat_0cbb9764 = {
#include "ev00bb/frog_action_1/dat_0cbb9764.inc"
};
static NJS_MATERIAL dat_0cbb9798[2] = {
#include "ev00bb/frog_action_1/dat_0cbb9798.inc"
};
static Sint16 dat_0cbb97c0[18] = {
#include "ev00bb/frog_action_1/dat_0cbb97c0.inc"
};
static Sint16 dat_0cbb97e4[6] = {
#include "ev00bb/frog_action_1/dat_0cbb97e4.inc"
};
static NJS_COLOR dat_0cbb97f0[18] = {
#include "ev00bb/frog_action_1/dat_0cbb97f0.inc"
};
static NJS_COLOR dat_0cbb9838[6] = {
#include "ev00bb/frog_action_1/dat_0cbb9838.inc"
};
static NJS_MESHSET dat_0cbb9850[2] = {
#include "ev00bb/frog_action_1/dat_0cbb9850.inc"
};
static NJS_POINT3 dat_0cbb9880[9] = {
#include "ev00bb/frog_action_1/dat_0cbb9880.inc"
};
static NJS_VECTOR dat_0cbb98ec[9] = {
#include "ev00bb/frog_action_1/dat_0cbb98ec.inc"
};
static NJS_MODEL dat_0cbb9958 = {
#include "ev00bb/frog_action_1/dat_0cbb9958.inc"
};
static NJS_OBJECT dat_0cbb9980 = {
#include "ev00bb/frog_action_1/dat_0cbb9980.inc"
};
static NJS_OBJECT dat_0cbb99b4 = {
#include "ev00bb/frog_action_1/dat_0cbb99b4.inc"
};
static NJS_OBJECT dat_0cbb99e8 = {
#include "ev00bb/frog_action_1/dat_0cbb99e8.inc"
};
static NJS_OBJECT dat_0cbb9a1c = {
#include "ev00bb/frog_action_1/dat_0cbb9a1c.inc"
};
static NJS_OBJECT dat_0cbb9a50 = {
#include "ev00bb/frog_action_1/dat_0cbb9a50.inc"
};
static NJS_OBJECT dat_0cbb9a84 = {
#include "ev00bb/frog_action_1/dat_0cbb9a84.inc"
};
static NJS_MATERIAL dat_0cbb9ab8[1] = {
#include "ev00bb/frog_action_1/dat_0cbb9ab8.inc"
};
static Sint16 dat_0cbb9acc[24] = {
#include "ev00bb/frog_action_1/dat_0cbb9acc.inc"
};
static NJS_COLOR dat_0cbb9afc[24] = {
#include "ev00bb/frog_action_1/dat_0cbb9afc.inc"
};
static NJS_MESHSET dat_0cbb9b5c[1] = {
#include "ev00bb/frog_action_1/dat_0cbb9b5c.inc"
};
static NJS_POINT3 dat_0cbb9b74[6] = {
#include "ev00bb/frog_action_1/dat_0cbb9b74.inc"
};
static NJS_VECTOR dat_0cbb9bbc[6] = {
#include "ev00bb/frog_action_1/dat_0cbb9bbc.inc"
};
static NJS_MODEL dat_0cbb9c04 = {
#include "ev00bb/frog_action_1/dat_0cbb9c04.inc"
};
static NJS_OBJECT dat_0cbb9c2c = {
#include "ev00bb/frog_action_1/dat_0cbb9c2c.inc"
};
static NJS_MATERIAL dat_0cbb9c60[1] = {
#include "ev00bb/frog_action_1/dat_0cbb9c60.inc"
};
static Sint16 dat_0cbb9c74[24] = {
#include "ev00bb/frog_action_1/dat_0cbb9c74.inc"
};
static NJS_COLOR dat_0cbb9ca4[24] = {
#include "ev00bb/frog_action_1/dat_0cbb9ca4.inc"
};
static NJS_MESHSET dat_0cbb9d04[1] = {
#include "ev00bb/frog_action_1/dat_0cbb9d04.inc"
};
static NJS_POINT3 dat_0cbb9d1c[6] = {
#include "ev00bb/frog_action_1/dat_0cbb9d1c.inc"
};
static NJS_VECTOR dat_0cbb9d64[6] = {
#include "ev00bb/frog_action_1/dat_0cbb9d64.inc"
};
static NJS_MODEL dat_0cbb9dac = {
#include "ev00bb/frog_action_1/dat_0cbb9dac.inc"
};
static NJS_OBJECT dat_0cbb9dd4 = {
#include "ev00bb/frog_action_1/dat_0cbb9dd4.inc"
};
static NJS_MATERIAL dat_0cbb9e08[2] = {
#include "ev00bb/frog_action_1/dat_0cbb9e08.inc"
};
static Sint16 dat_0cbb9e30[18] = {
#include "ev00bb/frog_action_1/dat_0cbb9e30.inc"
};
static Sint16 dat_0cbb9e54[6] = {
#include "ev00bb/frog_action_1/dat_0cbb9e54.inc"
};
static NJS_COLOR dat_0cbb9e60[18] = {
#include "ev00bb/frog_action_1/dat_0cbb9e60.inc"
};
static NJS_COLOR dat_0cbb9ea8[6] = {
#include "ev00bb/frog_action_1/dat_0cbb9ea8.inc"
};
static NJS_MESHSET dat_0cbb9ec0[2] = {
#include "ev00bb/frog_action_1/dat_0cbb9ec0.inc"
};
static NJS_POINT3 dat_0cbb9ef0[9] = {
#include "ev00bb/frog_action_1/dat_0cbb9ef0.inc"
};
static NJS_VECTOR dat_0cbb9f5c[9] = {
#include "ev00bb/frog_action_1/dat_0cbb9f5c.inc"
};
static NJS_MODEL dat_0cbb9fc8 = {
#include "ev00bb/frog_action_1/dat_0cbb9fc8.inc"
};
static NJS_OBJECT dat_0cbb9ff0 = {
#include "ev00bb/frog_action_1/dat_0cbb9ff0.inc"
};
static NJS_OBJECT dat_0cbba024 = {
#include "ev00bb/frog_action_1/dat_0cbba024.inc"
};
static NJS_OBJECT dat_0cbba058 = {
#include "ev00bb/frog_action_1/dat_0cbba058.inc"
};
static NJS_OBJECT dat_0cbba08c = {
#include "ev00bb/frog_action_1/dat_0cbba08c.inc"
};
static NJS_OBJECT dat_0cbba0c0 = {
#include "ev00bb/frog_action_1/dat_0cbba0c0.inc"
};
static NJS_OBJECT dat_0cbba0f4 = {
#include "ev00bb/frog_action_1/dat_0cbba0f4.inc"
};
static NJS_MATERIAL dat_0cbba128[1] = {
#include "ev00bb/frog_action_1/dat_0cbba128.inc"
};
static Sint16 dat_0cbba13c[4] = {
#include "ev00bb/frog_action_1/dat_0cbba13c.inc"
};
static NJS_COLOR dat_0cbba144[4] = {
#include "ev00bb/frog_action_1/dat_0cbba144.inc"
};
static NJS_MESHSET dat_0cbba154[1] = {
#include "ev00bb/frog_action_1/dat_0cbba154.inc"
};
static NJS_POINT3 dat_0cbba16c[4] = {
#include "ev00bb/frog_action_1/dat_0cbba16c.inc"
};
static NJS_VECTOR dat_0cbba19c[4] = {
#include "ev00bb/frog_action_1/dat_0cbba19c.inc"
};
static NJS_MODEL dat_0cbba1cc = {
#include "ev00bb/frog_action_1/dat_0cbba1cc.inc"
};
static NJS_OBJECT dat_0cbba1f4 = {
#include "ev00bb/frog_action_1/dat_0cbba1f4.inc"
};
static NJS_MATERIAL dat_0cbba228[5] = {
#include "ev00bb/frog_action_1/dat_0cbba228.inc"
};
static Sint16 dat_0cbba28c[42] = {
#include "ev00bb/frog_action_1/dat_0cbba28c.inc"
};
static Sint16 dat_0cbba2e0[36] = {
#include "ev00bb/frog_action_1/dat_0cbba2e0.inc"
};
static Sint16 dat_0cbba328[60] = {
#include "ev00bb/frog_action_1/dat_0cbba328.inc"
};
static Sint16 dat_0cbba3a0[8] = {
#include "ev00bb/frog_action_1/dat_0cbba3a0.inc"
};
static Sint16 dat_0cbba3b0[84] = {
#include "ev00bb/frog_action_1/dat_0cbba3b0.inc"
};
static Sint16 dat_0cbba458[8] = {
#include "ev00bb/frog_action_1/dat_0cbba458.inc"
};
static Sint16 dat_0cbba468[18] = {
#include "ev00bb/frog_action_1/dat_0cbba468.inc"
};
static Sint16 dat_0cbba48c[4] = {
#include "ev00bb/frog_action_1/dat_0cbba48c.inc"
};
static NJS_COLOR dat_0cbba494[42] = {
#include "ev00bb/frog_action_1/dat_0cbba494.inc"
};
static NJS_COLOR dat_0cbba53c[36] = {
#include "ev00bb/frog_action_1/dat_0cbba53c.inc"
};
static NJS_COLOR dat_0cbba5cc[60] = {
#include "ev00bb/frog_action_1/dat_0cbba5cc.inc"
};
static NJS_COLOR dat_0cbba6bc[8] = {
#include "ev00bb/frog_action_1/dat_0cbba6bc.inc"
};
static NJS_COLOR dat_0cbba6dc[84] = {
#include "ev00bb/frog_action_1/dat_0cbba6dc.inc"
};
static NJS_COLOR dat_0cbba82c[8] = {
#include "ev00bb/frog_action_1/dat_0cbba82c.inc"
};
static NJS_COLOR dat_0cbba84c[18] = {
#include "ev00bb/frog_action_1/dat_0cbba84c.inc"
};
static NJS_COLOR dat_0cbba894[4] = {
#include "ev00bb/frog_action_1/dat_0cbba894.inc"
};
static NJS_MESHSET dat_0cbba8a4[8] = {
#include "ev00bb/frog_action_1/dat_0cbba8a4.inc"
};
static NJS_POINT3 dat_0cbba964[52] = {
#include "ev00bb/frog_action_1/dat_0cbba964.inc"
};
static NJS_VECTOR dat_0cbbabd4[52] = {
#include "ev00bb/frog_action_1/dat_0cbbabd4.inc"
};
static NJS_MODEL dat_0cbbae44 = {
#include "ev00bb/frog_action_1/dat_0cbbae44.inc"
};
static NJS_OBJECT dat_0cbbae6c = {
#include "ev00bb/frog_action_1/dat_0cbbae6c.inc"
};
static NJS_OBJECT dat_0cbbaea0 = {
#include "ev00bb/frog_action_1/dat_0cbbaea0.inc"
};
static NJS_OBJECT dat_0cbbaed4 = {
#include "ev00bb/frog_action_1/dat_0cbbaed4.inc"
};
NJS_OBJECT ev00bb_frog_1_object = {
#include "ev00bb/frog_action_1/ev00bb_frog_1_object.inc"
};
static NJS_TEXNAME dat_0cbbaf3c[7] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0}, {TEXT_9, 0, 0},  {TEXT_10, 0, 0},
    {TEXT_11, 0, 0}, {TEXT_8, 0, 0}, {TEXT_12, 0, 0},
};
static NJS_TEXLIST dat_0cbbaf90 = {
#include "ev00bb/frog_action_1/dat_0cbbaf90.inc"
};
static NJS_OBJECT dat_0cbbaf98 = {
#include "ev00bb/frog_action_1/dat_0cbbaf98.inc"
};
static NJS_OBJECT dat_0cbbafcc = {
#include "ev00bb/frog_action_1/dat_0cbbafcc.inc"
};
static NJS_OBJECT dat_0cbbb000 = {
#include "ev00bb/frog_action_1/dat_0cbbb000.inc"
};
static NJS_OBJECT dat_0cbbb034 = {
#include "ev00bb/frog_action_1/dat_0cbbb034.inc"
};
static NJS_MATERIAL dat_0cbbb068[1] = {
#include "ev00bb/frog_action_1/dat_0cbbb068.inc"
};
static Sint16 dat_0cbbb07c[18] = {
#include "ev00bb/frog_action_1/dat_0cbbb07c.inc"
};
static NJS_COLOR dat_0cbbb0a0[18] = {
#include "ev00bb/frog_action_1/dat_0cbbb0a0.inc"
};
static NJS_MESHSET dat_0cbbb0e8[1] = {
#include "ev00bb/frog_action_1/dat_0cbbb0e8.inc"
};
static NJS_POINT3 dat_0cbbb100[5] = {
#include "ev00bb/frog_action_1/dat_0cbbb100.inc"
};
static NJS_VECTOR dat_0cbbb13c[5] = {
#include "ev00bb/frog_action_1/dat_0cbbb13c.inc"
};
static NJS_MODEL dat_0cbbb178 = {
#include "ev00bb/frog_action_1/dat_0cbbb178.inc"
};
static NJS_OBJECT dat_0cbbb1a0 = {
#include "ev00bb/frog_action_1/dat_0cbbb1a0.inc"
};
static NJS_MATERIAL dat_0cbbb1d4[1] = {
#include "ev00bb/frog_action_1/dat_0cbbb1d4.inc"
};
static Sint16 dat_0cbbb1e8[30] = {
#include "ev00bb/frog_action_1/dat_0cbbb1e8.inc"
};
static Sint16 dat_0cbbb224[12] = {
#include "ev00bb/frog_action_1/dat_0cbbb224.inc"
};
static NJS_COLOR dat_0cbbb23c[30] = {
#include "ev00bb/frog_action_1/dat_0cbbb23c.inc"
};
static NJS_COLOR dat_0cbbb2b4[12] = {
#include "ev00bb/frog_action_1/dat_0cbbb2b4.inc"
};
static NJS_MESHSET dat_0cbbb2e4[2] = {
#include "ev00bb/frog_action_1/dat_0cbbb2e4.inc"
};
static NJS_POINT3 dat_0cbbb314[10] = {
#include "ev00bb/frog_action_1/dat_0cbbb314.inc"
};
static NJS_VECTOR dat_0cbbb38c[10] = {
#include "ev00bb/frog_action_1/dat_0cbbb38c.inc"
};
static NJS_MODEL dat_0cbbb404 = {
#include "ev00bb/frog_action_1/dat_0cbbb404.inc"
};
static NJS_OBJECT dat_0cbbb42c = {
#include "ev00bb/frog_action_1/dat_0cbbb42c.inc"
};
static NJS_OBJECT dat_0cbbb460 = {
#include "ev00bb/frog_action_1/dat_0cbbb460.inc"
};
static NJS_OBJECT dat_0cbbb494 = {
#include "ev00bb/frog_action_1/dat_0cbbb494.inc"
};
static NJS_OBJECT dat_0cbbb4c8 = {
#include "ev00bb/frog_action_1/dat_0cbbb4c8.inc"
};
static NJS_OBJECT dat_0cbbb4fc = {
#include "ev00bb/frog_action_1/dat_0cbbb4fc.inc"
};
static NJS_MATERIAL dat_0cbbb530[1] = {
#include "ev00bb/frog_action_1/dat_0cbbb530.inc"
};
static Sint16 dat_0cbbb544[24] = {
#include "ev00bb/frog_action_1/dat_0cbbb544.inc"
};
static NJS_COLOR dat_0cbbb574[24] = {
#include "ev00bb/frog_action_1/dat_0cbbb574.inc"
};
static NJS_MESHSET dat_0cbbb5d4[1] = {
#include "ev00bb/frog_action_1/dat_0cbbb5d4.inc"
};
static NJS_POINT3 dat_0cbbb5ec[6] = {
#include "ev00bb/frog_action_1/dat_0cbbb5ec.inc"
};
static NJS_VECTOR dat_0cbbb634[6] = {
#include "ev00bb/frog_action_1/dat_0cbbb634.inc"
};
static NJS_MODEL dat_0cbbb67c = {
#include "ev00bb/frog_action_1/dat_0cbbb67c.inc"
};
static NJS_OBJECT dat_0cbbb6a4 = {
#include "ev00bb/frog_action_1/dat_0cbbb6a4.inc"
};
static NJS_MATERIAL dat_0cbbb6d8[1] = {
#include "ev00bb/frog_action_1/dat_0cbbb6d8.inc"
};
static Sint16 dat_0cbbb6ec[24] = {
#include "ev00bb/frog_action_1/dat_0cbbb6ec.inc"
};
static NJS_COLOR dat_0cbbb71c[24] = {
#include "ev00bb/frog_action_1/dat_0cbbb71c.inc"
};
static NJS_MESHSET dat_0cbbb77c[1] = {
#include "ev00bb/frog_action_1/dat_0cbbb77c.inc"
};
static NJS_POINT3 dat_0cbbb794[6] = {
#include "ev00bb/frog_action_1/dat_0cbbb794.inc"
};
static NJS_VECTOR dat_0cbbb7dc[6] = {
#include "ev00bb/frog_action_1/dat_0cbbb7dc.inc"
};
static NJS_MODEL dat_0cbbb824 = {
#include "ev00bb/frog_action_1/dat_0cbbb824.inc"
};
static NJS_OBJECT dat_0cbbb84c = {
#include "ev00bb/frog_action_1/dat_0cbbb84c.inc"
};
static NJS_MATERIAL dat_0cbbb880[2] = {
#include "ev00bb/frog_action_1/dat_0cbbb880.inc"
};
static Sint16 dat_0cbbb8a8[18] = {
#include "ev00bb/frog_action_1/dat_0cbbb8a8.inc"
};
static Sint16 dat_0cbbb8cc[6] = {
#include "ev00bb/frog_action_1/dat_0cbbb8cc.inc"
};
static NJS_COLOR dat_0cbbb8d8[18] = {
#include "ev00bb/frog_action_1/dat_0cbbb8d8.inc"
};
static NJS_COLOR dat_0cbbb920[6] = {
#include "ev00bb/frog_action_1/dat_0cbbb920.inc"
};
static NJS_MESHSET dat_0cbbb938[2] = {
#include "ev00bb/frog_action_1/dat_0cbbb938.inc"
};
static NJS_POINT3 dat_0cbbb968[9] = {
#include "ev00bb/frog_action_1/dat_0cbbb968.inc"
};
static NJS_VECTOR dat_0cbbb9d4[9] = {
#include "ev00bb/frog_action_1/dat_0cbbb9d4.inc"
};
static NJS_MODEL dat_0cbbba40 = {
#include "ev00bb/frog_action_1/dat_0cbbba40.inc"
};
static NJS_OBJECT dat_0cbbba68 = {
#include "ev00bb/frog_action_1/dat_0cbbba68.inc"
};
static NJS_OBJECT dat_0cbbba9c = {
#include "ev00bb/frog_action_1/dat_0cbbba9c.inc"
};
static NJS_OBJECT dat_0cbbbad0 = {
#include "ev00bb/frog_action_1/dat_0cbbbad0.inc"
};
static NJS_OBJECT dat_0cbbbb04 = {
#include "ev00bb/frog_action_1/dat_0cbbbb04.inc"
};
static NJS_OBJECT dat_0cbbbb38 = {
#include "ev00bb/frog_action_1/dat_0cbbbb38.inc"
};
static NJS_OBJECT dat_0cbbbb6c = {
#include "ev00bb/frog_action_1/dat_0cbbbb6c.inc"
};
static NJS_MATERIAL dat_0cbbbba0[1] = {
#include "ev00bb/frog_action_1/dat_0cbbbba0.inc"
};
static Sint16 dat_0cbbbbb4[24] = {
#include "ev00bb/frog_action_1/dat_0cbbbbb4.inc"
};
static NJS_COLOR dat_0cbbbbe4[24] = {
#include "ev00bb/frog_action_1/dat_0cbbbbe4.inc"
};
static NJS_MESHSET dat_0cbbbc44[1] = {
#include "ev00bb/frog_action_1/dat_0cbbbc44.inc"
};
static NJS_POINT3 dat_0cbbbc5c[6] = {
#include "ev00bb/frog_action_1/dat_0cbbbc5c.inc"
};
static NJS_VECTOR dat_0cbbbca4[6] = {
#include "ev00bb/frog_action_1/dat_0cbbbca4.inc"
};
static NJS_MODEL dat_0cbbbcec = {
#include "ev00bb/frog_action_1/dat_0cbbbcec.inc"
};
static NJS_OBJECT dat_0cbbbd14 = {
#include "ev00bb/frog_action_1/dat_0cbbbd14.inc"
};
static NJS_MATERIAL dat_0cbbbd48[1] = {
#include "ev00bb/frog_action_1/dat_0cbbbd48.inc"
};
static Sint16 dat_0cbbbd5c[24] = {
#include "ev00bb/frog_action_1/dat_0cbbbd5c.inc"
};
static NJS_COLOR dat_0cbbbd8c[24] = {
#include "ev00bb/frog_action_1/dat_0cbbbd8c.inc"
};
static NJS_MESHSET dat_0cbbbdec[1] = {
#include "ev00bb/frog_action_1/dat_0cbbbdec.inc"
};
static NJS_POINT3 dat_0cbbbe04[6] = {
#include "ev00bb/frog_action_1/dat_0cbbbe04.inc"
};
static NJS_VECTOR dat_0cbbbe4c[6] = {
#include "ev00bb/frog_action_1/dat_0cbbbe4c.inc"
};
static NJS_MODEL dat_0cbbbe94 = {
#include "ev00bb/frog_action_1/dat_0cbbbe94.inc"
};
static NJS_OBJECT dat_0cbbbebc = {
#include "ev00bb/frog_action_1/dat_0cbbbebc.inc"
};
static NJS_MATERIAL dat_0cbbbef0[2] = {
#include "ev00bb/frog_action_1/dat_0cbbbef0.inc"
};
static Sint16 dat_0cbbbf18[18] = {
#include "ev00bb/frog_action_1/dat_0cbbbf18.inc"
};
static Sint16 dat_0cbbbf3c[6] = {
#include "ev00bb/frog_action_1/dat_0cbbbf3c.inc"
};
static NJS_COLOR dat_0cbbbf48[18] = {
#include "ev00bb/frog_action_1/dat_0cbbbf48.inc"
};
static NJS_COLOR dat_0cbbbf90[6] = {
#include "ev00bb/frog_action_1/dat_0cbbbf90.inc"
};
static NJS_MESHSET dat_0cbbbfa8[2] = {
#include "ev00bb/frog_action_1/dat_0cbbbfa8.inc"
};
static NJS_POINT3 dat_0cbbbfd8[9] = {
#include "ev00bb/frog_action_1/dat_0cbbbfd8.inc"
};
static NJS_VECTOR dat_0cbbc044[9] = {
#include "ev00bb/frog_action_1/dat_0cbbc044.inc"
};
static NJS_MODEL dat_0cbbc0b0 = {
#include "ev00bb/frog_action_1/dat_0cbbc0b0.inc"
};
static NJS_OBJECT dat_0cbbc0d8 = {
#include "ev00bb/frog_action_1/dat_0cbbc0d8.inc"
};
static NJS_OBJECT dat_0cbbc10c = {
#include "ev00bb/frog_action_1/dat_0cbbc10c.inc"
};
static NJS_OBJECT dat_0cbbc140 = {
#include "ev00bb/frog_action_1/dat_0cbbc140.inc"
};
static NJS_OBJECT dat_0cbbc174 = {
#include "ev00bb/frog_action_1/dat_0cbbc174.inc"
};
static NJS_OBJECT dat_0cbbc1a8 = {
#include "ev00bb/frog_action_1/dat_0cbbc1a8.inc"
};
static NJS_OBJECT dat_0cbbc1dc = {
#include "ev00bb/frog_action_1/dat_0cbbc1dc.inc"
};
static NJS_MATERIAL dat_0cbbc210[1] = {
#include "ev00bb/frog_action_1/dat_0cbbc210.inc"
};
static Sint16 dat_0cbbc224[18] = {
#include "ev00bb/frog_action_1/dat_0cbbc224.inc"
};
static NJS_COLOR dat_0cbbc248[18] = {
#include "ev00bb/frog_action_1/dat_0cbbc248.inc"
};
static NJS_MESHSET dat_0cbbc290[1] = {
#include "ev00bb/frog_action_1/dat_0cbbc290.inc"
};
static NJS_POINT3 dat_0cbbc2a8[5] = {
#include "ev00bb/frog_action_1/dat_0cbbc2a8.inc"
};
static NJS_VECTOR dat_0cbbc2e4[5] = {
#include "ev00bb/frog_action_1/dat_0cbbc2e4.inc"
};
static NJS_MODEL dat_0cbbc320 = {
#include "ev00bb/frog_action_1/dat_0cbbc320.inc"
};
static NJS_OBJECT dat_0cbbc348 = {
#include "ev00bb/frog_action_1/dat_0cbbc348.inc"
};
static NJS_MATERIAL dat_0cbbc37c[1] = {
#include "ev00bb/frog_action_1/dat_0cbbc37c.inc"
};
static Sint16 dat_0cbbc390[30] = {
#include "ev00bb/frog_action_1/dat_0cbbc390.inc"
};
static Sint16 dat_0cbbc3cc[12] = {
#include "ev00bb/frog_action_1/dat_0cbbc3cc.inc"
};
static NJS_COLOR dat_0cbbc3e4[30] = {
#include "ev00bb/frog_action_1/dat_0cbbc3e4.inc"
};
static NJS_COLOR dat_0cbbc45c[12] = {
#include "ev00bb/frog_action_1/dat_0cbbc45c.inc"
};
static NJS_MESHSET dat_0cbbc48c[2] = {
#include "ev00bb/frog_action_1/dat_0cbbc48c.inc"
};
static NJS_POINT3 dat_0cbbc4bc[10] = {
#include "ev00bb/frog_action_1/dat_0cbbc4bc.inc"
};
static NJS_VECTOR dat_0cbbc534[10] = {
#include "ev00bb/frog_action_1/dat_0cbbc534.inc"
};
static NJS_MODEL dat_0cbbc5ac = {
#include "ev00bb/frog_action_1/dat_0cbbc5ac.inc"
};
static NJS_OBJECT dat_0cbbc5d4 = {
#include "ev00bb/frog_action_1/dat_0cbbc5d4.inc"
};
static NJS_OBJECT dat_0cbbc608 = {
#include "ev00bb/frog_action_1/dat_0cbbc608.inc"
};
static NJS_OBJECT dat_0cbbc63c = {
#include "ev00bb/frog_action_1/dat_0cbbc63c.inc"
};
static NJS_OBJECT dat_0cbbc670 = {
#include "ev00bb/frog_action_1/dat_0cbbc670.inc"
};
static NJS_OBJECT dat_0cbbc6a4 = {
#include "ev00bb/frog_action_1/dat_0cbbc6a4.inc"
};
static NJS_MATERIAL dat_0cbbc6d8[1] = {
#include "ev00bb/frog_action_1/dat_0cbbc6d8.inc"
};
static Sint16 dat_0cbbc6ec[4] = {
#include "ev00bb/frog_action_1/dat_0cbbc6ec.inc"
};
static NJS_COLOR dat_0cbbc6f4[4] = {
#include "ev00bb/frog_action_1/dat_0cbbc6f4.inc"
};
static NJS_MESHSET dat_0cbbc704[1] = {
#include "ev00bb/frog_action_1/dat_0cbbc704.inc"
};
static NJS_POINT3 dat_0cbbc71c[4] = {
#include "ev00bb/frog_action_1/dat_0cbbc71c.inc"
};
static NJS_VECTOR dat_0cbbc74c[4] = {
#include "ev00bb/frog_action_1/dat_0cbbc74c.inc"
};
static NJS_MODEL dat_0cbbc77c = {
#include "ev00bb/frog_action_1/dat_0cbbc77c.inc"
};
static NJS_OBJECT dat_0cbbc7a4 = {
#include "ev00bb/frog_action_1/dat_0cbbc7a4.inc"
};
static NJS_MATERIAL dat_0cbbc7d8[4] = {
#include "ev00bb/frog_action_1/dat_0cbbc7d8.inc"
};
static Sint16 dat_0cbbc828[48] = {
#include "ev00bb/frog_action_1/dat_0cbbc828.inc"
};
static Sint16 dat_0cbbc888[16] = {
#include "ev00bb/frog_action_1/dat_0cbbc888.inc"
};
static Sint16 dat_0cbbc8a8[126] = {
#include "ev00bb/frog_action_1/dat_0cbbc8a8.inc"
};
static Sint16 dat_0cbbc9a4[8] = {
#include "ev00bb/frog_action_1/dat_0cbbc9a4.inc"
};
static Sint16 dat_0cbbc9b4[18] = {
#include "ev00bb/frog_action_1/dat_0cbbc9b4.inc"
};
static Sint16 dat_0cbbc9d8[4] = {
#include "ev00bb/frog_action_1/dat_0cbbc9d8.inc"
};
static Sint16 dat_0cbbc9e0[36] = {
#include "ev00bb/frog_action_1/dat_0cbbc9e0.inc"
};
static NJS_COLOR dat_0cbbca28[48] = {
#include "ev00bb/frog_action_1/dat_0cbbca28.inc"
};
static NJS_COLOR dat_0cbbcae8[16] = {
#include "ev00bb/frog_action_1/dat_0cbbcae8.inc"
};
static NJS_COLOR dat_0cbbcb28[126] = {
#include "ev00bb/frog_action_1/dat_0cbbcb28.inc"
};
static NJS_COLOR dat_0cbbcd20[8] = {
#include "ev00bb/frog_action_1/dat_0cbbcd20.inc"
};
static NJS_COLOR dat_0cbbcd40[18] = {
#include "ev00bb/frog_action_1/dat_0cbbcd40.inc"
};
static NJS_COLOR dat_0cbbcd88[4] = {
#include "ev00bb/frog_action_1/dat_0cbbcd88.inc"
};
static NJS_COLOR dat_0cbbcd98[36] = {
#include "ev00bb/frog_action_1/dat_0cbbcd98.inc"
};
static NJS_MESHSET dat_0cbbce28[7] = {
#include "ev00bb/frog_action_1/dat_0cbbce28.inc"
};
static NJS_POINT3 dat_0cbbced0[52] = {
#include "ev00bb/frog_action_1/dat_0cbbced0.inc"
};
static NJS_VECTOR dat_0cbbd140[52] = {
#include "ev00bb/frog_action_1/dat_0cbbd140.inc"
};
static NJS_MODEL dat_0cbbd3b0 = {
#include "ev00bb/frog_action_1/dat_0cbbd3b0.inc"
};
static NJS_OBJECT dat_0cbbd3d8 = {
#include "ev00bb/frog_action_1/dat_0cbbd3d8.inc"
};
static NJS_OBJECT dat_0cbbd40c = {
#include "ev00bb/frog_action_1/dat_0cbbd40c.inc"
};
static NJS_OBJECT dat_0cbbd440 = {
#include "ev00bb/frog_action_1/dat_0cbbd440.inc"
};
NJS_OBJECT ev00bb_frog_2_object = {
#include "ev00bb/frog_action_1/ev00bb_frog_2_object.inc"
};
static NJS_TEXNAME dat_0cbbd4a8[8] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_14, 0, 0}, {TEXT_15, 0, 0},
    {TEXT_16, 0, 0}, {TEXT_13, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
};
