/* the root node of the bird's first model. its other 56 nodes are in the
 * data before this file. the model's 12 texture names and texlist,
 * ev0012_bird_texlist_1, follow. the event loads the texlist from
 * VER1_WING. ev0012_bird_action_1, the model's action, comes next with its
 * motion. the bird's second model, of 57 nodes, follows. the same 12 names
 * and ev0012_bird_texlist_2 come after it, and the event loads that texlist
 * from VER2_WING. the file ends on the motion of ev0012_bird_action_2, the
 * second model's action. the action starts the data after this file */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0012/bird_textures_1.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 9
#include "ev0012/bird_textures_1/text.inc"

extern NJS_OBJECT dat_0cbaada4;

static NJS_OBJECT dat_0cbaadd8 = {
#include "ev0012/bird_textures_1/dat_0cbaadd8.inc"
};
static NJS_TEXNAME dat_0cbaae0c[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0012_bird_texlist_1 = {
#include "ev0012/bird_textures_1/ev0012_bird_texlist_1.inc"
};
static NJS_MKEY_F dat_0cbaaea4[32] = {
#include "ev0012/bird_textures_1/dat_0cbaaea4.inc"
};
static NJS_MKEY_A dat_0cbab0a4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab0a4.inc"
};
static NJS_MKEY_F dat_0cbab0c4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab0c4.inc"
};
static NJS_MKEY_F dat_0cbab0e4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab0e4.inc"
};
static NJS_MKEY_A dat_0cbab104[2] = {
#include "ev0012/bird_textures_1/dat_0cbab104.inc"
};
static NJS_MKEY_A dat_0cbab124[32] = {
#include "ev0012/bird_textures_1/dat_0cbab124.inc"
};
static NJS_MKEY_F dat_0cbab324[2] = {
#include "ev0012/bird_textures_1/dat_0cbab324.inc"
};
static NJS_MKEY_F dat_0cbab344[2] = {
#include "ev0012/bird_textures_1/dat_0cbab344.inc"
};
static NJS_MKEY_A dat_0cbab364[2] = {
#include "ev0012/bird_textures_1/dat_0cbab364.inc"
};
static NJS_MKEY_A dat_0cbab384[32] = {
#include "ev0012/bird_textures_1/dat_0cbab384.inc"
};
static NJS_MKEY_F dat_0cbab584[2] = {
#include "ev0012/bird_textures_1/dat_0cbab584.inc"
};
static NJS_MKEY_F dat_0cbab5a4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab5a4.inc"
};
static NJS_MKEY_A dat_0cbab5c4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab5c4.inc"
};
static NJS_MKEY_A dat_0cbab5e4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab5e4.inc"
};
static NJS_MKEY_F dat_0cbab604[2] = {
#include "ev0012/bird_textures_1/dat_0cbab604.inc"
};
static NJS_MKEY_F dat_0cbab624[2] = {
#include "ev0012/bird_textures_1/dat_0cbab624.inc"
};
static NJS_MKEY_A dat_0cbab644[2] = {
#include "ev0012/bird_textures_1/dat_0cbab644.inc"
};
static NJS_MKEY_F dat_0cbab664[2] = {
#include "ev0012/bird_textures_1/dat_0cbab664.inc"
};
static NJS_MKEY_A dat_0cbab684[2] = {
#include "ev0012/bird_textures_1/dat_0cbab684.inc"
};
static NJS_MKEY_F dat_0cbab6a4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab6a4.inc"
};
static NJS_MKEY_A dat_0cbab6c4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab6c4.inc"
};
static NJS_MKEY_F dat_0cbab6e4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab6e4.inc"
};
static NJS_MKEY_A dat_0cbab704[2] = {
#include "ev0012/bird_textures_1/dat_0cbab704.inc"
};
static NJS_MKEY_A dat_0cbab724[32] = {
#include "ev0012/bird_textures_1/dat_0cbab724.inc"
};
static NJS_MKEY_F dat_0cbab924[2] = {
#include "ev0012/bird_textures_1/dat_0cbab924.inc"
};
static NJS_MKEY_F dat_0cbab944[2] = {
#include "ev0012/bird_textures_1/dat_0cbab944.inc"
};
static NJS_MKEY_A dat_0cbab964[2] = {
#include "ev0012/bird_textures_1/dat_0cbab964.inc"
};
static NJS_MKEY_F dat_0cbab984[2] = {
#include "ev0012/bird_textures_1/dat_0cbab984.inc"
};
static NJS_MKEY_A dat_0cbab9a4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab9a4.inc"
};
static NJS_MKEY_F dat_0cbab9c4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab9c4.inc"
};
static NJS_MKEY_A dat_0cbab9e4[2] = {
#include "ev0012/bird_textures_1/dat_0cbab9e4.inc"
};
static NJS_MKEY_F dat_0cbaba04[2] = {
#include "ev0012/bird_textures_1/dat_0cbaba04.inc"
};
static NJS_MKEY_A dat_0cbaba24[2] = {
#include "ev0012/bird_textures_1/dat_0cbaba24.inc"
};
static NJS_MKEY_F dat_0cbaba44[2] = {
#include "ev0012/bird_textures_1/dat_0cbaba44.inc"
};
static NJS_MKEY_A dat_0cbaba64[2] = {
#include "ev0012/bird_textures_1/dat_0cbaba64.inc"
};
static NJS_MKEY_A dat_0cbaba84[2] = {
#include "ev0012/bird_textures_1/dat_0cbaba84.inc"
};
static NJS_MKEY_F dat_0cbabaa4[2] = {
#include "ev0012/bird_textures_1/dat_0cbabaa4.inc"
};
static NJS_MKEY_F dat_0cbabac4[32] = {
#include "ev0012/bird_textures_1/dat_0cbabac4.inc"
};
static NJS_MKEY_A dat_0cbabcc4[2] = {
#include "ev0012/bird_textures_1/dat_0cbabcc4.inc"
};
static NJS_MKEY_F dat_0cbabce4[32] = {
#include "ev0012/bird_textures_1/dat_0cbabce4.inc"
};
static NJS_MKEY_A dat_0cbabee4[2] = {
#include "ev0012/bird_textures_1/dat_0cbabee4.inc"
};
static NJS_MKEY_F dat_0cbabf04[2] = {
#include "ev0012/bird_textures_1/dat_0cbabf04.inc"
};
static NJS_MKEY_A dat_0cbabf24[2] = {
#include "ev0012/bird_textures_1/dat_0cbabf24.inc"
};
static NJS_MKEY_F dat_0cbabf44[32] = {
#include "ev0012/bird_textures_1/dat_0cbabf44.inc"
};
static NJS_MKEY_A dat_0cbac144[2] = {
#include "ev0012/bird_textures_1/dat_0cbac144.inc"
};
static NJS_MKEY_F dat_0cbac164[2] = {
#include "ev0012/bird_textures_1/dat_0cbac164.inc"
};
static NJS_MKEY_A dat_0cbac184[2] = {
#include "ev0012/bird_textures_1/dat_0cbac184.inc"
};
static NJS_MKEY_F dat_0cbac1a4[2] = {
#include "ev0012/bird_textures_1/dat_0cbac1a4.inc"
};
static NJS_MKEY_A dat_0cbac1c4[2] = {
#include "ev0012/bird_textures_1/dat_0cbac1c4.inc"
};
static NJS_MKEY_F dat_0cbac1e4[2] = {
#include "ev0012/bird_textures_1/dat_0cbac1e4.inc"
};
static NJS_MKEY_A dat_0cbac204[2] = {
#include "ev0012/bird_textures_1/dat_0cbac204.inc"
};
static NJS_MKEY_F dat_0cbac224[2] = {
#include "ev0012/bird_textures_1/dat_0cbac224.inc"
};
static NJS_MKEY_F dat_0cbac244[2] = {
#include "ev0012/bird_textures_1/dat_0cbac244.inc"
};
static NJS_MKEY_F dat_0cbac264[2] = {
#include "ev0012/bird_textures_1/dat_0cbac264.inc"
};
static NJS_MKEY_A dat_0cbac284[2] = {
#include "ev0012/bird_textures_1/dat_0cbac284.inc"
};
static NJS_MKEY_F dat_0cbac2a4[2] = {
#include "ev0012/bird_textures_1/dat_0cbac2a4.inc"
};
static NJS_MKEY_A dat_0cbac2c4[2] = {
#include "ev0012/bird_textures_1/dat_0cbac2c4.inc"
};
static NJS_MKEY_F dat_0cbac2e4[2] = {
#include "ev0012/bird_textures_1/dat_0cbac2e4.inc"
};
static NJS_MDATA2 dat_0cbac304[57] = {
#include "ev0012/bird_textures_1/dat_0cbac304.inc"
};
static NJS_MOTION ev0012_bird_motion_1 = {
#include "ev0012/bird_textures_1/ev0012_bird_motion_1.inc"
};
NJS_ACTION ev0012_bird_action_1 = {
#include "ev0012/bird_textures_1/ev0012_bird_action_1.inc"
};
static NJS_MATERIAL dat_0cbac6a8[1] = {
#include "ev0012/bird_textures_1/dat_0cbac6a8.inc"
};
static Sint16 dat_0cbac6bc[6] = {
#include "ev0012/bird_textures_1/dat_0cbac6bc.inc"
};
static NJS_COLOR dat_0cbac6c8[4] = {
#include "ev0012/bird_textures_1/dat_0cbac6c8.inc"
};
static NJS_MESHSET dat_0cbac6d8[1] = {
#include "ev0012/bird_textures_1/dat_0cbac6d8.inc"
};
static NJS_POINT3 dat_0cbac6f0[4] = {
#include "ev0012/bird_textures_1/dat_0cbac6f0.inc"
};
static NJS_VECTOR dat_0cbac720[4] = {
#include "ev0012/bird_textures_1/dat_0cbac720.inc"
};
static NJS_MODEL dat_0cbac750 = {
#include "ev0012/bird_textures_1/dat_0cbac750.inc"
};
static NJS_OBJECT dat_0cbac778 = {
#include "ev0012/bird_textures_1/dat_0cbac778.inc"
};
static NJS_MATERIAL dat_0cbac7ac[1] = {
#include "ev0012/bird_textures_1/dat_0cbac7ac.inc"
};
static Sint16 dat_0cbac7c0[12] = {
#include "ev0012/bird_textures_1/dat_0cbac7c0.inc"
};
static NJS_COLOR dat_0cbac7d8[9] = {
#include "ev0012/bird_textures_1/dat_0cbac7d8.inc"
};
static NJS_MESHSET dat_0cbac7fc[1] = {
#include "ev0012/bird_textures_1/dat_0cbac7fc.inc"
};
static NJS_POINT3 dat_0cbac814[7] = {
#include "ev0012/bird_textures_1/dat_0cbac814.inc"
};
static NJS_VECTOR dat_0cbac868[7] = {
#include "ev0012/bird_textures_1/dat_0cbac868.inc"
};
static NJS_MODEL dat_0cbac8bc = {
#include "ev0012/bird_textures_1/dat_0cbac8bc.inc"
};
static NJS_OBJECT dat_0cbac8e4 = {
#include "ev0012/bird_textures_1/dat_0cbac8e4.inc"
};
static NJS_OBJECT dat_0cbac918 = {
#include "ev0012/bird_textures_1/dat_0cbac918.inc"
};
static NJS_OBJECT dat_0cbac94c = {
#include "ev0012/bird_textures_1/dat_0cbac94c.inc"
};
static NJS_OBJECT dat_0cbac980 = {
#include "ev0012/bird_textures_1/dat_0cbac980.inc"
};
static NJS_OBJECT dat_0cbac9b4 = {
#include "ev0012/bird_textures_1/dat_0cbac9b4.inc"
};
static NJS_OBJECT dat_0cbac9e8 = {
#include "ev0012/bird_textures_1/dat_0cbac9e8.inc"
};
static NJS_OBJECT dat_0cbaca1c = {
#include "ev0012/bird_textures_1/dat_0cbaca1c.inc"
};
static NJS_MATERIAL dat_0cbaca50[1] = {
#include "ev0012/bird_textures_1/dat_0cbaca50.inc"
};
static Sint16 dat_0cbaca64[6] = {
#include "ev0012/bird_textures_1/dat_0cbaca64.inc"
};
static NJS_COLOR dat_0cbaca70[4] = {
#include "ev0012/bird_textures_1/dat_0cbaca70.inc"
};
static NJS_MESHSET dat_0cbaca80[1] = {
#include "ev0012/bird_textures_1/dat_0cbaca80.inc"
};
static NJS_POINT3 dat_0cbaca98[4] = {
#include "ev0012/bird_textures_1/dat_0cbaca98.inc"
};
static NJS_VECTOR dat_0cbacac8[4] = {
#include "ev0012/bird_textures_1/dat_0cbacac8.inc"
};
static NJS_MODEL dat_0cbacaf8 = {
#include "ev0012/bird_textures_1/dat_0cbacaf8.inc"
};
static NJS_OBJECT dat_0cbacb20 = {
#include "ev0012/bird_textures_1/dat_0cbacb20.inc"
};
static NJS_MATERIAL dat_0cbacb54[1] = {
#include "ev0012/bird_textures_1/dat_0cbacb54.inc"
};
static Sint16 dat_0cbacb68[12] = {
#include "ev0012/bird_textures_1/dat_0cbacb68.inc"
};
static NJS_COLOR dat_0cbacb80[9] = {
#include "ev0012/bird_textures_1/dat_0cbacb80.inc"
};
static NJS_MESHSET dat_0cbacba4[1] = {
#include "ev0012/bird_textures_1/dat_0cbacba4.inc"
};
static NJS_POINT3 dat_0cbacbbc[7] = {
#include "ev0012/bird_textures_1/dat_0cbacbbc.inc"
};
static NJS_VECTOR dat_0cbacc10[7] = {
#include "ev0012/bird_textures_1/dat_0cbacc10.inc"
};
static NJS_MODEL dat_0cbacc64 = {
#include "ev0012/bird_textures_1/dat_0cbacc64.inc"
};
static NJS_OBJECT dat_0cbacc8c = {
#include "ev0012/bird_textures_1/dat_0cbacc8c.inc"
};
static NJS_OBJECT dat_0cbaccc0 = {
#include "ev0012/bird_textures_1/dat_0cbaccc0.inc"
};
static NJS_OBJECT dat_0cbaccf4 = {
#include "ev0012/bird_textures_1/dat_0cbaccf4.inc"
};
static NJS_OBJECT dat_0cbacd28 = {
#include "ev0012/bird_textures_1/dat_0cbacd28.inc"
};
static NJS_OBJECT dat_0cbacd5c = {
#include "ev0012/bird_textures_1/dat_0cbacd5c.inc"
};
static NJS_OBJECT dat_0cbacd90 = {
#include "ev0012/bird_textures_1/dat_0cbacd90.inc"
};
static NJS_OBJECT dat_0cbacdc4 = {
#include "ev0012/bird_textures_1/dat_0cbacdc4.inc"
};
static NJS_OBJECT dat_0cbacdf8 = {
#include "ev0012/bird_textures_1/dat_0cbacdf8.inc"
};
static NJS_OBJECT dat_0cbace2c = {
#include "ev0012/bird_textures_1/dat_0cbace2c.inc"
};
static NJS_OBJECT dat_0cbace60 = {
#include "ev0012/bird_textures_1/dat_0cbace60.inc"
};
static NJS_OBJECT dat_0cbace94 = {
#include "ev0012/bird_textures_1/dat_0cbace94.inc"
};
static NJS_OBJECT dat_0cbacec8 = {
#include "ev0012/bird_textures_1/dat_0cbacec8.inc"
};
static NJS_MATERIAL dat_0cbacefc[1] = {
#include "ev0012/bird_textures_1/dat_0cbacefc.inc"
};
static Sint16 dat_0cbacf10[18] = {
#include "ev0012/bird_textures_1/dat_0cbacf10.inc"
};
static NJS_COLOR dat_0cbacf34[15] = {
#include "ev0012/bird_textures_1/dat_0cbacf34.inc"
};
static NJS_MESHSET dat_0cbacf70[1] = {
#include "ev0012/bird_textures_1/dat_0cbacf70.inc"
};
static NJS_POINT3 dat_0cbacf88[7] = {
#include "ev0012/bird_textures_1/dat_0cbacf88.inc"
};
static NJS_VECTOR dat_0cbacfdc[7] = {
#include "ev0012/bird_textures_1/dat_0cbacfdc.inc"
};
static NJS_MODEL dat_0cbad030 = {
#include "ev0012/bird_textures_1/dat_0cbad030.inc"
};
static NJS_OBJECT dat_0cbad058 = {
#include "ev0012/bird_textures_1/dat_0cbad058.inc"
};
static NJS_OBJECT dat_0cbad08c = {
#include "ev0012/bird_textures_1/dat_0cbad08c.inc"
};
static NJS_OBJECT dat_0cbad0c0 = {
#include "ev0012/bird_textures_1/dat_0cbad0c0.inc"
};
static NJS_OBJECT dat_0cbad0f4 = {
#include "ev0012/bird_textures_1/dat_0cbad0f4.inc"
};
static NJS_MATERIAL dat_0cbad128[1] = {
#include "ev0012/bird_textures_1/dat_0cbad128.inc"
};
static Sint16 dat_0cbad13c[10] = {
#include "ev0012/bird_textures_1/dat_0cbad13c.inc"
};
static NJS_COLOR dat_0cbad150[8] = {
#include "ev0012/bird_textures_1/dat_0cbad150.inc"
};
static NJS_MESHSET dat_0cbad170[1] = {
#include "ev0012/bird_textures_1/dat_0cbad170.inc"
};
static NJS_POINT3 dat_0cbad188[8] = {
#include "ev0012/bird_textures_1/dat_0cbad188.inc"
};
static NJS_VECTOR dat_0cbad1e8[8] = {
#include "ev0012/bird_textures_1/dat_0cbad1e8.inc"
};
static NJS_MODEL dat_0cbad248 = {
#include "ev0012/bird_textures_1/dat_0cbad248.inc"
};
static NJS_OBJECT dat_0cbad270 = {
#include "ev0012/bird_textures_1/dat_0cbad270.inc"
};
static NJS_MATERIAL dat_0cbad2a4[1] = {
#include "ev0012/bird_textures_1/dat_0cbad2a4.inc"
};
static Sint16 dat_0cbad2b8[10] = {
#include "ev0012/bird_textures_1/dat_0cbad2b8.inc"
};
static NJS_COLOR dat_0cbad2cc[8] = {
#include "ev0012/bird_textures_1/dat_0cbad2cc.inc"
};
static NJS_MESHSET dat_0cbad2ec[1] = {
#include "ev0012/bird_textures_1/dat_0cbad2ec.inc"
};
static NJS_POINT3 dat_0cbad304[8] = {
#include "ev0012/bird_textures_1/dat_0cbad304.inc"
};
static NJS_VECTOR dat_0cbad364[8] = {
#include "ev0012/bird_textures_1/dat_0cbad364.inc"
};
static NJS_MODEL dat_0cbad3c4 = {
#include "ev0012/bird_textures_1/dat_0cbad3c4.inc"
};
static NJS_OBJECT dat_0cbad3ec = {
#include "ev0012/bird_textures_1/dat_0cbad3ec.inc"
};
static NJS_MATERIAL dat_0cbad420[1] = {
#include "ev0012/bird_textures_1/dat_0cbad420.inc"
};
static Sint16 dat_0cbad434[16] = {
#include "ev0012/bird_textures_1/dat_0cbad434.inc"
};
static NJS_COLOR dat_0cbad454[12] = {
#include "ev0012/bird_textures_1/dat_0cbad454.inc"
};
static NJS_MESHSET dat_0cbad484[1] = {
#include "ev0012/bird_textures_1/dat_0cbad484.inc"
};
static NJS_POINT3 dat_0cbad49c[12] = {
#include "ev0012/bird_textures_1/dat_0cbad49c.inc"
};
static NJS_VECTOR dat_0cbad52c[12] = {
#include "ev0012/bird_textures_1/dat_0cbad52c.inc"
};
static NJS_MODEL dat_0cbad5bc = {
#include "ev0012/bird_textures_1/dat_0cbad5bc.inc"
};
static NJS_OBJECT dat_0cbad5e4 = {
#include "ev0012/bird_textures_1/dat_0cbad5e4.inc"
};
static NJS_MATERIAL dat_0cbad618[1] = {
#include "ev0012/bird_textures_1/dat_0cbad618.inc"
};
static Sint16 dat_0cbad62c[16] = {
#include "ev0012/bird_textures_1/dat_0cbad62c.inc"
};
static NJS_COLOR dat_0cbad64c[12] = {
#include "ev0012/bird_textures_1/dat_0cbad64c.inc"
};
static NJS_MESHSET dat_0cbad67c[1] = {
#include "ev0012/bird_textures_1/dat_0cbad67c.inc"
};
static NJS_POINT3 dat_0cbad694[12] = {
#include "ev0012/bird_textures_1/dat_0cbad694.inc"
};
static NJS_VECTOR dat_0cbad724[12] = {
#include "ev0012/bird_textures_1/dat_0cbad724.inc"
};
static NJS_MODEL dat_0cbad7b4 = {
#include "ev0012/bird_textures_1/dat_0cbad7b4.inc"
};
static NJS_OBJECT dat_0cbad7dc = {
#include "ev0012/bird_textures_1/dat_0cbad7dc.inc"
};
static NJS_MATERIAL dat_0cbad810[4] = {
#include "ev0012/bird_textures_1/dat_0cbad810.inc"
};
static Sint16 dat_0cbad860[239] = {
#include "ev0012/bird_textures_1/dat_0cbad860.inc"
};
static Sint16 dat_0cbada3e[22] = {
#include "ev0012/bird_textures_1/dat_0cbada3e.inc"
};
static Sint16 dat_0cbada6a[21] = {
#include "ev0012/bird_textures_1/dat_0cbada6a.inc"
};
static Sint16 dat_0cbada94[22] = {
#include "ev0012/bird_textures_1/dat_0cbada94.inc"
};
static NJS_COLOR dat_0cbadac0[209] = {
#include "ev0012/bird_textures_1/dat_0cbadac0.inc"
};
static NJS_COLOR dat_0cbade04[20] = {
#include "ev0012/bird_textures_1/dat_0cbade04.inc"
};
static NJS_COLOR dat_0cbade54[16] = {
#include "ev0012/bird_textures_1/dat_0cbade54.inc"
};
static NJS_COLOR dat_0cbade94[18] = {
#include "ev0012/bird_textures_1/dat_0cbade94.inc"
};
static NJS_MESHSET dat_0cbadedc[4] = {
#include "ev0012/bird_textures_1/dat_0cbadedc.inc"
};
static NJS_POINT3 dat_0cbadf3c[127] = {
#include "ev0012/bird_textures_1/dat_0cbadf3c.inc"
};
static NJS_VECTOR dat_0cbae530[127] = {
#include "ev0012/bird_textures_1/dat_0cbae530.inc"
};
static NJS_MODEL dat_0cbaeb24 = {
#include "ev0012/bird_textures_1/dat_0cbaeb24.inc"
};
static NJS_OBJECT dat_0cbaeb4c = {
#include "ev0012/bird_textures_1/dat_0cbaeb4c.inc"
};
static NJS_OBJECT dat_0cbaeb80 = {
#include "ev0012/bird_textures_1/dat_0cbaeb80.inc"
};
static NJS_OBJECT dat_0cbaebb4 = {
#include "ev0012/bird_textures_1/dat_0cbaebb4.inc"
};
static NJS_OBJECT dat_0cbaebe8 = {
#include "ev0012/bird_textures_1/dat_0cbaebe8.inc"
};
static NJS_MATERIAL dat_0cbaec1c[1] = {
#include "ev0012/bird_textures_1/dat_0cbaec1c.inc"
};
static Sint16 dat_0cbaec30[8] = {
#include "ev0012/bird_textures_1/dat_0cbaec30.inc"
};
static NJS_COLOR dat_0cbaec40[6] = {
#include "ev0012/bird_textures_1/dat_0cbaec40.inc"
};
static NJS_MESHSET dat_0cbaec58[1] = {
#include "ev0012/bird_textures_1/dat_0cbaec58.inc"
};
static NJS_POINT3 dat_0cbaec70[6] = {
#include "ev0012/bird_textures_1/dat_0cbaec70.inc"
};
static NJS_VECTOR dat_0cbaecb8[6] = {
#include "ev0012/bird_textures_1/dat_0cbaecb8.inc"
};
static NJS_MODEL dat_0cbaed00 = {
#include "ev0012/bird_textures_1/dat_0cbaed00.inc"
};
static NJS_OBJECT dat_0cbaed28 = {
#include "ev0012/bird_textures_1/dat_0cbaed28.inc"
};
static NJS_MATERIAL dat_0cbaed5c[1] = {
#include "ev0012/bird_textures_1/dat_0cbaed5c.inc"
};
static Sint16 dat_0cbaed70[22] = {
#include "ev0012/bird_textures_1/dat_0cbaed70.inc"
};
static NJS_COLOR dat_0cbaed9c[16] = {
#include "ev0012/bird_textures_1/dat_0cbaed9c.inc"
};
static NJS_MESHSET dat_0cbaeddc[1] = {
#include "ev0012/bird_textures_1/dat_0cbaeddc.inc"
};
static NJS_POINT3 dat_0cbaedf4[5] = {
#include "ev0012/bird_textures_1/dat_0cbaedf4.inc"
};
static NJS_VECTOR dat_0cbaee30[5] = {
#include "ev0012/bird_textures_1/dat_0cbaee30.inc"
};
static NJS_MODEL dat_0cbaee6c = {
#include "ev0012/bird_textures_1/dat_0cbaee6c.inc"
};
static NJS_OBJECT dat_0cbaee94 = {
#include "ev0012/bird_textures_1/dat_0cbaee94.inc"
};
static NJS_MATERIAL dat_0cbaeec8[1] = {
#include "ev0012/bird_textures_1/dat_0cbaeec8.inc"
};
static Sint16 dat_0cbaeedc[30] = {
#include "ev0012/bird_textures_1/dat_0cbaeedc.inc"
};
static NJS_COLOR dat_0cbaef18[28] = {
#include "ev0012/bird_textures_1/dat_0cbaef18.inc"
};
static NJS_MESHSET dat_0cbaef88[1] = {
#include "ev0012/bird_textures_1/dat_0cbaef88.inc"
};
static NJS_POINT3 dat_0cbaefa0[18] = {
#include "ev0012/bird_textures_1/dat_0cbaefa0.inc"
};
static NJS_VECTOR dat_0cbaf078[18] = {
#include "ev0012/bird_textures_1/dat_0cbaf078.inc"
};
static NJS_MODEL dat_0cbaf150 = {
#include "ev0012/bird_textures_1/dat_0cbaf150.inc"
};
static NJS_OBJECT dat_0cbaf178 = {
#include "ev0012/bird_textures_1/dat_0cbaf178.inc"
};
static NJS_MATERIAL dat_0cbaf1ac[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf1ac.inc"
};
static Sint16 dat_0cbaf1c0[20] = {
#include "ev0012/bird_textures_1/dat_0cbaf1c0.inc"
};
static NJS_COLOR dat_0cbaf1e8[16] = {
#include "ev0012/bird_textures_1/dat_0cbaf1e8.inc"
};
static NJS_MESHSET dat_0cbaf228[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf228.inc"
};
static NJS_POINT3 dat_0cbaf240[12] = {
#include "ev0012/bird_textures_1/dat_0cbaf240.inc"
};
static NJS_VECTOR dat_0cbaf2d0[12] = {
#include "ev0012/bird_textures_1/dat_0cbaf2d0.inc"
};
static NJS_MODEL dat_0cbaf360 = {
#include "ev0012/bird_textures_1/dat_0cbaf360.inc"
};
static NJS_OBJECT dat_0cbaf388 = {
#include "ev0012/bird_textures_1/dat_0cbaf388.inc"
};
static NJS_MATERIAL dat_0cbaf3bc[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf3bc.inc"
};
static Sint16 dat_0cbaf3d0[10] = {
#include "ev0012/bird_textures_1/dat_0cbaf3d0.inc"
};
static NJS_COLOR dat_0cbaf3e4[8] = {
#include "ev0012/bird_textures_1/dat_0cbaf3e4.inc"
};
static NJS_MESHSET dat_0cbaf404[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf404.inc"
};
static NJS_POINT3 dat_0cbaf41c[5] = {
#include "ev0012/bird_textures_1/dat_0cbaf41c.inc"
};
static NJS_VECTOR dat_0cbaf458[5] = {
#include "ev0012/bird_textures_1/dat_0cbaf458.inc"
};
static NJS_MODEL dat_0cbaf494 = {
#include "ev0012/bird_textures_1/dat_0cbaf494.inc"
};
static NJS_OBJECT dat_0cbaf4bc = {
#include "ev0012/bird_textures_1/dat_0cbaf4bc.inc"
};
static NJS_MATERIAL dat_0cbaf4f0[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf4f0.inc"
};
static Sint16 dat_0cbaf504[10] = {
#include "ev0012/bird_textures_1/dat_0cbaf504.inc"
};
static NJS_COLOR dat_0cbaf518[8] = {
#include "ev0012/bird_textures_1/dat_0cbaf518.inc"
};
static NJS_MESHSET dat_0cbaf538[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf538.inc"
};
static NJS_POINT3 dat_0cbaf550[5] = {
#include "ev0012/bird_textures_1/dat_0cbaf550.inc"
};
static NJS_VECTOR dat_0cbaf58c[5] = {
#include "ev0012/bird_textures_1/dat_0cbaf58c.inc"
};
static NJS_MODEL dat_0cbaf5c8 = {
#include "ev0012/bird_textures_1/dat_0cbaf5c8.inc"
};
static NJS_OBJECT dat_0cbaf5f0 = {
#include "ev0012/bird_textures_1/dat_0cbaf5f0.inc"
};
static NJS_MATERIAL dat_0cbaf624[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf624.inc"
};
static Sint16 dat_0cbaf638[24] = {
#include "ev0012/bird_textures_1/dat_0cbaf638.inc"
};
static NJS_COLOR dat_0cbaf668[20] = {
#include "ev0012/bird_textures_1/dat_0cbaf668.inc"
};
static NJS_MESHSET dat_0cbaf6b8[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf6b8.inc"
};
static NJS_POINT3 dat_0cbaf6d0[12] = {
#include "ev0012/bird_textures_1/dat_0cbaf6d0.inc"
};
static NJS_VECTOR dat_0cbaf760[12] = {
#include "ev0012/bird_textures_1/dat_0cbaf760.inc"
};
static NJS_MODEL dat_0cbaf7f0 = {
#include "ev0012/bird_textures_1/dat_0cbaf7f0.inc"
};
static NJS_OBJECT dat_0cbaf818 = {
#include "ev0012/bird_textures_1/dat_0cbaf818.inc"
};
static NJS_MATERIAL dat_0cbaf84c[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf84c.inc"
};
static Sint16 dat_0cbaf860[36] = {
#include "ev0012/bird_textures_1/dat_0cbaf860.inc"
};
static NJS_COLOR dat_0cbaf8a8[30] = {
#include "ev0012/bird_textures_1/dat_0cbaf8a8.inc"
};
static NJS_MESHSET dat_0cbaf920[1] = {
#include "ev0012/bird_textures_1/dat_0cbaf920.inc"
};
static NJS_POINT3 dat_0cbaf938[12] = {
#include "ev0012/bird_textures_1/dat_0cbaf938.inc"
};
static NJS_VECTOR dat_0cbaf9c8[12] = {
#include "ev0012/bird_textures_1/dat_0cbaf9c8.inc"
};
static NJS_MODEL dat_0cbafa58 = {
#include "ev0012/bird_textures_1/dat_0cbafa58.inc"
};
static NJS_OBJECT dat_0cbafa80 = {
#include "ev0012/bird_textures_1/dat_0cbafa80.inc"
};
static NJS_OBJECT dat_0cbafab4 = {
#include "ev0012/bird_textures_1/dat_0cbafab4.inc"
};
static NJS_OBJECT dat_0cbafae8 = {
#include "ev0012/bird_textures_1/dat_0cbafae8.inc"
};
static NJS_OBJECT dat_0cbafb1c = {
#include "ev0012/bird_textures_1/dat_0cbafb1c.inc"
};
static NJS_MATERIAL dat_0cbafb50[1] = {
#include "ev0012/bird_textures_1/dat_0cbafb50.inc"
};
static Sint16 dat_0cbafb64[48] = {
#include "ev0012/bird_textures_1/dat_0cbafb64.inc"
};
static NJS_COLOR dat_0cbafbc4[38] = {
#include "ev0012/bird_textures_1/dat_0cbafbc4.inc"
};
static NJS_MESHSET dat_0cbafc5c[1] = {
#include "ev0012/bird_textures_1/dat_0cbafc5c.inc"
};
static NJS_POINT3 dat_0cbafc74[12] = {
#include "ev0012/bird_textures_1/dat_0cbafc74.inc"
};
static NJS_VECTOR dat_0cbafd04[12] = {
#include "ev0012/bird_textures_1/dat_0cbafd04.inc"
};
static NJS_MODEL dat_0cbafd94 = {
#include "ev0012/bird_textures_1/dat_0cbafd94.inc"
};
static NJS_OBJECT dat_0cbafdbc = {
#include "ev0012/bird_textures_1/dat_0cbafdbc.inc"
};
static NJS_OBJECT dat_0cbafdf0 = {
#include "ev0012/bird_textures_1/dat_0cbafdf0.inc"
};
static NJS_OBJECT dat_0cbafe24 = {
#include "ev0012/bird_textures_1/dat_0cbafe24.inc"
};
static NJS_OBJECT dat_0cbafe58 = {
#include "ev0012/bird_textures_1/dat_0cbafe58.inc"
};
static NJS_MATERIAL dat_0cbafe8c[1] = {
#include "ev0012/bird_textures_1/dat_0cbafe8c.inc"
};
static Sint16 dat_0cbafea0[48] = {
#include "ev0012/bird_textures_1/dat_0cbafea0.inc"
};
static NJS_COLOR dat_0cbaff00[38] = {
#include "ev0012/bird_textures_1/dat_0cbaff00.inc"
};
static NJS_MESHSET dat_0cbaff98[1] = {
#include "ev0012/bird_textures_1/dat_0cbaff98.inc"
};
static NJS_POINT3 dat_0cbaffb0[12] = {
#include "ev0012/bird_textures_1/dat_0cbaffb0.inc"
};
static NJS_VECTOR dat_0cbb0040[12] = {
#include "ev0012/bird_textures_1/dat_0cbb0040.inc"
};
static NJS_MODEL dat_0cbb00d0 = {
#include "ev0012/bird_textures_1/dat_0cbb00d0.inc"
};
static NJS_OBJECT dat_0cbb00f8 = {
#include "ev0012/bird_textures_1/dat_0cbb00f8.inc"
};
static NJS_OBJECT dat_0cbb012c = {
#include "ev0012/bird_textures_1/dat_0cbb012c.inc"
};
static NJS_OBJECT dat_0cbb0160 = {
#include "ev0012/bird_textures_1/dat_0cbb0160.inc"
};
static NJS_OBJECT dat_0cbb0194 = {
#include "ev0012/bird_textures_1/dat_0cbb0194.inc"
};
static NJS_MATERIAL dat_0cbb01c8[2] = {
#include "ev0012/bird_textures_1/dat_0cbb01c8.inc"
};
static Sint16 dat_0cbb01f0[54] = {
#include "ev0012/bird_textures_1/dat_0cbb01f0.inc"
};
static Sint16 dat_0cbb025c[22] = {
#include "ev0012/bird_textures_1/dat_0cbb025c.inc"
};
static NJS_COLOR dat_0cbb0288[48] = {
#include "ev0012/bird_textures_1/dat_0cbb0288.inc"
};
static NJS_COLOR dat_0cbb0348[18] = {
#include "ev0012/bird_textures_1/dat_0cbb0348.inc"
};
static NJS_MESHSET dat_0cbb0390[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0390.inc"
};
static NJS_POINT3 dat_0cbb03c0[26] = {
#include "ev0012/bird_textures_1/dat_0cbb03c0.inc"
};
static NJS_VECTOR dat_0cbb04f8[26] = {
#include "ev0012/bird_textures_1/dat_0cbb04f8.inc"
};
static NJS_MODEL dat_0cbb0630 = {
#include "ev0012/bird_textures_1/dat_0cbb0630.inc"
};
static NJS_OBJECT dat_0cbb0658 = {
#include "ev0012/bird_textures_1/dat_0cbb0658.inc"
};
static NJS_OBJECT dat_0cbb068c = {
#include "ev0012/bird_textures_1/dat_0cbb068c.inc"
};
static NJS_OBJECT dat_0cbb06c0 = {
#include "ev0012/bird_textures_1/dat_0cbb06c0.inc"
};
static NJS_OBJECT dat_0cbb06f4 = {
#include "ev0012/bird_textures_1/dat_0cbb06f4.inc"
};
NJS_OBJECT dat_0cbb0728 = {
#include "ev0012/bird_textures_1/dat_0cbb0728.inc"
};
static NJS_TEXNAME dat_0cbb075c[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0012_bird_texlist_2 = {
#include "ev0012/bird_textures_1/ev0012_bird_texlist_2.inc"
};
static NJS_MKEY_F dat_0cbb07f4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb07f4.inc"
};
static NJS_MKEY_F dat_0cbb0814[30] = {
#include "ev0012/bird_textures_1/dat_0cbb0814.inc"
};
static NJS_MKEY_F dat_0cbb09f4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb09f4.inc"
};
static NJS_MKEY_A dat_0cbb0a14[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0a14.inc"
};
static NJS_MKEY_F dat_0cbb0a34[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0a34.inc"
};
static NJS_MKEY_F dat_0cbb0a54[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0a54.inc"
};
static NJS_MKEY_F dat_0cbb0a74[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0a74.inc"
};
static NJS_MKEY_F dat_0cbb0a94[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0a94.inc"
};
static NJS_MKEY_F dat_0cbb0ab4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0ab4.inc"
};
static NJS_MKEY_A dat_0cbb0ad4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0ad4.inc"
};
static NJS_MKEY_F dat_0cbb0af4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0af4.inc"
};
static NJS_MKEY_A dat_0cbb0b14[30] = {
#include "ev0012/bird_textures_1/dat_0cbb0b14.inc"
};
static NJS_MKEY_F dat_0cbb0cf4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0cf4.inc"
};
static NJS_MKEY_F dat_0cbb0d14[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0d14.inc"
};
static NJS_MKEY_F dat_0cbb0d34[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0d34.inc"
};
static NJS_MKEY_F dat_0cbb0d54[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0d54.inc"
};
static NJS_MKEY_F dat_0cbb0d74[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0d74.inc"
};
static NJS_MKEY_A dat_0cbb0d94[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0d94.inc"
};
static NJS_MKEY_F dat_0cbb0db4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0db4.inc"
};
static NJS_MKEY_A dat_0cbb0dd4[30] = {
#include "ev0012/bird_textures_1/dat_0cbb0dd4.inc"
};
static NJS_MKEY_F dat_0cbb0fb4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0fb4.inc"
};
static NJS_MKEY_F dat_0cbb0fd4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0fd4.inc"
};
static NJS_MKEY_F dat_0cbb0ff4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb0ff4.inc"
};
static NJS_MKEY_F dat_0cbb1014[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1014.inc"
};
static NJS_MKEY_F dat_0cbb1034[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1034.inc"
};
static NJS_MKEY_A dat_0cbb1054[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1054.inc"
};
static NJS_MKEY_F dat_0cbb1074[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1074.inc"
};
static NJS_MKEY_A dat_0cbb1094[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1094.inc"
};
static NJS_MKEY_F dat_0cbb10b4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb10b4.inc"
};
static NJS_MKEY_F dat_0cbb10d4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb10d4.inc"
};
static NJS_MKEY_F dat_0cbb10f4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb10f4.inc"
};
static NJS_MKEY_F dat_0cbb1114[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1114.inc"
};
static NJS_MKEY_F dat_0cbb1134[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1134.inc"
};
static NJS_MKEY_F dat_0cbb1154[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1154.inc"
};
static NJS_MKEY_F dat_0cbb1174[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1174.inc"
};
static NJS_MKEY_F dat_0cbb1194[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1194.inc"
};
static NJS_MKEY_A dat_0cbb11b4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb11b4.inc"
};
static NJS_MKEY_F dat_0cbb11d4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb11d4.inc"
};
static NJS_MKEY_F dat_0cbb11f4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb11f4.inc"
};
static NJS_MKEY_A dat_0cbb1214[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1214.inc"
};
static NJS_MKEY_F dat_0cbb1234[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1234.inc"
};
static NJS_MKEY_A dat_0cbb1254[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1254.inc"
};
static NJS_MKEY_F dat_0cbb1274[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1274.inc"
};
static NJS_MKEY_A dat_0cbb1294[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1294.inc"
};
static NJS_MKEY_F dat_0cbb12b4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb12b4.inc"
};
static NJS_MKEY_A dat_0cbb12d4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb12d4.inc"
};
static NJS_MKEY_F dat_0cbb12f4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb12f4.inc"
};
static NJS_MKEY_F dat_0cbb1314[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1314.inc"
};
static NJS_MKEY_F dat_0cbb1334[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1334.inc"
};
static NJS_MKEY_F dat_0cbb1354[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1354.inc"
};
static NJS_MKEY_F dat_0cbb1374[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1374.inc"
};
static NJS_MKEY_A dat_0cbb1394[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1394.inc"
};
static NJS_MKEY_F dat_0cbb13b4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb13b4.inc"
};
static NJS_MKEY_F dat_0cbb13d4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb13d4.inc"
};
static NJS_MKEY_A dat_0cbb13f4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb13f4.inc"
};
static NJS_MKEY_F dat_0cbb1414[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1414.inc"
};
static NJS_MKEY_F dat_0cbb1434[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1434.inc"
};
static NJS_MKEY_A dat_0cbb1454[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1454.inc"
};
static NJS_MKEY_F dat_0cbb1474[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1474.inc"
};
static NJS_MKEY_F dat_0cbb1494[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1494.inc"
};
static NJS_MKEY_A dat_0cbb14b4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb14b4.inc"
};
static NJS_MKEY_F dat_0cbb14d4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb14d4.inc"
};
static NJS_MKEY_F dat_0cbb14f4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb14f4.inc"
};
static NJS_MKEY_A dat_0cbb1514[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1514.inc"
};
static NJS_MKEY_F dat_0cbb1534[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1534.inc"
};
static NJS_MKEY_A dat_0cbb1554[30] = {
#include "ev0012/bird_textures_1/dat_0cbb1554.inc"
};
static NJS_MKEY_F dat_0cbb1734[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1734.inc"
};
static NJS_MKEY_F dat_0cbb1754[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1754.inc"
};
static NJS_MKEY_F dat_0cbb1774[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1774.inc"
};
static NJS_MKEY_F dat_0cbb1794[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1794.inc"
};
static NJS_MKEY_F dat_0cbb17b4[30] = {
#include "ev0012/bird_textures_1/dat_0cbb17b4.inc"
};
static NJS_MKEY_A dat_0cbb1994[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1994.inc"
};
static NJS_MKEY_F dat_0cbb19b4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb19b4.inc"
};
static NJS_MKEY_F dat_0cbb19d4[30] = {
#include "ev0012/bird_textures_1/dat_0cbb19d4.inc"
};
static NJS_MKEY_A dat_0cbb1bb4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1bb4.inc"
};
static NJS_MKEY_F dat_0cbb1bd4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1bd4.inc"
};
static NJS_MKEY_F dat_0cbb1bf4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1bf4.inc"
};
static NJS_MKEY_A dat_0cbb1c14[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1c14.inc"
};
static NJS_MKEY_F dat_0cbb1c34[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1c34.inc"
};
static NJS_MKEY_F dat_0cbb1c54[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1c54.inc"
};
static NJS_MKEY_A dat_0cbb1c74[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1c74.inc"
};
static NJS_MKEY_F dat_0cbb1c94[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1c94.inc"
};
static NJS_MKEY_F dat_0cbb1cb4[30] = {
#include "ev0012/bird_textures_1/dat_0cbb1cb4.inc"
};
static NJS_MKEY_A dat_0cbb1e94[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1e94.inc"
};
static NJS_MKEY_F dat_0cbb1eb4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1eb4.inc"
};
static NJS_MKEY_F dat_0cbb1ed4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1ed4.inc"
};
static NJS_MKEY_A dat_0cbb1ef4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1ef4.inc"
};
static NJS_MKEY_F dat_0cbb1f14[2] = {
#include "ev0012/bird_textures_1/dat_0cbb1f14.inc"
};
static NJS_MKEY_A dat_0cbb1f34[30] = {
#include "ev0012/bird_textures_1/dat_0cbb1f34.inc"
};
static NJS_MKEY_F dat_0cbb2114[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2114.inc"
};
static NJS_MKEY_F dat_0cbb2134[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2134.inc"
};
static NJS_MKEY_F dat_0cbb2154[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2154.inc"
};
static NJS_MKEY_A dat_0cbb2174[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2174.inc"
};
static NJS_MKEY_F dat_0cbb2194[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2194.inc"
};
static NJS_MKEY_A dat_0cbb21b4[30] = {
#include "ev0012/bird_textures_1/dat_0cbb21b4.inc"
};
static NJS_MKEY_F dat_0cbb2394[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2394.inc"
};
static NJS_MKEY_F dat_0cbb23b4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb23b4.inc"
};
static NJS_MKEY_F dat_0cbb23d4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb23d4.inc"
};
static NJS_MKEY_F dat_0cbb23f4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb23f4.inc"
};
static NJS_MKEY_F dat_0cbb2414[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2414.inc"
};
static NJS_MKEY_F dat_0cbb2434[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2434.inc"
};
static NJS_MKEY_F dat_0cbb2454[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2454.inc"
};
static NJS_MKEY_A dat_0cbb2474[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2474.inc"
};
static NJS_MKEY_F dat_0cbb2494[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2494.inc"
};
static NJS_MKEY_A dat_0cbb24b4[30] = {
#include "ev0012/bird_textures_1/dat_0cbb24b4.inc"
};
static NJS_MKEY_F dat_0cbb2694[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2694.inc"
};
static NJS_MKEY_F dat_0cbb26b4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb26b4.inc"
};
static NJS_MKEY_F dat_0cbb26d4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb26d4.inc"
};
static NJS_MKEY_A dat_0cbb26f4[2] = {
#include "ev0012/bird_textures_1/dat_0cbb26f4.inc"
};
static NJS_MKEY_F dat_0cbb2714[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2714.inc"
};
static NJS_MKEY_A dat_0cbb2734[30] = {
#include "ev0012/bird_textures_1/dat_0cbb2734.inc"
};
static NJS_MKEY_F dat_0cbb2914[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2914.inc"
};
static NJS_MKEY_F dat_0cbb2934[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2934.inc"
};
static NJS_MKEY_F dat_0cbb2954[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2954.inc"
};
static NJS_MKEY_F dat_0cbb2974[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2974.inc"
};
static NJS_MKEY_F dat_0cbb2994[2] = {
#include "ev0012/bird_textures_1/dat_0cbb2994.inc"
};
static NJS_MDATA3 dat_0cbb29b4[57] = {
#include "ev0012/bird_textures_1/dat_0cbb29b4.inc"
};
NJS_MOTION dat_0cbb2f0c = {
#include "ev0012/bird_textures_1/dat_0cbb2f0c.inc"
};
