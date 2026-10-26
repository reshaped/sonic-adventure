/* EV0047, Tails' story: Tails takes off in the Tornado 2 to join Sonic */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/station_door.h"
#include "ev0047/tails_action_4.h"
#include "ev0047/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "shared/sprite.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 28
#define TEXT_2_LENGTH 55
#define TEXT_3_LENGTH 21
#define TEXT_4_LENGTH 18
#define TEXT_5_LENGTH 23
#define TEXT_6_LENGTH 21
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 13
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 13
#define TEXT_11_LENGTH 13
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 11
#define TEXT_20_LENGTH 11
#define TEXT_21_LENGTH 11
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 11
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 10
#define TEXT_34_LENGTH 10
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 10
#define TEXT_37_LENGTH 10
#define TEXT_38_LENGTH 10
#define TEXT_39_LENGTH 10
#define TEXT_40_LENGTH 8
#define TEXT_41_LENGTH 8
#define TEXT_42_LENGTH 8
#define TEXT_43_LENGTH 12
#define TEXT_44_LENGTH 12
#define TEXT_45_LENGTH 12
#define TEXT_46_LENGTH 13
#define TEXT_47_LENGTH 13
#define TEXT_48_LENGTH 3
#define TEXT_49_LENGTH 3
#define TEXT_50_LENGTH 2
#define TEXT_51_LENGTH 1
#define TEXT_52_LENGTH 4
#include "ev0047/script/text.inc"

/* @hack type: event_sprite_set_scale under a name of its own, declared without
 * a prototype. a declaration of event_sprite_set_scale itself would keep the
 * prototype shared/sprite.h gives it. ev0047_script's one call that passes
 * ints, (0, 0, 0), then passes them in R5 and R6, as the image does (0x0cb80bc0
 * to 0x0cb80bc8). by event_sprite_set_scale at every call, that call passes
 * 0.0F in FR4 and FR5, and 56 of ev0047_script's 1159 words differ. the two
 * calls that pass 1.8F take this name too, and load its address from the (0, 0,
 * 0) call's pool word at 0x0cb80c88. by event_sprite_set_scale, they load it
 * from a word of their own, the code runs 4 bytes long, and 96 of 1159
 * differ */
/* NOLINTNEXTLINE(clang-diagnostic-strict-prototypes) */
extern void ev0047_sprite_set_scale();

extern NJS_ACTION ev0047_tails_action_0;
extern NJS_ACTION ev0047_tails_action_1;
extern NJS_ACTION ev0047_tails_action_2;
extern NJS_ACTION ev0047_tails_action_3;
extern NJS_TEXLIST ev0047_emerald_texlist;
extern NJS_ACTION ev0047_emerald_action_0;
extern NJS_ACTION ev0047_emerald_action_1;
extern NJS_ACTION ev0047_emerald_action_2;
extern NJS_ACTION ev0047_emerald_action_3;
extern NJS_ACTION ev0047_emerald_action_4;

static struct event_path_point ev0047_tornado_rise_path_points[50] = {
#include "ev0047/script/ev0047_tornado_rise_path_points.inc"
};
static struct event_path ev0047_tornado_rise_path = {
    50, ev0047_tornado_rise_path_points};
static struct event_path_point ev0047_tornado_runway_path_points[45] = {
#include "ev0047/script/ev0047_tornado_runway_path_points.inc"
};
static struct event_path ev0047_tornado_runway_path = {
    45, ev0047_tornado_runway_path_points};
static struct event_path_point ev0047_tornado_takeoff_path_points[178] = {
#include "ev0047/script/ev0047_tornado_takeoff_path_points.inc"
};
static struct event_path ev0047_tornado_takeoff_path = {
    178, ev0047_tornado_takeoff_path_points};
