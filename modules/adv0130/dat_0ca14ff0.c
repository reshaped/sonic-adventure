/* the kurotama's object, the A-Life textures and their texlist, and the
 * monorail camera's action with its camera and motion. the models of the
 * booster, the laser and the long hammer follow, three level items whose root
 * objects 1ST_READ.BIN's level_item_models names by address. a list of six
 * textures and its texlist stands ahead of each of the first two models */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/dat_0ca14ff0.h"

/* a camera motion's key of one angle, its roll's or its angle of view's, at
 * keyframe */
struct angle_key {
    Uint32 keyframe;
    Angle key;
};

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 9
#include "adv0130/dat_0ca14ff0/text.inc"

/* the kurotama's model */
extern NJS_MODEL dat_0ca14fc8;

NJS_OBJECT kurotama_object = {
#include "adv0130/dat_0ca14ff0/kurotama_object.inc"
};
static NJS_TEXNAME dat_0ca15024[10] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0},
};
NJS_TEXLIST ec_alife_texlist = {
#include "adv0130/dat_0ca14ff0/ec_alife_texlist.inc"
};
static NJS_CAMERA dat_0ca150a4 = {
#include "adv0130/dat_0ca14ff0/dat_0ca150a4.inc"
};
/* the keys of the camera motion's four factors, in the order njCameraAction
 * reads them. the motion's type, 0x1c1, sets a bit for each: 0x1 for the
 * position, 0x40 for the point the camera looks at, 0x80 for the roll and
 * 0x100 for the angle of view. the first two take 200 keys of three floats
 * each, and the last two take 2 keys of one angle each */
static NJS_MKEY_F dat_0ca150e4[200] = {
#include "adv0130/dat_0ca14ff0/dat_0ca150e4.inc"
};
static NJS_MKEY_F dat_0ca15d64[200] = {
#include "adv0130/dat_0ca14ff0/dat_0ca15d64.inc"
};
static struct angle_key dat_0ca169e4[2] = {
#include "adv0130/dat_0ca14ff0/dat_0ca169e4.inc"
};
static struct angle_key dat_0ca169f4[2] = {
#include "adv0130/dat_0ca14ff0/dat_0ca169f4.inc"
};
static NJS_MDATA4 dat_0ca16a04[1] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16a04.inc"
};
static NJS_MOTION dat_0ca16a24 = {
#include "adv0130/dat_0ca14ff0/dat_0ca16a24.inc"
};
NJS_CACTION monorail_camera_action = {
#include "adv0130/dat_0ca14ff0/monorail_camera_action.inc"
};
static NJS_TEXNAME dat_0ca16a38[6] = {
    {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
    {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
};
static NJS_TEXLIST dat_0ca16a80 = {
#include "adv0130/dat_0ca14ff0/dat_0ca16a80.inc"
};
static NJS_MATERIAL dat_0ca16a88[6] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16a88.inc"
};
static Sint16 dat_0ca16b00[30] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16b00.inc"
};
static Sint16 dat_0ca16b3c[7] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16b3c.inc"
};
static Sint16 dat_0ca16b4a[15] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16b4a.inc"
};
static Sint16 dat_0ca16b68[15] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16b68.inc"
};
static Sint16 dat_0ca16b86[7] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16b86.inc"
};
static Sint16 dat_0ca16b94[8] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16b94.inc"
};
static NJS_COLOR dat_0ca16ba4[24] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16ba4.inc"
};
static NJS_COLOR dat_0ca16c04[6] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16c04.inc"
};
static NJS_COLOR dat_0ca16c1c[12] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16c1c.inc"
};
static NJS_COLOR dat_0ca16c4c[12] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16c4c.inc"
};
static NJS_COLOR dat_0ca16c7c[6] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16c7c.inc"
};
static NJS_COLOR dat_0ca16c94[6] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16c94.inc"
};
static NJS_MESHSET dat_0ca16cac[6] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16cac.inc"
};
static NJS_POINT3 dat_0ca16d3c[24] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16d3c.inc"
};
static NJS_VECTOR dat_0ca16e5c[24] = {
#include "adv0130/dat_0ca14ff0/dat_0ca16e5c.inc"
};
static NJS_MODEL dat_0ca16f7c = {
#include "adv0130/dat_0ca14ff0/dat_0ca16f7c.inc"
};
NJS_OBJECT level_item_booster_object = {
#include "adv0130/dat_0ca14ff0/level_item_booster_object.inc"
};
static NJS_TEXNAME dat_0ca16fd8[6] = {
    {TEXT_12, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
    {TEXT_19, 0, 0}, {TEXT_20, 0, 0}, {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0ca17020 = {
#include "adv0130/dat_0ca14ff0/dat_0ca17020.inc"
};
static NJS_MATERIAL dat_0ca17028[1] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17028.inc"
};
static Sint16 dat_0ca1703c[36] = {
#include "adv0130/dat_0ca14ff0/dat_0ca1703c.inc"
};
static NJS_COLOR dat_0ca17084[30] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17084.inc"
};
static NJS_MESHSET dat_0ca170fc[1] = {
#include "adv0130/dat_0ca14ff0/dat_0ca170fc.inc"
};
static NJS_POINT3 dat_0ca17114[18] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17114.inc"
};
static NJS_VECTOR dat_0ca171ec[18] = {
#include "adv0130/dat_0ca14ff0/dat_0ca171ec.inc"
};
static NJS_MODEL dat_0ca172c4 = {
#include "adv0130/dat_0ca14ff0/dat_0ca172c4.inc"
};
static NJS_OBJECT dat_0ca172ec = {
#include "adv0130/dat_0ca14ff0/dat_0ca172ec.inc"
};
static NJS_MATERIAL dat_0ca17320[7] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17320.inc"
};
static Sint16 dat_0ca173ac[7] = {
#include "adv0130/dat_0ca14ff0/dat_0ca173ac.inc"
};
static Sint16 dat_0ca173ba[63] = {
#include "adv0130/dat_0ca14ff0/dat_0ca173ba.inc"
};
static Sint16 dat_0ca17438[12] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17438.inc"
};
static Sint16 dat_0ca17450[15] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17450.inc"
};
static Sint16 dat_0ca1746e[10] = {
#include "adv0130/dat_0ca14ff0/dat_0ca1746e.inc"
};
static Sint16 dat_0ca17482[10] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17482.inc"
};
static Sint16 dat_0ca17496[51] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17496.inc"
};
static NJS_COLOR dat_0ca174fc[6] = {
#include "adv0130/dat_0ca14ff0/dat_0ca174fc.inc"
};
static NJS_COLOR dat_0ca17514[50] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17514.inc"
};
static NJS_COLOR dat_0ca175dc[10] = {
#include "adv0130/dat_0ca14ff0/dat_0ca175dc.inc"
};
static NJS_COLOR dat_0ca17604[12] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17604.inc"
};
static NJS_COLOR dat_0ca17634[8] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17634.inc"
};
static NJS_COLOR dat_0ca17654[40] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17654.inc"
};
static NJS_MESHSET dat_0ca176f4[7] = {
#include "adv0130/dat_0ca14ff0/dat_0ca176f4.inc"
};
static NJS_POINT3 dat_0ca1779c[48] = {
#include "adv0130/dat_0ca14ff0/dat_0ca1779c.inc"
};
static NJS_VECTOR dat_0ca179dc[48] = {
#include "adv0130/dat_0ca14ff0/dat_0ca179dc.inc"
};
static NJS_MODEL dat_0ca17c1c = {
#include "adv0130/dat_0ca14ff0/dat_0ca17c1c.inc"
};
NJS_OBJECT level_item_laser_object = {
#include "adv0130/dat_0ca14ff0/level_item_laser_object.inc"
};
static NJS_MATERIAL dat_0ca17c78[4] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17c78.inc"
};
static Sint16 dat_0ca17cc8[50] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17cc8.inc"
};
static Sint16 dat_0ca17d2c[25] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17d2c.inc"
};
static Sint16 dat_0ca17d5e[50] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17d5e.inc"
};
static Sint16 dat_0ca17dc2[13] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17dc2.inc"
};
static NJS_COLOR dat_0ca17ddc[40] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17ddc.inc"
};
static NJS_COLOR dat_0ca17e7c[20] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17e7c.inc"
};
static NJS_COLOR dat_0ca17ecc[40] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17ecc.inc"
};
static NJS_COLOR dat_0ca17f6c[10] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17f6c.inc"
};
static NJS_MESHSET dat_0ca17f94[4] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17f94.inc"
};
static NJS_POINT3 dat_0ca17ff4[30] = {
#include "adv0130/dat_0ca14ff0/dat_0ca17ff4.inc"
};
static NJS_VECTOR dat_0ca1815c[30] = {
#include "adv0130/dat_0ca14ff0/dat_0ca1815c.inc"
};
static NJS_MODEL dat_0ca182c4 = {
#include "adv0130/dat_0ca14ff0/dat_0ca182c4.inc"
};
static NJS_OBJECT dat_0ca182ec = {
#include "adv0130/dat_0ca14ff0/dat_0ca182ec.inc"
};
static NJS_MATERIAL dat_0ca18320[3] = {
#include "adv0130/dat_0ca14ff0/dat_0ca18320.inc"
};
static Sint16 dat_0ca1835c[20] = {
#include "adv0130/dat_0ca14ff0/dat_0ca1835c.inc"
};
static Sint16 dat_0ca18384[35] = {
#include "adv0130/dat_0ca14ff0/dat_0ca18384.inc"
};
static Sint16 dat_0ca183ca[11] = {
#include "adv0130/dat_0ca14ff0/dat_0ca183ca.inc"
};
static NJS_COLOR dat_0ca183e0[16] = {
#include "adv0130/dat_0ca14ff0/dat_0ca183e0.inc"
};
static NJS_COLOR dat_0ca18420[28] = {
#include "adv0130/dat_0ca14ff0/dat_0ca18420.inc"
};
static NJS_COLOR dat_0ca18490[10] = {
#include "adv0130/dat_0ca14ff0/dat_0ca18490.inc"
};
static NJS_MESHSET dat_0ca184b8[3] = {
#include "adv0130/dat_0ca14ff0/dat_0ca184b8.inc"
};
static NJS_POINT3 dat_0ca18500[24] = {
#include "adv0130/dat_0ca14ff0/dat_0ca18500.inc"
};
static NJS_VECTOR dat_0ca18620[24] = {
#include "adv0130/dat_0ca14ff0/dat_0ca18620.inc"
};
static NJS_MODEL dat_0ca18740 = {
#include "adv0130/dat_0ca14ff0/dat_0ca18740.inc"
};
NJS_OBJECT level_item_long_hammer_object = {
#include "adv0130/dat_0ca14ff0/level_item_long_hammer_object.inc"
};