static NJS_TEXNAME ev0047_tornado_textures[41] = {
    {TEXT_7, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},  {TEXT_10, 0, 0},
    {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
    {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
    {TEXT_19, 0, 0}, {TEXT_20, 0, 0}, {TEXT_21, 0, 0}, {TEXT_22, 0, 0},
    {TEXT_23, 0, 0}, {TEXT_24, 0, 0}, {TEXT_25, 0, 0}, {TEXT_26, 0, 0},
    {TEXT_27, 0, 0}, {TEXT_28, 0, 0}, {TEXT_29, 0, 0}, {TEXT_30, 0, 0},
    {TEXT_31, 0, 0}, {TEXT_32, 0, 0}, {TEXT_33, 0, 0}, {TEXT_34, 0, 0},
    {TEXT_35, 0, 0}, {TEXT_36, 0, 0}, {TEXT_37, 0, 0}, {TEXT_38, 0, 0},
    {TEXT_39, 0, 0}, {TEXT_40, 0, 0}, {TEXT_41, 0, 0}, {TEXT_42, 0, 0},
    {TEXT_43, 0, 0}, {TEXT_44, 0, 0}, {TEXT_45, 0, 0}, {TEXT_46, 0, 0},
    {TEXT_47, 0, 0},
};
static NJS_TEXLIST ev0047_tornado_texlist = {
#include "ev0047/script/ev0047_tornado_texlist.inc"
};
static NJS_OBJECT ev0047_tornado_77 = {
#include "ev0047/script/ev0047_tornado_77.inc"
};
static NJS_OBJECT ev0047_tornado_76 = {
#include "ev0047/script/ev0047_tornado_76.inc"
};
static NJS_OBJECT ev0047_tornado_75 = {
#include "ev0047/script/ev0047_tornado_75.inc"
};
static NJS_OBJECT ev0047_tornado_74 = {
#include "ev0047/script/ev0047_tornado_74.inc"
};
static NJS_MATERIAL ev0047_tornado_73_materials[2] = {
#include "ev0047/script/ev0047_tornado_73_materials.inc"
};
static Sint16 ev0047_tornado_73_meshes0[10] = {
#include "ev0047/script/ev0047_tornado_73_meshes0.inc"
};
static Sint16 ev0047_tornado_73_meshes1[10] = {
#include "ev0047/script/ev0047_tornado_73_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_73_uvs0[8] = {
#include "ev0047/script/ev0047_tornado_73_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_73_uvs1[8] = {
#include "ev0047/script/ev0047_tornado_73_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_73_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_73_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_73_points[13] = {
#include "ev0047/script/ev0047_tornado_73_points.inc"
};
static NJS_VECTOR ev0047_tornado_73_normals[13] = {
#include "ev0047/script/ev0047_tornado_73_normals.inc"
};
static NJS_MODEL ev0047_tornado_73_model = {
#include "ev0047/script/ev0047_tornado_73_model.inc"
};
static NJS_OBJECT ev0047_tornado_73 = {
#include "ev0047/script/ev0047_tornado_73.inc"
};
static NJS_MATERIAL ev0047_tornado_72_materials[2] = {
#include "ev0047/script/ev0047_tornado_72_materials.inc"
};
static Sint16 ev0047_tornado_72_meshes0[10] = {
#include "ev0047/script/ev0047_tornado_72_meshes0.inc"
};
static Sint16 ev0047_tornado_72_meshes1[10] = {
#include "ev0047/script/ev0047_tornado_72_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_72_uvs0[8] = {
#include "ev0047/script/ev0047_tornado_72_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_72_uvs1[8] = {
#include "ev0047/script/ev0047_tornado_72_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_72_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_72_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_72_points[13] = {
#include "ev0047/script/ev0047_tornado_72_points.inc"
};
static NJS_VECTOR ev0047_tornado_72_normals[13] = {
#include "ev0047/script/ev0047_tornado_72_normals.inc"
};
static NJS_MODEL ev0047_tornado_72_model = {
#include "ev0047/script/ev0047_tornado_72_model.inc"
};
static NJS_OBJECT ev0047_tornado_72 = {
#include "ev0047/script/ev0047_tornado_72.inc"
};
static NJS_MATERIAL ev0047_tornado_71_materials[3] = {
#include "ev0047/script/ev0047_tornado_71_materials.inc"
};
static Sint16 ev0047_tornado_71_meshes0[12] = {
#include "ev0047/script/ev0047_tornado_71_meshes0.inc"
};
static Sint16 ev0047_tornado_71_meshes1[39] = {
#include "ev0047/script/ev0047_tornado_71_meshes1.inc"
};
static Sint16 ev0047_tornado_71_meshes2[39] = {
#include "ev0047/script/ev0047_tornado_71_meshes2.inc"
};
static NJS_COLOR ev0047_tornado_71_uvs0[9] = {
#include "ev0047/script/ev0047_tornado_71_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_71_uvs1[34] = {
#include "ev0047/script/ev0047_tornado_71_uvs1.inc"
};
static NJS_COLOR ev0047_tornado_71_uvs2[34] = {
#include "ev0047/script/ev0047_tornado_71_uvs2.inc"
};
static NJS_MESHSET ev0047_tornado_71_meshsets[3] = {
#include "ev0047/script/ev0047_tornado_71_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_71_points[41] = {
#include "ev0047/script/ev0047_tornado_71_points.inc"
};
static NJS_VECTOR ev0047_tornado_71_normals[41] = {
#include "ev0047/script/ev0047_tornado_71_normals.inc"
};
static NJS_MODEL ev0047_tornado_71_model = {
#include "ev0047/script/ev0047_tornado_71_model.inc"
};
static NJS_OBJECT ev0047_tornado_71 = {
#include "ev0047/script/ev0047_tornado_71.inc"
};
static NJS_MATERIAL ev0047_tornado_70_materials[2] = {
#include "ev0047/script/ev0047_tornado_70_materials.inc"
};
static Sint16 ev0047_tornado_70_meshes0[10] = {
#include "ev0047/script/ev0047_tornado_70_meshes0.inc"
};
static Sint16 ev0047_tornado_70_meshes1[42] = {
#include "ev0047/script/ev0047_tornado_70_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_70_uvs0[8] = {
#include "ev0047/script/ev0047_tornado_70_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_70_uvs1[36] = {
#include "ev0047/script/ev0047_tornado_70_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_70_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_70_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_70_points[44] = {
#include "ev0047/script/ev0047_tornado_70_points.inc"
};
static NJS_VECTOR ev0047_tornado_70_normals[44] = {
#include "ev0047/script/ev0047_tornado_70_normals.inc"
};
static NJS_MODEL ev0047_tornado_70_model = {
#include "ev0047/script/ev0047_tornado_70_model.inc"
};
static NJS_OBJECT ev0047_tornado_70 = {
#include "ev0047/script/ev0047_tornado_70.inc"
};
static NJS_MATERIAL ev0047_tornado_69_materials[2] = {
#include "ev0047/script/ev0047_tornado_69_materials.inc"
};
static Sint16 ev0047_tornado_69_meshes0[79] = {
#include "ev0047/script/ev0047_tornado_69_meshes0.inc"
};
static Sint16 ev0047_tornado_69_meshes1[17] = {
#include "ev0047/script/ev0047_tornado_69_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_69_uvs0[72] = {
#include "ev0047/script/ev0047_tornado_69_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_69_uvs1[14] = {
#include "ev0047/script/ev0047_tornado_69_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_69_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_69_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_69_points[35] = {
#include "ev0047/script/ev0047_tornado_69_points.inc"
};
static NJS_VECTOR ev0047_tornado_69_normals[35] = {
#include "ev0047/script/ev0047_tornado_69_normals.inc"
};
static NJS_MODEL ev0047_tornado_69_model = {
#include "ev0047/script/ev0047_tornado_69_model.inc"
};
static NJS_OBJECT ev0047_tornado_69 = {
#include "ev0047/script/ev0047_tornado_69.inc"
};
static NJS_MATERIAL ev0047_tornado_68_materials[1] = {
#include "ev0047/script/ev0047_tornado_68_materials.inc"
};
static Sint16 ev0047_tornado_68_meshes0[6] = {
#include "ev0047/script/ev0047_tornado_68_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_68_uvs0[5] = {
#include "ev0047/script/ev0047_tornado_68_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_68_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_68_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_68_points[4] = {
#include "ev0047/script/ev0047_tornado_68_points.inc"
};
static NJS_VECTOR ev0047_tornado_68_normals[4] = {
#include "ev0047/script/ev0047_tornado_68_normals.inc"
};
static NJS_MODEL ev0047_tornado_68_model = {
#include "ev0047/script/ev0047_tornado_68_model.inc"
};
static NJS_OBJECT ev0047_tornado_68 = {
#include "ev0047/script/ev0047_tornado_68.inc"
};
static NJS_OBJECT ev0047_tornado_67 = {
#include "ev0047/script/ev0047_tornado_67.inc"
};
static NJS_OBJECT ev0047_tornado_66 = {
#include "ev0047/script/ev0047_tornado_66.inc"
};
static NJS_OBJECT ev0047_tornado_65 = {
#include "ev0047/script/ev0047_tornado_65.inc"
};
static NJS_MATERIAL ev0047_tornado_64_materials[1] = {
#include "ev0047/script/ev0047_tornado_64_materials.inc"
};
static Sint16 ev0047_tornado_64_meshes0[21] = {
#include "ev0047/script/ev0047_tornado_64_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_64_uvs0[18] = {
#include "ev0047/script/ev0047_tornado_64_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_64_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_64_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_64_points[8] = {
#include "ev0047/script/ev0047_tornado_64_points.inc"
};
static NJS_VECTOR ev0047_tornado_64_normals[8] = {
#include "ev0047/script/ev0047_tornado_64_normals.inc"
};
static NJS_MODEL ev0047_tornado_64_model = {
#include "ev0047/script/ev0047_tornado_64_model.inc"
};
static NJS_OBJECT ev0047_tornado_64 = {
#include "ev0047/script/ev0047_tornado_64.inc"
};
static NJS_MATERIAL ev0047_tornado_63_materials[2] = {
#include "ev0047/script/ev0047_tornado_63_materials.inc"
};
static Sint16 ev0047_tornado_63_meshes0[16] = {
#include "ev0047/script/ev0047_tornado_63_meshes0.inc"
};
static Sint16 ev0047_tornado_63_meshes1[21] = {
#include "ev0047/script/ev0047_tornado_63_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_63_uvs0[14] = {
#include "ev0047/script/ev0047_tornado_63_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_63_uvs1[18] = {
#include "ev0047/script/ev0047_tornado_63_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_63_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_63_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_63_points[16] = {
#include "ev0047/script/ev0047_tornado_63_points.inc"
};
static NJS_VECTOR ev0047_tornado_63_normals[16] = {
#include "ev0047/script/ev0047_tornado_63_normals.inc"
};
static NJS_MODEL ev0047_tornado_63_model = {
#include "ev0047/script/ev0047_tornado_63_model.inc"
};
static NJS_OBJECT ev0047_tornado_63 = {
#include "ev0047/script/ev0047_tornado_63.inc"
};
static NJS_MATERIAL ev0047_tornado_62_materials[1] = {
#include "ev0047/script/ev0047_tornado_62_materials.inc"
};
static Sint16 ev0047_tornado_62_meshes0[38] = {
#include "ev0047/script/ev0047_tornado_62_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_62_uvs0[32] = {
#include "ev0047/script/ev0047_tornado_62_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_62_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_62_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_62_points[12] = {
#include "ev0047/script/ev0047_tornado_62_points.inc"
};
static NJS_VECTOR ev0047_tornado_62_normals[12] = {
#include "ev0047/script/ev0047_tornado_62_normals.inc"
};
static NJS_MODEL ev0047_tornado_62_model = {
#include "ev0047/script/ev0047_tornado_62_model.inc"
};
static NJS_OBJECT ev0047_tornado_62 = {
#include "ev0047/script/ev0047_tornado_62.inc"
};
static NJS_OBJECT ev0047_tornado_61 = {
#include "ev0047/script/ev0047_tornado_61.inc"
};
static NJS_OBJECT ev0047_tornado_60 = {
#include "ev0047/script/ev0047_tornado_60.inc"
};
static NJS_OBJECT ev0047_tornado_59 = {
#include "ev0047/script/ev0047_tornado_59.inc"
};
static NJS_OBJECT ev0047_tornado_58 = {
#include "ev0047/script/ev0047_tornado_58.inc"
};
static NJS_OBJECT ev0047_tornado_57 = {
#include "ev0047/script/ev0047_tornado_57.inc"
};
static NJS_MATERIAL ev0047_tornado_56_materials[1] = {
#include "ev0047/script/ev0047_tornado_56_materials.inc"
};
static Sint16 ev0047_tornado_56_meshes0[21] = {
#include "ev0047/script/ev0047_tornado_56_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_56_uvs0[18] = {
#include "ev0047/script/ev0047_tornado_56_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_56_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_56_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_56_points[8] = {
#include "ev0047/script/ev0047_tornado_56_points.inc"
};
static NJS_VECTOR ev0047_tornado_56_normals[8] = {
#include "ev0047/script/ev0047_tornado_56_normals.inc"
};
static NJS_MODEL ev0047_tornado_56_model = {
#include "ev0047/script/ev0047_tornado_56_model.inc"
};
static NJS_OBJECT ev0047_tornado_56 = {
#include "ev0047/script/ev0047_tornado_56.inc"
};
static NJS_MATERIAL ev0047_tornado_55_materials[2] = {
#include "ev0047/script/ev0047_tornado_55_materials.inc"
};
static Sint16 ev0047_tornado_55_meshes0[19] = {
#include "ev0047/script/ev0047_tornado_55_meshes0.inc"
};
static Sint16 ev0047_tornado_55_meshes1[21] = {
#include "ev0047/script/ev0047_tornado_55_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_55_uvs0[16] = {
#include "ev0047/script/ev0047_tornado_55_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_55_uvs1[18] = {
#include "ev0047/script/ev0047_tornado_55_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_55_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_55_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_55_points[16] = {
#include "ev0047/script/ev0047_tornado_55_points.inc"
};
static NJS_VECTOR ev0047_tornado_55_normals[16] = {
#include "ev0047/script/ev0047_tornado_55_normals.inc"
};
static NJS_MODEL ev0047_tornado_55_model = {
#include "ev0047/script/ev0047_tornado_55_model.inc"
};
static NJS_OBJECT ev0047_tornado_55 = {
#include "ev0047/script/ev0047_tornado_55.inc"
};
static NJS_MATERIAL ev0047_tornado_54_materials[1] = {
#include "ev0047/script/ev0047_tornado_54_materials.inc"
};
static Sint16 ev0047_tornado_54_meshes0[32] = {
#include "ev0047/script/ev0047_tornado_54_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_54_uvs0[28] = {
#include "ev0047/script/ev0047_tornado_54_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_54_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_54_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_54_points[12] = {
#include "ev0047/script/ev0047_tornado_54_points.inc"
};
static NJS_VECTOR ev0047_tornado_54_normals[12] = {
#include "ev0047/script/ev0047_tornado_54_normals.inc"
};
static NJS_MODEL ev0047_tornado_54_model = {
#include "ev0047/script/ev0047_tornado_54_model.inc"
};
static NJS_OBJECT ev0047_tornado_54 = {
#include "ev0047/script/ev0047_tornado_54.inc"
};
static NJS_OBJECT ev0047_tornado_53 = {
#include "ev0047/script/ev0047_tornado_53.inc"
};
static NJS_OBJECT ev0047_tornado_52 = {
#include "ev0047/script/ev0047_tornado_52.inc"
};
static NJS_OBJECT ev0047_tornado_51 = {
#include "ev0047/script/ev0047_tornado_51.inc"
};
static NJS_OBJECT ev0047_tornado_50 = {
#include "ev0047/script/ev0047_tornado_50.inc"
};
static NJS_OBJECT ev0047_tornado_49 = {
#include "ev0047/script/ev0047_tornado_49.inc"
};
static NJS_MATERIAL ev0047_tornado_48_materials[1] = {
#include "ev0047/script/ev0047_tornado_48_materials.inc"
};
static Sint16 ev0047_tornado_48_meshes0[16] = {
#include "ev0047/script/ev0047_tornado_48_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_48_uvs0[14] = {
#include "ev0047/script/ev0047_tornado_48_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_48_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_48_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_48_points[8] = {
#include "ev0047/script/ev0047_tornado_48_points.inc"
};
static NJS_VECTOR ev0047_tornado_48_normals[8] = {
#include "ev0047/script/ev0047_tornado_48_normals.inc"
};
static NJS_MODEL ev0047_tornado_48_model = {
#include "ev0047/script/ev0047_tornado_48_model.inc"
};
static NJS_OBJECT ev0047_tornado_48 = {
#include "ev0047/script/ev0047_tornado_48.inc"
};
static NJS_MATERIAL ev0047_tornado_47_materials[2] = {
#include "ev0047/script/ev0047_tornado_47_materials.inc"
};
static Sint16 ev0047_tornado_47_meshes0[21] = {
#include "ev0047/script/ev0047_tornado_47_meshes0.inc"
};
static Sint16 ev0047_tornado_47_meshes1[16] = {
#include "ev0047/script/ev0047_tornado_47_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_47_uvs0[18] = {
#include "ev0047/script/ev0047_tornado_47_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_47_uvs1[14] = {
#include "ev0047/script/ev0047_tornado_47_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_47_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_47_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_47_points[16] = {
#include "ev0047/script/ev0047_tornado_47_points.inc"
};
static NJS_VECTOR ev0047_tornado_47_normals[16] = {
#include "ev0047/script/ev0047_tornado_47_normals.inc"
};
static NJS_MODEL ev0047_tornado_47_model = {
#include "ev0047/script/ev0047_tornado_47_model.inc"
};
static NJS_OBJECT ev0047_tornado_47 = {
#include "ev0047/script/ev0047_tornado_47.inc"
};
static NJS_MATERIAL ev0047_tornado_46_materials[5] = {
#include "ev0047/script/ev0047_tornado_46_materials.inc"
};
static Sint16 ev0047_tornado_46_meshes0[25] = {
#include "ev0047/script/ev0047_tornado_46_meshes0.inc"
};
static Sint16 ev0047_tornado_46_meshes1[44] = {
#include "ev0047/script/ev0047_tornado_46_meshes1.inc"
};
static Sint16 ev0047_tornado_46_meshes2[8] = {
#include "ev0047/script/ev0047_tornado_46_meshes2.inc"
};
static Sint16 ev0047_tornado_46_meshes3[5] = {
#include "ev0047/script/ev0047_tornado_46_meshes3.inc"
};
static Sint16 ev0047_tornado_46_meshes4[10] = {
#include "ev0047/script/ev0047_tornado_46_meshes4.inc"
};
static NJS_COLOR ev0047_tornado_46_uvs0[21] = {
#include "ev0047/script/ev0047_tornado_46_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_46_uvs1[38] = {
#include "ev0047/script/ev0047_tornado_46_uvs1.inc"
};
static NJS_COLOR ev0047_tornado_46_uvs2[7] = {
#include "ev0047/script/ev0047_tornado_46_uvs2.inc"
};
static NJS_COLOR ev0047_tornado_46_uvs3[4] = {
#include "ev0047/script/ev0047_tornado_46_uvs3.inc"
};
static NJS_COLOR ev0047_tornado_46_uvs4[8] = {
#include "ev0047/script/ev0047_tornado_46_uvs4.inc"
};
static NJS_MESHSET ev0047_tornado_46_meshsets[5] = {
#include "ev0047/script/ev0047_tornado_46_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_46_points[36] = {
#include "ev0047/script/ev0047_tornado_46_points.inc"
};
static NJS_VECTOR ev0047_tornado_46_normals[36] = {
#include "ev0047/script/ev0047_tornado_46_normals.inc"
};
static NJS_MODEL ev0047_tornado_46_model = {
#include "ev0047/script/ev0047_tornado_46_model.inc"
};
static NJS_OBJECT ev0047_tornado_46 = {
#include "ev0047/script/ev0047_tornado_46.inc"
};
static NJS_OBJECT ev0047_tornado_45 = {
#include "ev0047/script/ev0047_tornado_45.inc"
};
static NJS_OBJECT ev0047_tornado_44 = {
#include "ev0047/script/ev0047_tornado_44.inc"
};
static NJS_OBJECT ev0047_tornado_43 = {
#include "ev0047/script/ev0047_tornado_43.inc"
};
static NJS_OBJECT ev0047_tornado_42 = {
#include "ev0047/script/ev0047_tornado_42.inc"
};
static NJS_OBJECT ev0047_tornado_41 = {
#include "ev0047/script/ev0047_tornado_41.inc"
};
static NJS_MATERIAL ev0047_tornado_40_materials[1] = {
#include "ev0047/script/ev0047_tornado_40_materials.inc"
};
static Sint16 ev0047_tornado_40_meshes0[19] = {
#include "ev0047/script/ev0047_tornado_40_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_40_uvs0[16] = {
#include "ev0047/script/ev0047_tornado_40_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_40_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_40_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_40_points[8] = {
#include "ev0047/script/ev0047_tornado_40_points.inc"
};
static NJS_VECTOR ev0047_tornado_40_normals[8] = {
#include "ev0047/script/ev0047_tornado_40_normals.inc"
};
static NJS_MODEL ev0047_tornado_40_model = {
#include "ev0047/script/ev0047_tornado_40_model.inc"
};
static NJS_OBJECT ev0047_tornado_40 = {
#include "ev0047/script/ev0047_tornado_40.inc"
};
static NJS_MATERIAL ev0047_tornado_39_materials[2] = {
#include "ev0047/script/ev0047_tornado_39_materials.inc"
};
static Sint16 ev0047_tornado_39_meshes0[21] = {
#include "ev0047/script/ev0047_tornado_39_meshes0.inc"
};
static Sint16 ev0047_tornado_39_meshes1[19] = {
#include "ev0047/script/ev0047_tornado_39_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_39_uvs0[18] = {
#include "ev0047/script/ev0047_tornado_39_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_39_uvs1[16] = {
#include "ev0047/script/ev0047_tornado_39_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_39_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_39_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_39_points[16] = {
#include "ev0047/script/ev0047_tornado_39_points.inc"
};
static NJS_VECTOR ev0047_tornado_39_normals[16] = {
#include "ev0047/script/ev0047_tornado_39_normals.inc"
};
static NJS_MODEL ev0047_tornado_39_model = {
#include "ev0047/script/ev0047_tornado_39_model.inc"
};
static NJS_OBJECT ev0047_tornado_39 = {
#include "ev0047/script/ev0047_tornado_39.inc"
};
static NJS_MATERIAL ev0047_tornado_38_materials[5] = {
#include "ev0047/script/ev0047_tornado_38_materials.inc"
};
static Sint16 ev0047_tornado_38_meshes0[22] = {
#include "ev0047/script/ev0047_tornado_38_meshes0.inc"
};
static Sint16 ev0047_tornado_38_meshes1[44] = {
#include "ev0047/script/ev0047_tornado_38_meshes1.inc"
};
static Sint16 ev0047_tornado_38_meshes2[8] = {
#include "ev0047/script/ev0047_tornado_38_meshes2.inc"
};
static Sint16 ev0047_tornado_38_meshes3[5] = {
#include "ev0047/script/ev0047_tornado_38_meshes3.inc"
};
static Sint16 ev0047_tornado_38_meshes4[10] = {
#include "ev0047/script/ev0047_tornado_38_meshes4.inc"
};
static NJS_COLOR ev0047_tornado_38_uvs0[19] = {
#include "ev0047/script/ev0047_tornado_38_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_38_uvs1[38] = {
#include "ev0047/script/ev0047_tornado_38_uvs1.inc"
};
static NJS_COLOR ev0047_tornado_38_uvs2[7] = {
#include "ev0047/script/ev0047_tornado_38_uvs2.inc"
};
static NJS_COLOR ev0047_tornado_38_uvs3[4] = {
#include "ev0047/script/ev0047_tornado_38_uvs3.inc"
};
static NJS_COLOR ev0047_tornado_38_uvs4[8] = {
#include "ev0047/script/ev0047_tornado_38_uvs4.inc"
};
static NJS_MESHSET ev0047_tornado_38_meshsets[5] = {
#include "ev0047/script/ev0047_tornado_38_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_38_points[36] = {
#include "ev0047/script/ev0047_tornado_38_points.inc"
};
static NJS_VECTOR ev0047_tornado_38_normals[36] = {
#include "ev0047/script/ev0047_tornado_38_normals.inc"
};
static NJS_MODEL ev0047_tornado_38_model = {
#include "ev0047/script/ev0047_tornado_38_model.inc"
};
static NJS_OBJECT ev0047_tornado_38 = {
#include "ev0047/script/ev0047_tornado_38.inc"
};
static NJS_OBJECT ev0047_tornado_37 = {
#include "ev0047/script/ev0047_tornado_37.inc"
};
static NJS_OBJECT ev0047_tornado_36 = {
#include "ev0047/script/ev0047_tornado_36.inc"
};
static NJS_OBJECT ev0047_tornado_35 = {
#include "ev0047/script/ev0047_tornado_35.inc"
};
static NJS_OBJECT ev0047_tornado_34 = {
#include "ev0047/script/ev0047_tornado_34.inc"
};
static NJS_OBJECT ev0047_tornado_33 = {
#include "ev0047/script/ev0047_tornado_33.inc"
};
static NJS_MATERIAL ev0047_tornado_32_materials[5] = {
#include "ev0047/script/ev0047_tornado_32_materials.inc"
};
static Sint16 ev0047_tornado_32_meshes0[433] = {
#include "ev0047/script/ev0047_tornado_32_meshes0.inc"
};
static Sint16 ev0047_tornado_32_meshes1[35] = {
#include "ev0047/script/ev0047_tornado_32_meshes1.inc"
};
static Sint16 ev0047_tornado_32_meshes2[59] = {
#include "ev0047/script/ev0047_tornado_32_meshes2.inc"
};
static Sint16 ev0047_tornado_32_meshes3[21] = {
#include "ev0047/script/ev0047_tornado_32_meshes3.inc"
};
static Sint16 ev0047_tornado_32_meshes4[120] = {
#include "ev0047/script/ev0047_tornado_32_meshes4.inc"
};
static NJS_COLOR ev0047_tornado_32_uvs0[381] = {
#include "ev0047/script/ev0047_tornado_32_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_32_uvs1[31] = {
#include "ev0047/script/ev0047_tornado_32_uvs1.inc"
};
static NJS_COLOR ev0047_tornado_32_uvs2[52] = {
#include "ev0047/script/ev0047_tornado_32_uvs2.inc"
};
static NJS_COLOR ev0047_tornado_32_uvs3[18] = {
#include "ev0047/script/ev0047_tornado_32_uvs3.inc"
};
static NJS_COLOR ev0047_tornado_32_uvs4[104] = {
#include "ev0047/script/ev0047_tornado_32_uvs4.inc"
};
static NJS_MESHSET ev0047_tornado_32_meshsets[5] = {
#include "ev0047/script/ev0047_tornado_32_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_32_points[247] = {
#include "ev0047/script/ev0047_tornado_32_points.inc"
};
static NJS_VECTOR ev0047_tornado_32_normals[247] = {
#include "ev0047/script/ev0047_tornado_32_normals.inc"
};
static NJS_MODEL ev0047_tornado_32_model = {
#include "ev0047/script/ev0047_tornado_32_model.inc"
};
static NJS_OBJECT ev0047_tornado_32 = {
#include "ev0047/script/ev0047_tornado_32.inc"
};
static NJS_OBJECT ev0047_tornado_31 = {
#include "ev0047/script/ev0047_tornado_31.inc"
};
static NJS_OBJECT ev0047_tornado_30 = {
#include "ev0047/script/ev0047_tornado_30.inc"
};
static NJS_OBJECT ev0047_tornado_29 = {
#include "ev0047/script/ev0047_tornado_29.inc"
};
static NJS_OBJECT ev0047_tornado_28 = {
#include "ev0047/script/ev0047_tornado_28.inc"
};
static NJS_OBJECT ev0047_tornado_27 = {
#include "ev0047/script/ev0047_tornado_27.inc"
};
static NJS_OBJECT ev0047_tornado_26 = {
#include "ev0047/script/ev0047_tornado_26.inc"
};
static NJS_MATERIAL ev0047_tornado_25_materials[2] = {
#include "ev0047/script/ev0047_tornado_25_materials.inc"
};
static Sint16 ev0047_tornado_25_meshes0[44] = {
#include "ev0047/script/ev0047_tornado_25_meshes0.inc"
};
static Sint16 ev0047_tornado_25_meshes1[11] = {
#include "ev0047/script/ev0047_tornado_25_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_25_uvs0[40] = {
#include "ev0047/script/ev0047_tornado_25_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_25_uvs1[10] = {
#include "ev0047/script/ev0047_tornado_25_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_25_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_25_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_25_points[22] = {
#include "ev0047/script/ev0047_tornado_25_points.inc"
};
static NJS_VECTOR ev0047_tornado_25_normals[22] = {
#include "ev0047/script/ev0047_tornado_25_normals.inc"
};
static NJS_MODEL ev0047_tornado_25_model = {
#include "ev0047/script/ev0047_tornado_25_model.inc"
};
static NJS_OBJECT ev0047_tornado_25 = {
#include "ev0047/script/ev0047_tornado_25.inc"
};
static NJS_MATERIAL ev0047_tornado_24_materials[1] = {
#include "ev0047/script/ev0047_tornado_24_materials.inc"
};
static Sint16 ev0047_tornado_24_meshes0[21] = {
#include "ev0047/script/ev0047_tornado_24_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_24_uvs0[18] = {
#include "ev0047/script/ev0047_tornado_24_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_24_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_24_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_24_points[8] = {
#include "ev0047/script/ev0047_tornado_24_points.inc"
};
static NJS_VECTOR ev0047_tornado_24_normals[8] = {
#include "ev0047/script/ev0047_tornado_24_normals.inc"
};
static NJS_MODEL ev0047_tornado_24_model = {
#include "ev0047/script/ev0047_tornado_24_model.inc"
};
static NJS_OBJECT ev0047_tornado_24 = {
#include "ev0047/script/ev0047_tornado_24.inc"
};
static NJS_MATERIAL ev0047_tornado_23_materials[2] = {
#include "ev0047/script/ev0047_tornado_23_materials.inc"
};
static Sint16 ev0047_tornado_23_meshes0[16] = {
#include "ev0047/script/ev0047_tornado_23_meshes0.inc"
};
static Sint16 ev0047_tornado_23_meshes1[21] = {
#include "ev0047/script/ev0047_tornado_23_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_23_uvs0[14] = {
#include "ev0047/script/ev0047_tornado_23_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_23_uvs1[18] = {
#include "ev0047/script/ev0047_tornado_23_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_23_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_23_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_23_points[16] = {
#include "ev0047/script/ev0047_tornado_23_points.inc"
};
static NJS_VECTOR ev0047_tornado_23_normals[16] = {
#include "ev0047/script/ev0047_tornado_23_normals.inc"
};
static NJS_MODEL ev0047_tornado_23_model = {
#include "ev0047/script/ev0047_tornado_23_model.inc"
};
static NJS_OBJECT ev0047_tornado_23 = {
#include "ev0047/script/ev0047_tornado_23.inc"
};
static NJS_MATERIAL ev0047_tornado_22_materials[1] = {
#include "ev0047/script/ev0047_tornado_22_materials.inc"
};
static Sint16 ev0047_tornado_22_meshes0[32] = {
#include "ev0047/script/ev0047_tornado_22_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_22_uvs0[28] = {
#include "ev0047/script/ev0047_tornado_22_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_22_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_22_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_22_points[12] = {
#include "ev0047/script/ev0047_tornado_22_points.inc"
};
static NJS_VECTOR ev0047_tornado_22_normals[12] = {
#include "ev0047/script/ev0047_tornado_22_normals.inc"
};
static NJS_MODEL ev0047_tornado_22_model = {
#include "ev0047/script/ev0047_tornado_22_model.inc"
};
static NJS_OBJECT ev0047_tornado_22 = {
#include "ev0047/script/ev0047_tornado_22.inc"
};
static NJS_OBJECT ev0047_tornado_21 = {
#include "ev0047/script/ev0047_tornado_21.inc"
};
static NJS_OBJECT ev0047_tornado_20 = {
#include "ev0047/script/ev0047_tornado_20.inc"
};
static NJS_OBJECT ev0047_tornado_19 = {
#include "ev0047/script/ev0047_tornado_19.inc"
};
static NJS_OBJECT ev0047_tornado_18 = {
#include "ev0047/script/ev0047_tornado_18.inc"
};
static NJS_OBJECT ev0047_tornado_17 = {
#include "ev0047/script/ev0047_tornado_17.inc"
};
static NJS_MATERIAL ev0047_tornado_16_materials[1] = {
#include "ev0047/script/ev0047_tornado_16_materials.inc"
};
static Sint16 ev0047_tornado_16_meshes0[21] = {
#include "ev0047/script/ev0047_tornado_16_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_16_uvs0[18] = {
#include "ev0047/script/ev0047_tornado_16_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_16_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_16_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_16_points[8] = {
#include "ev0047/script/ev0047_tornado_16_points.inc"
};
static NJS_VECTOR ev0047_tornado_16_normals[8] = {
#include "ev0047/script/ev0047_tornado_16_normals.inc"
};
static NJS_MODEL ev0047_tornado_16_model = {
#include "ev0047/script/ev0047_tornado_16_model.inc"
};
static NJS_OBJECT ev0047_tornado_16 = {
#include "ev0047/script/ev0047_tornado_16.inc"
};
static NJS_MATERIAL ev0047_tornado_15_materials[2] = {
#include "ev0047/script/ev0047_tornado_15_materials.inc"
};
static Sint16 ev0047_tornado_15_meshes0[16] = {
#include "ev0047/script/ev0047_tornado_15_meshes0.inc"
};
static Sint16 ev0047_tornado_15_meshes1[21] = {
#include "ev0047/script/ev0047_tornado_15_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_15_uvs0[14] = {
#include "ev0047/script/ev0047_tornado_15_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_15_uvs1[18] = {
#include "ev0047/script/ev0047_tornado_15_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_15_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_15_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_15_points[16] = {
#include "ev0047/script/ev0047_tornado_15_points.inc"
};
static NJS_VECTOR ev0047_tornado_15_normals[16] = {
#include "ev0047/script/ev0047_tornado_15_normals.inc"
};
static NJS_MODEL ev0047_tornado_15_model = {
#include "ev0047/script/ev0047_tornado_15_model.inc"
};
static NJS_OBJECT ev0047_tornado_15 = {
#include "ev0047/script/ev0047_tornado_15.inc"
};
static NJS_MATERIAL ev0047_tornado_14_materials[1] = {
#include "ev0047/script/ev0047_tornado_14_materials.inc"
};
static Sint16 ev0047_tornado_14_meshes0[32] = {
#include "ev0047/script/ev0047_tornado_14_meshes0.inc"
};
static NJS_COLOR ev0047_tornado_14_uvs0[28] = {
#include "ev0047/script/ev0047_tornado_14_uvs0.inc"
};
static NJS_MESHSET ev0047_tornado_14_meshsets[1] = {
#include "ev0047/script/ev0047_tornado_14_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_14_points[12] = {
#include "ev0047/script/ev0047_tornado_14_points.inc"
};
static NJS_VECTOR ev0047_tornado_14_normals[12] = {
#include "ev0047/script/ev0047_tornado_14_normals.inc"
};
static NJS_MODEL ev0047_tornado_14_model = {
#include "ev0047/script/ev0047_tornado_14_model.inc"
};
static NJS_OBJECT ev0047_tornado_14 = {
#include "ev0047/script/ev0047_tornado_14.inc"
};
static NJS_OBJECT ev0047_tornado_13 = {
#include "ev0047/script/ev0047_tornado_13.inc"
};
static NJS_OBJECT ev0047_tornado_12 = {
#include "ev0047/script/ev0047_tornado_12.inc"
};
static NJS_OBJECT ev0047_tornado_11 = {
#include "ev0047/script/ev0047_tornado_11.inc"
};
static NJS_OBJECT ev0047_tornado_10 = {
#include "ev0047/script/ev0047_tornado_10.inc"
};
static NJS_OBJECT ev0047_tornado_9 = {
#include "ev0047/script/ev0047_tornado_9.inc"
};
static NJS_MATERIAL ev0047_tornado_8_materials[4] = {
#include "ev0047/script/ev0047_tornado_8_materials.inc"
};
static Sint16 ev0047_tornado_8_meshes0[18] = {
#include "ev0047/script/ev0047_tornado_8_meshes0.inc"
};
static Sint16 ev0047_tornado_8_meshes1[297] = {
#include "ev0047/script/ev0047_tornado_8_meshes1.inc"
};
static Sint16 ev0047_tornado_8_meshes2[204] = {
#include "ev0047/script/ev0047_tornado_8_meshes2.inc"
};
static Sint16 ev0047_tornado_8_meshes3[12] = {
#include "ev0047/script/ev0047_tornado_8_meshes3.inc"
};
static NJS_COLOR ev0047_tornado_8_uvs0[16] = {
#include "ev0047/script/ev0047_tornado_8_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_8_uvs1[259] = {
#include "ev0047/script/ev0047_tornado_8_uvs1.inc"
};
static NJS_COLOR ev0047_tornado_8_uvs2[172] = {
#include "ev0047/script/ev0047_tornado_8_uvs2.inc"
};
static NJS_COLOR ev0047_tornado_8_uvs3[10] = {
#include "ev0047/script/ev0047_tornado_8_uvs3.inc"
};
static NJS_MESHSET ev0047_tornado_8_meshsets[4] = {
#include "ev0047/script/ev0047_tornado_8_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_8_points[202] = {
#include "ev0047/script/ev0047_tornado_8_points.inc"
};
static NJS_VECTOR ev0047_tornado_8_normals[202] = {
#include "ev0047/script/ev0047_tornado_8_normals.inc"
};
static NJS_MODEL ev0047_tornado_8_model = {
#include "ev0047/script/ev0047_tornado_8_model.inc"
};
static NJS_OBJECT ev0047_tornado_8 = {
#include "ev0047/script/ev0047_tornado_8.inc"
};
static NJS_OBJECT ev0047_tornado_7 = {
#include "ev0047/script/ev0047_tornado_7.inc"
};
static NJS_OBJECT ev0047_tornado_6 = {
#include "ev0047/script/ev0047_tornado_6.inc"
};
static NJS_OBJECT ev0047_tornado_5 = {
#include "ev0047/script/ev0047_tornado_5.inc"
};
static NJS_OBJECT ev0047_tornado_4 = {
#include "ev0047/script/ev0047_tornado_4.inc"
};
static NJS_OBJECT ev0047_tornado_3 = {
#include "ev0047/script/ev0047_tornado_3.inc"
};
static NJS_OBJECT ev0047_tornado_2 = {
#include "ev0047/script/ev0047_tornado_2.inc"
};
static NJS_MATERIAL ev0047_tornado_1_materials[2] = {
#include "ev0047/script/ev0047_tornado_1_materials.inc"
};
static Sint16 ev0047_tornado_1_meshes0[93] = {
#include "ev0047/script/ev0047_tornado_1_meshes0.inc"
};
static Sint16 ev0047_tornado_1_meshes1[36] = {
#include "ev0047/script/ev0047_tornado_1_meshes1.inc"
};
static NJS_COLOR ev0047_tornado_1_uvs0[78] = {
#include "ev0047/script/ev0047_tornado_1_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_1_uvs1[32] = {
#include "ev0047/script/ev0047_tornado_1_uvs1.inc"
};
static NJS_MESHSET ev0047_tornado_1_meshsets[2] = {
#include "ev0047/script/ev0047_tornado_1_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_1_points[44] = {
#include "ev0047/script/ev0047_tornado_1_points.inc"
};
static NJS_VECTOR ev0047_tornado_1_normals[44] = {
#include "ev0047/script/ev0047_tornado_1_normals.inc"
};
static NJS_MODEL ev0047_tornado_1_model = {
#include "ev0047/script/ev0047_tornado_1_model.inc"
};
static NJS_OBJECT ev0047_tornado_1 = {
#include "ev0047/script/ev0047_tornado_1.inc"
};
static NJS_MATERIAL ev0047_tornado_0_materials[19] = {
#include "ev0047/script/ev0047_tornado_0_materials.inc"
};
static Sint16 ev0047_tornado_0_meshes0[204] = {
#include "ev0047/script/ev0047_tornado_0_meshes0.inc"
};
static Sint16 ev0047_tornado_0_meshes1[11] = {
#include "ev0047/script/ev0047_tornado_0_meshes1.inc"
};
static Sint16 ev0047_tornado_0_meshes2[15] = {
#include "ev0047/script/ev0047_tornado_0_meshes2.inc"
};
static Sint16 ev0047_tornado_0_meshes3[5] = {
#include "ev0047/script/ev0047_tornado_0_meshes3.inc"
};
static Sint16 ev0047_tornado_0_meshes4[32] = {
#include "ev0047/script/ev0047_tornado_0_meshes4.inc"
};
static Sint16 ev0047_tornado_0_meshes5[28] = {
#include "ev0047/script/ev0047_tornado_0_meshes5.inc"
};
static Sint16 ev0047_tornado_0_meshes6[28] = {
#include "ev0047/script/ev0047_tornado_0_meshes6.inc"
};
static Sint16 ev0047_tornado_0_meshes7[30] = {
#include "ev0047/script/ev0047_tornado_0_meshes7.inc"
};
static Sint16 ev0047_tornado_0_meshes8[83] = {
#include "ev0047/script/ev0047_tornado_0_meshes8.inc"
};
static Sint16 ev0047_tornado_0_meshes9[10] = {
#include "ev0047/script/ev0047_tornado_0_meshes9.inc"
};
static Sint16 ev0047_tornado_0_meshes10[10] = {
#include "ev0047/script/ev0047_tornado_0_meshes10.inc"
};
static Sint16 ev0047_tornado_0_meshes11[12] = {
#include "ev0047/script/ev0047_tornado_0_meshes11.inc"
};
static Sint16 ev0047_tornado_0_meshes12[98] = {
#include "ev0047/script/ev0047_tornado_0_meshes12.inc"
};
static Sint16 ev0047_tornado_0_meshes13[48] = {
#include "ev0047/script/ev0047_tornado_0_meshes13.inc"
};
static Sint16 ev0047_tornado_0_meshes14[48] = {
#include "ev0047/script/ev0047_tornado_0_meshes14.inc"
};
static Sint16 ev0047_tornado_0_meshes15[15] = {
#include "ev0047/script/ev0047_tornado_0_meshes15.inc"
};
static Sint16 ev0047_tornado_0_meshes16[9] = {
#include "ev0047/script/ev0047_tornado_0_meshes16.inc"
};
static Sint16 ev0047_tornado_0_meshes17[16] = {
#include "ev0047/script/ev0047_tornado_0_meshes17.inc"
};
static Sint16 ev0047_tornado_0_meshes18[6] = {
#include "ev0047/script/ev0047_tornado_0_meshes18.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs0[180] = {
#include "ev0047/script/ev0047_tornado_0_uvs0.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs1[10] = {
#include "ev0047/script/ev0047_tornado_0_uvs1.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs2[12] = {
#include "ev0047/script/ev0047_tornado_0_uvs2.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs3[4] = {
#include "ev0047/script/ev0047_tornado_0_uvs3.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs4[25] = {
#include "ev0047/script/ev0047_tornado_0_uvs4.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs5[22] = {
#include "ev0047/script/ev0047_tornado_0_uvs5.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs6[22] = {
#include "ev0047/script/ev0047_tornado_0_uvs6.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs7[25] = {
#include "ev0047/script/ev0047_tornado_0_uvs7.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs8[68] = {
#include "ev0047/script/ev0047_tornado_0_uvs8.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs9[8] = {
#include "ev0047/script/ev0047_tornado_0_uvs9.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs10[8] = {
#include "ev0047/script/ev0047_tornado_0_uvs10.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs11[10] = {
#include "ev0047/script/ev0047_tornado_0_uvs11.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs12[84] = {
#include "ev0047/script/ev0047_tornado_0_uvs12.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs13[40] = {
#include "ev0047/script/ev0047_tornado_0_uvs13.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs14[40] = {
#include "ev0047/script/ev0047_tornado_0_uvs14.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs15[13] = {
#include "ev0047/script/ev0047_tornado_0_uvs15.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs16[8] = {
#include "ev0047/script/ev0047_tornado_0_uvs16.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs17[14] = {
#include "ev0047/script/ev0047_tornado_0_uvs17.inc"
};
static NJS_COLOR ev0047_tornado_0_uvs18[5] = {
#include "ev0047/script/ev0047_tornado_0_uvs18.inc"
};
static NJS_MESHSET ev0047_tornado_0_meshsets[19] = {
#include "ev0047/script/ev0047_tornado_0_meshsets.inc"
};
static NJS_POINT3 ev0047_tornado_0_points[211] = {
#include "ev0047/script/ev0047_tornado_0_points.inc"
};
static NJS_VECTOR ev0047_tornado_0_normals[211] = {
#include "ev0047/script/ev0047_tornado_0_normals.inc"
};
static NJS_MODEL ev0047_tornado_0_model = {
#include "ev0047/script/ev0047_tornado_0_model.inc"
};
static NJS_OBJECT ev0047_tornado_object = {
#include "ev0047/script/ev0047_tornado_object.inc"
};
static NJS_MKEY_F ev0047_tornado_pos0[45] = {
#include "ev0047/script/ev0047_tornado_pos0.inc"
};
static NJS_MKEY_A ev0047_tornado_ang3[40] = {
#include "ev0047/script/ev0047_tornado_ang3.inc"
};
static NJS_MKEY_A ev0047_tornado_ang6[44] = {
#include "ev0047/script/ev0047_tornado_ang6.inc"
};
static NJS_MKEY_A ev0047_tornado_ang10[43] = {
#include "ev0047/script/ev0047_tornado_ang10.inc"
};
static NJS_MKEY_A ev0047_tornado_ang11[45] = {
#include "ev0047/script/ev0047_tornado_ang11.inc"
};
static NJS_MKEY_A ev0047_tornado_ang18[2] = {
#include "ev0047/script/ev0047_tornado_ang18.inc"
};
static NJS_MKEY_A ev0047_tornado_ang19[44] = {
#include "ev0047/script/ev0047_tornado_ang19.inc"
};
static NJS_MKEY_F ev0047_tornado_pos26[45] = {
#include "ev0047/script/ev0047_tornado_pos26.inc"
};
static NJS_MKEY_A ev0047_tornado_ang30[45] = {
#include "ev0047/script/ev0047_tornado_ang30.inc"
};
static NJS_MKEY_A ev0047_tornado_ang34[45] = {
#include "ev0047/script/ev0047_tornado_ang34.inc"
};
static NJS_MKEY_A ev0047_tornado_ang35[45] = {
#include "ev0047/script/ev0047_tornado_ang35.inc"
};
static NJS_MKEY_A ev0047_tornado_ang36[44] = {
#include "ev0047/script/ev0047_tornado_ang36.inc"
};
static NJS_MKEY_A ev0047_tornado_ang42[45] = {
#include "ev0047/script/ev0047_tornado_ang42.inc"
};
static NJS_MKEY_A ev0047_tornado_ang43[42] = {
#include "ev0047/script/ev0047_tornado_ang43.inc"
};
static NJS_MKEY_A ev0047_tornado_ang44[45] = {
#include "ev0047/script/ev0047_tornado_ang44.inc"
};
static NJS_MKEY_A ev0047_tornado_ang50[45] = {
#include "ev0047/script/ev0047_tornado_ang50.inc"
};
static NJS_MKEY_A ev0047_tornado_ang51[44] = {
#include "ev0047/script/ev0047_tornado_ang51.inc"
};
static NJS_MKEY_A ev0047_tornado_ang52[41] = {
#include "ev0047/script/ev0047_tornado_ang52.inc"
};
static NJS_MKEY_A ev0047_tornado_ang58[45] = {
#include "ev0047/script/ev0047_tornado_ang58.inc"
};
static NJS_MKEY_A ev0047_tornado_ang59[39] = {
#include "ev0047/script/ev0047_tornado_ang59.inc"
};
static NJS_MKEY_A ev0047_tornado_ang60[38] = {
#include "ev0047/script/ev0047_tornado_ang60.inc"
};
static NJS_MKEY_A ev0047_tornado_ang71[45] = {
#include "ev0047/script/ev0047_tornado_ang71.inc"
};
static NJS_MKEY_A ev0047_tornado_ang72[45] = {
#include "ev0047/script/ev0047_tornado_ang72.inc"
};
static NJS_MKEY_F ev0047_tornado_scl72[45] = {
#include "ev0047/script/ev0047_tornado_scl72.inc"
};
static NJS_MKEY_A ev0047_tornado_ang73[45] = {
#include "ev0047/script/ev0047_tornado_ang73.inc"
};
static NJS_MKEY_F ev0047_tornado_scl73[45] = {
#include "ev0047/script/ev0047_tornado_scl73.inc"
};
static NJS_MDATA3 ev0047_tornado_mdata[78] = {
#include "ev0047/script/ev0047_tornado_mdata.inc"
};
static NJS_MOTION ev0047_tornado_motion = {
#include "ev0047/script/ev0047_tornado_motion.inc"
};
/* @hack type: an array of one. SHC then keeps its address in R8 for the
 * Tornado's four plays, from 0x0cb806ec. as one NJS_ACTION passed as
 * &ev0047_tornado_action, each play loads the address from a pool, the code
 * runs 12 bytes short, and 546 of ev0047_script's 1159 words differ */
static NJS_ACTION ev0047_tornado_action[1] = {
#include "ev0047/script/ev0047_tornado_action.inc"
};

static struct task *ev0047_emerald = NULL;
static struct task *ev0047_tornado = NULL;
/* black between Tails' scene and the Tornado's rise. it darkens after TEXT_5,
 * and lifts as the rise starts */
static struct task *ev0047_fade = NULL;
static struct task *ev0047_end_fade = NULL;

/* the stub that takes the place of Sonic's node on the Tornado.
 * ev0047_hide_sonic gives it that node's child and sibling, and no model. its
 * scale, which the bss leaves at 0, hides every node below the stub */
static NJS_OBJECT ev0047_tornado_sonic_stub;
/* Sonic's node, saved while the stub takes its place */
static NJS_OBJECT *ev0047_tornado_sonic;

/* put the stub in place of Sonic's node, the root's third child, and save
 * the node for ev0047_show_sonic */
static void ev0047_hide_sonic(void)
{
    ev0047_tornado_sonic_stub.model = NULL;
    ev0047_tornado_sonic_stub.child =
        ev0047_tornado_object.child->sibling->sibling->child;
    ev0047_tornado_sonic_stub.sibling =
        ev0047_tornado_object.child->sibling->sibling->sibling;
    ev0047_tornado_sonic = ev0047_tornado_object.child->sibling->sibling;
    ev0047_tornado_object.child->sibling->sibling = &ev0047_tornado_sonic_stub;
}

/* not static: the image keeps it at 0x0cb80038, though nothing calls it. SHC
 * drops a static function nothing in its file calls */
void ev0047_show_sonic(void)
{
    /* @hack spelling: the Tornado's root node reached through its address. SHC
     * then loads the node's address from the pool word ev0047_hide_sonic reads
     * it from (0x0cb80088), and reads its child field 44 bytes past it. as
     * ev0047_tornado_object.child, SHC loads the child field's address from a
     * pool word of its own, the code runs 4 bytes long, and 5 of
     * ev0047_show_sonic's 7 words differ */
    (&ev0047_tornado_object)->child->sibling->sibling = ev0047_tornado_sonic;
}

void ev0047_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        /* the splash, in slot 0 just past the station door, hidden at a scale
         * of 0 */
        event_sprite_create(NULL, 0, 442.0F, -268.5F, 960.0F,
                            &event_sprite_texlists[EVENT_SPRITE_SPLASH]);
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        ev0047_sprite_set_scale(0, 0.0F, 0.0F);
        sound_load_file("e_0047.mlt");
        event_sound_init(5);
        event_flags[FLAG_STATION_DOOR] = STATION_DOOR_SHUT;
        event_place_actor(&ev0047_tornado, -738.9F, 1120.0F, 3336.6F, 0, 0xf000,
                          0);
        event_set_pos(event_get_player(0), 1413.8357F, 201.8F, 813.49F);
        event_set_ang(event_get_player(0), 0, 0x532d, 0);
        event_place_actor(&ev0047_emerald, 0.0F, 5.5F, 0.0F, 0, 0x8000, 0);
        event_set_flags(ev0047_emerald, EVENT_IGNORE_GROUND);
        event_set_pos(ev0047_emerald, 1413.8357F, 201.8F, 813.49F);
        event_set_ang(ev0047_emerald, 0, 0x532d, 0);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0047_emerald);
        event_play_action(event_get_player(0), &ev0047_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(ev0047_emerald, &ev0047_emerald_action_0,
                          &ev0047_emerald_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1377.38F, 271.6F, 843.437F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1000, 0xdd00, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 1382.855F, 253.136F,
                             838.07F);
        event_wait(10);
        event_set_pos(event_get_player(0), 1413.8357F, 201.8F, 813.49F);
        event_set_ang(event_get_player(0), 0, 0x532d, 0);
        event_wait(35);
        event_set_flags(ev0047_tornado, EVENT_IGNORE_GROUND);
        event_set_pos(ev0047_tornado, 439.3791F, -299.0F, 296.0182F);
        event_set_ang(ev0047_tornado, 0, 0x532d, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, 1396.68F, 209.71F,
                             826.56F);
        event_wait(109);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 1399.0F, 202.319F, 824.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0f00, 0xdd00, 0);
        event_wait(10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_48);
        event_voice_play(1275);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 100, 0x3c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 1402.7273F, 202.86F,
                             816.86F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x1016, 0xc0e8, 0xffff);
        event_wait(20);
        event_talk_start(event_get_player(0), TEXT_49);
        event_voice_play(1276);
        event_message_show(TEXT_2);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, 1402.593F, 202.86F,
                             810.86F);
        event_wait(110);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1421.04F, 205.3656F,
                             814.7755F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1000, 0x3900, 0xfe00);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 30, 0x27d2);
        event_voice_play(1277);
        event_talk_start(event_get_player(0), TEXT_50);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0047_emerald);
        event_play_action(event_get_player(0), &ev0047_tails_action_1,
                          &tails_texlist, 1.0F, 0, 8);
        event_play_action(ev0047_emerald, &ev0047_emerald_action_1,
                          &ev0047_emerald_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &ev0047_tails_action_2,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0047_emerald, &ev0047_emerald_action_2,
                          &ev0047_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 110, 0x29f5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, 1423.0F, 204.554F, 814.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0x0e00, 0x3e00, 0xfe00);
        event_voice_play(1278);
        event_talk_start(event_get_player(0), TEXT_51);
        event_message_show(TEXT_4);
        event_wait(50);
        stream_request_music(MUSIC_THEMILES);
        event_play_action(event_get_player(0), &ev0047_tails_action_3,
                          &tails_texlist, 1.0F, 0, 8);
        event_play_action(ev0047_emerald, &ev0047_emerald_action_3,
                          &ev0047_emerald_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &ev0047_tails_action_4,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0047_emerald, &ev0047_emerald_action_4,
                          &ev0047_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_voice_play(1279);
        event_talk_start(event_get_player(0), TEXT_52);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        ev0047_fade = event_fade_create(1.0F / 60.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_wait(40);
        event_sound_play(1223, 0, 0, 0);
        event_wait(30);
        event_sound_start(0, 1225, 1800);
        event_set_pos(event_get_player(0), 1527.3F, 192.8F, 694.49F);
        event_set_ang(event_get_player(0), 0, 0x532d, 0);
        event_set_pos(ev0047_emerald, 1527.3F, 192.8F, 694.49F);
        event_set_ang(ev0047_emerald, 0, 0x532d, 0);
        /* @hack spelling: the test and the call as one expression, as
         * src/event/play.h's EVENT_KILL has them. SHC then keeps
         * event_get_player's address in R13 from 0x0cb800fe to 0x0cb807f4.
         * it keeps event_camera_set_pos's in R12 from 0x0cb8019a. as an if,
         * the two trade, and 995 of 1159 words differ */
        ev0047_fade != NULL && (event_fade_set_speed(ev0047_fade, -0.1F), 0);
        event_play_action(ev0047_tornado, ev0047_tornado_action,
                          &ev0047_tornado_texlist, 0.0F, EVENT_REPEAT, 0);
        ev0047_hide_sonic();
        event_play_path(ev0047_tornado, &ev0047_tornado_rise_path, 0.71F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 453.89F, -295.42F, 295.174F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0700, 0x44e0, 0xffe0);
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 453.8F, -281.15F, 305.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x1700, 0x64d0, 0xff00);
        event_wait(50);
        event_sound_stop(0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 404.88F, 374.5F, 1676.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf1d0, 0x0660, 0);
        event_sound_start(1, 1228, 1800);
        event_sound_fade_volume(1, -30, 1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, 416.45F, 137.4F,
                             1743.187F);
        event_wait(130);
        event_sound_stop(1);
        event_clear_motions(ev0047_tornado);
        event_play_action(ev0047_tornado, ev0047_tornado_action,
                          &ev0047_tornado_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x471c);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 435.288F, -270.7F, 291.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0149, 0x8381, 0xff00);
        event_sound_start(0, 1225, 1800);
        event_sound_fade_volume(0, -20, 1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 444.25F, -270.49F,
                             291.7336F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x3111);
        event_wait(60);
        event_sound_stop(0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 407.363F, -248.9F, 1034.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0bd4, 0xf561, 0);
        event_sound_start(3, 1228, 1800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 410.47F, -289.33F,
                             1022.538F);
        event_wait(49);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 411.795F, -306.4F,
                             1017.653F);
        event_wait(10);
        event_flags[FLAG_STATION_DOOR] = STATION_DOOR_OPENING;
        event_sound_start(2, 1224, 1800);
        event_sound_fade_volume(2, 120, 1);
        event_wait(60);
        event_sound_stop(2);
        event_sound_stop(3);
        event_flags[FLAG_STATION_DOOR] = STATION_DOOR_SHUT;
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2000);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 435.26F, -274.0F, 308.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x05d0, 0x9600, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 433.6F, -273.566F,
                             311.6645F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x05d0, 0xac00, 0x0200);
        event_wait(5);
        event_flags[FLAG_STATION_DOOR] = STATION_DOOR_OPENING;
        event_sound_play(1224, -40, 0, 0);
        event_wait(25);
        event_clear_motions(ev0047_tornado);
        event_play_action(ev0047_tornado, ev0047_tornado_action,
                          &ev0047_tornado_texlist, 2.0F, EVENT_REPEAT, 0);
        event_voice_play(1280);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_move(ev0047_tornado, 439.9926F, -271.0F, 321.01F, 439.9926F,
                   -268.0F, 846.0F, 120);
        event_wait(10);
        event_move_stop_all();
        event_clear_motions(ev0047_tornado);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 438.95F, -275.0F, 370.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0x8000, 0);
        event_sound_start(4, 1227, 1800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 438.8636F, -271.55F,
                             961.1F);
        event_wait(30);
        event_play_action(ev0047_tornado, ev0047_tornado_action,
                          &ev0047_tornado_texlist, 2.0F, EVENT_REPEAT, 0);
        event_wait(4);
        event_sound_play(1229, 120, 0, 0);
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        ev0047_sprite_set_scale(0, 1.8F, 1.8F);
        event_wait(6);
        /* @bug mismatched-call: the call passes the scale 0 as ints, in R5 and
         * R6, where event_sprite_set_scale takes x and y as Floats, in FR4 and
         * FR5 (0x0cb80bc0 to 0x0cb80bc8). the splash takes whatever scale
         * event_wait above left in FR4 and FR5 */
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        ev0047_sprite_set_scale(0, 0, 0);
        event_sound_stop(4);
        event_camera_follow_clear();
        event_clear_paths(ev0047_tornado);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 470.145F, -292.864F,
                             1009.935F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x08d0, 0x0e00, 0xff00);
        event_play_path(ev0047_tornado, &ev0047_tornado_takeoff_path, 1.0F, 0);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 1, ev0047_tornado, 0.0F,
                                  0.0F, 0.0F, 0);
        event_wait(54);
        event_sound_play(1229, 120, 0, 0);
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        ev0047_sprite_set_scale(0, 1.8F, 1.8F);
        event_wait(83);
        ev0047_end_fade =
            event_fade_create(1.0F / 60.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(43);
        event_clear_motions(ev0047_tornado);
        event_wait(18);
        event_sprite_kill(0);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_clear_paths(ev0047_tornado);
        event_move_stop_all();
        event_reset_player(0);
        event_kill_task(&ev0047_emerald);
        event_kill_task(&ev0047_tornado);
        event_sound_stop_all();
        EVENT_KILL(ev0047_fade);
        EVENT_KILL(ev0047_end_fade);
        event_sprite_kill(0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0047_textures[] = {
    {"M_EM_RED", &ev0047_emerald_texlist},
    {"EV_TR2BEFORE_WITH_SONIC", &ev0047_tornado_texlist},
    {NULL, NULL},
};
