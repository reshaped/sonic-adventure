/* a Chao's shape, and its drawing. the shape is the Chao's own copy of its
 * type's tree, blended from the type's six trees by its record's h_pos,
 * v_pos and growth. it wears the parts animals give it, its coat and its
 * race wins */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "al_main/chao_name.h"
#include "al_main/empty.h"
#include "al_main/icon.h"
#include "al_main/models.h"
#include "al_main/shape.h"
#include "animal/animal.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "system/task.h"
#include "system/util.h"

/* a type's six trees, shape_trees' second index. TREE_START is the tree at
 * growth 0, and the rest are at growth 1. TREE_CENTER is at h_pos and v_pos
 * 0, and each of the others at a lean of 1 toward its skill */
enum shape_tree {
    TREE_START,
    TREE_CENTER,
    TREE_SWIM,
    TREE_FLY,
    TREE_RUN,
    TREE_POWER,
    SHAPE_TREES
};

/* the node that wears the model of a race the Chao has won */
#define WINS_NODE 34

/* beyond the far distance from the camera, a Chao neither jewel, gold nor
 * pal draws as alife_far_object. beyond the middle distance it draws as
 * alife_middle_object. alife_chao_show takes SHOW_FAR and SHOW_MIDDLE, and
 * alife_chao_draw DRAW_FAR and DRAW_MIDDLE */
#define SHOW_FAR 115.0F
#define SHOW_MIDDLE 95.0F
#define DRAW_FAR 125.0F
#define DRAW_MIDDLE 105.0F

/* the trees shape_trees names */
extern NJS_OBJECT dat_0c922420;
extern NJS_OBJECT dat_0c928a80;
extern NJS_OBJECT dat_0c92f118;
extern NJS_OBJECT dat_0c935778;
extern NJS_OBJECT dat_0c93be04;
extern NJS_OBJECT dat_0c94248c;
extern NJS_OBJECT dat_0c946d04;
extern NJS_OBJECT dat_0c94b5bc;
extern NJS_OBJECT dat_0c94fe68;
extern NJS_OBJECT dat_0c954708;
extern NJS_OBJECT dat_0c958f98;
extern NJS_OBJECT dat_0c95d844;
extern NJS_OBJECT dat_0c962860;
extern NJS_OBJECT dat_0c967878;
extern NJS_OBJECT dat_0c96c854;
extern NJS_OBJECT dat_0c971848;
extern NJS_OBJECT dat_0c9768e8;
extern NJS_OBJECT dat_0c97b8f0;
extern NJS_OBJECT dat_0c9806cc;
extern NJS_OBJECT dat_0c9854d4;
extern NJS_OBJECT dat_0c98a300;
extern NJS_OBJECT dat_0c98f158;
extern NJS_OBJECT dat_0c993f60;
extern NJS_OBJECT dat_0c998d6c;
extern NJS_OBJECT dat_0c99ddd8;
extern NJS_OBJECT dat_0c9a2e84;
extern NJS_OBJECT dat_0c9a7efc;
extern NJS_OBJECT dat_0c9acf08;
extern NJS_OBJECT dat_0c9b1f5c;
extern NJS_OBJECT dat_0c9b6fbc;
extern NJS_OBJECT dat_0c9bbdb0;
extern NJS_OBJECT dat_0c9c0ba8;
extern NJS_OBJECT dat_0c9c59ec;
extern NJS_OBJECT dat_0c9ca818;
extern NJS_OBJECT dat_0c9cf648;
extern NJS_OBJECT dat_0c9d4440;
extern NJS_OBJECT dat_0c9d8f5c;
/* the parts' trees, which child_part_objects and adult_part_objects list */
extern NJS_OBJECT dat_0ca1a3d8;
extern NJS_OBJECT dat_0ca1a7b0;
extern NJS_OBJECT dat_0ca1abd0;
extern NJS_OBJECT dat_0ca1ae80;
extern NJS_OBJECT dat_0ca1b130;
extern NJS_OBJECT dat_0ca1b454;
extern NJS_OBJECT dat_0ca1b778;
extern NJS_OBJECT dat_0ca1bbf0;
extern NJS_OBJECT dat_0ca1c068;
extern NJS_OBJECT dat_0ca1c4a8;
extern NJS_OBJECT dat_0ca1c8e8;
extern NJS_OBJECT dat_0ca1cb84;
extern NJS_OBJECT dat_0ca1cf18;
extern NJS_OBJECT dat_0ca1d150;
extern NJS_OBJECT dat_0ca1d388;
extern NJS_OBJECT dat_0ca1d80c;
extern NJS_OBJECT dat_0ca1dc90;
extern NJS_OBJECT dat_0ca1e4d0;
extern NJS_OBJECT dat_0ca1e8e8;
extern NJS_OBJECT dat_0ca1ecf4;
extern NJS_OBJECT dat_0ca1f0b0;
extern NJS_OBJECT dat_0ca1f478;
extern NJS_OBJECT dat_0ca1f75c;
extern NJS_OBJECT dat_0ca1fac0;
extern NJS_OBJECT dat_0ca1fd5c;
extern NJS_OBJECT dat_0ca1fff8;
extern NJS_OBJECT dat_0ca20450;
extern NJS_OBJECT dat_0ca2088c;
extern NJS_OBJECT dat_0ca20bf0;
extern NJS_OBJECT dat_0ca21064;
extern NJS_OBJECT dat_0ca213d8;
extern NJS_OBJECT dat_0ca21740;
extern NJS_OBJECT dat_0ca21b20;
extern NJS_OBJECT dat_0ca21f00;
extern NJS_OBJECT dat_0ca221f8;
extern NJS_OBJECT dat_0ca224dc;
extern NJS_OBJECT dat_0ca227c0;
extern NJS_OBJECT dat_0ca22b90;
extern NJS_OBJECT dat_0ca22f70;
extern NJS_OBJECT dat_0ca23348;
extern NJS_OBJECT dat_0ca23688;
extern NJS_OBJECT dat_0ca239c8;
extern NJS_OBJECT dat_0ca23d18;
extern NJS_OBJECT dat_0ca24068;
extern NJS_OBJECT dat_0ca243f8;
extern NJS_OBJECT dat_0ca24788;
extern NJS_OBJECT dat_0ca24b08;
extern NJS_OBJECT dat_0ca24e1c;
extern NJS_OBJECT dat_0ca25114;
extern NJS_OBJECT dat_0ca254c4;
extern NJS_OBJECT dat_0ca25890;
extern NJS_OBJECT dat_0ca25d40;
extern NJS_OBJECT dat_0ca261f0;
extern NJS_OBJECT dat_0ca26564;
extern NJS_OBJECT dat_0ca26984;
extern NJS_OBJECT dat_0ca26db0;
extern NJS_OBJECT dat_0ca27270;
extern NJS_OBJECT dat_0ca27730;
extern NJS_OBJECT dat_0ca27b58;
extern NJS_OBJECT dat_0ca27f8c;
extern NJS_OBJECT dat_0ca28384;
extern NJS_OBJECT dat_0ca2877c;
extern NJS_OBJECT dat_0ca28c18;
extern NJS_OBJECT dat_0ca290b4;
extern NJS_OBJECT dat_0ca294e8;
extern NJS_OBJECT dat_0ca2991c;
extern NJS_OBJECT dat_0ca29c90;
extern NJS_OBJECT dat_0ca2a0d8;
extern NJS_OBJECT dat_0ca2a3f4;
extern NJS_OBJECT dat_0ca2a710;
extern NJS_OBJECT dat_0ca2ac00;
extern NJS_OBJECT dat_0ca2b0e4;
extern NJS_OBJECT dat_0ca2b5a0;
extern NJS_OBJECT dat_0ca2ba5c;
extern NJS_OBJECT dat_0ca2bdfc;
extern NJS_OBJECT dat_0ca2c0c4;
extern NJS_OBJECT dat_0ca2c38c;
extern NJS_OBJECT dat_0ca2c80c;
extern NJS_OBJECT dat_0ca2cc8c;
extern NJS_OBJECT dat_0ca2d0a4;
extern NJS_OBJECT dat_0ca2d4bc;
extern NJS_OBJECT dat_0ca2d7d8;
extern NJS_OBJECT dat_0ca2db60;
extern NJS_OBJECT dat_0ca2e044;
extern NJS_OBJECT dat_0ca2e528;
extern NJS_OBJECT dat_0ca2e990;
extern NJS_OBJECT dat_0ca2edf8;
extern NJS_OBJECT dat_0ca2f218;
extern NJS_OBJECT dat_0ca2f568;
extern NJS_OBJECT dat_0ca2f89c;
extern NJS_OBJECT dat_0ca2fc94;
extern NJS_OBJECT dat_0ca3008c;
extern NJS_OBJECT dat_0ca30450;
extern NJS_OBJECT dat_0ca30814;
extern NJS_OBJECT dat_0ca30b1c;
extern NJS_OBJECT dat_0ca30ef4;
extern NJS_OBJECT dat_0ca312cc;
extern NJS_OBJECT dat_0ca316ec;
extern NJS_OBJECT dat_0ca31f70;
extern NJS_OBJECT dat_0ca32294;
extern NJS_OBJECT dat_0ca3270c;
extern NJS_OBJECT dat_0ca32b84;
extern NJS_OBJECT dat_0ca32fc4;
extern NJS_OBJECT dat_0ca33404;
extern NJS_OBJECT dat_0ca336a0;
extern NJS_OBJECT dat_0ca33a34;
extern NJS_OBJECT dat_0ca33c6c;
extern NJS_OBJECT dat_0ca33ea4;
extern NJS_OBJECT dat_0ca34328;
extern NJS_OBJECT dat_0ca347ac;
extern NJS_OBJECT dat_0ca35018;
extern NJS_OBJECT dat_0ca35414;
extern NJS_OBJECT dat_0ca35810;
extern NJS_OBJECT dat_0ca35bd8;
extern NJS_OBJECT dat_0ca35fa0;
extern NJS_OBJECT dat_0ca36284;
extern NJS_OBJECT dat_0ca365e8;
extern NJS_OBJECT dat_0ca36884;
extern NJS_OBJECT dat_0ca36b20;
extern NJS_OBJECT dat_0ca36f88;
extern NJS_OBJECT dat_0ca373c4;
extern NJS_OBJECT dat_0ca37738;
extern NJS_OBJECT dat_0ca37bac;
extern NJS_OBJECT dat_0ca37f20;
extern NJS_OBJECT dat_0ca38288;
extern NJS_OBJECT dat_0ca38668;
extern NJS_OBJECT dat_0ca38a48;
extern NJS_OBJECT dat_0ca38cf8;
extern NJS_OBJECT dat_0ca38fcc;
extern NJS_OBJECT dat_0ca392a0;
extern NJS_OBJECT dat_0ca39670;
extern NJS_OBJECT dat_0ca39a50;
extern NJS_OBJECT dat_0ca39e18;
extern NJS_OBJECT dat_0ca3a168;
extern NJS_OBJECT dat_0ca3a4b8;
extern NJS_OBJECT dat_0ca3a824;
extern NJS_OBJECT dat_0ca3ab90;
extern NJS_OBJECT dat_0ca3af20;
extern NJS_OBJECT dat_0ca3b2b0;
extern NJS_OBJECT dat_0ca3b630;
extern NJS_OBJECT dat_0ca3b928;
extern NJS_OBJECT dat_0ca3bc20;
extern NJS_OBJECT dat_0ca3bfd0;
extern NJS_OBJECT dat_0ca3c39c;
extern NJS_OBJECT dat_0ca3c84c;
extern NJS_OBJECT dat_0ca3ccfc;
extern NJS_OBJECT dat_0ca3d060;
extern NJS_OBJECT dat_0ca3d480;
extern NJS_OBJECT dat_0ca3d8ac;
extern NJS_OBJECT dat_0ca3dd6c;
extern NJS_OBJECT dat_0ca3e22c;
extern NJS_OBJECT dat_0ca3e654;
extern NJS_OBJECT dat_0ca3ea98;
extern NJS_OBJECT dat_0ca3eea0;
extern NJS_OBJECT dat_0ca3f2a8;
extern NJS_OBJECT dat_0ca3f750;
extern NJS_OBJECT dat_0ca3fbf8;
extern NJS_OBJECT dat_0ca4002c;
extern NJS_OBJECT dat_0ca40460;
extern NJS_OBJECT dat_0ca407d4;
extern NJS_OBJECT dat_0ca40c38;
extern NJS_OBJECT dat_0ca40f54;
extern NJS_OBJECT dat_0ca41270;
extern NJS_OBJECT dat_0ca41754;
extern NJS_OBJECT dat_0ca41c38;
extern NJS_OBJECT dat_0ca42104;
extern NJS_OBJECT dat_0ca425d0;
extern NJS_OBJECT dat_0ca42970;
extern NJS_OBJECT dat_0ca42c38;
extern NJS_OBJECT dat_0ca42f00;
extern NJS_OBJECT dat_0ca43380;
extern NJS_OBJECT dat_0ca43800;
extern NJS_OBJECT dat_0ca43c18;
extern NJS_OBJECT dat_0ca44030;
extern NJS_OBJECT dat_0ca4434c;
extern NJS_OBJECT dat_0ca446c4;
extern NJS_OBJECT dat_0ca44ba8;
extern NJS_OBJECT dat_0ca4508c;
extern NJS_OBJECT dat_0ca454f4;
extern NJS_OBJECT dat_0ca45940;
extern NJS_OBJECT dat_0ca45d60;
extern NJS_OBJECT dat_0ca46094;
extern NJS_OBJECT dat_0ca463c8;
extern NJS_OBJECT dat_0ca467c0;
extern NJS_OBJECT dat_0ca46bb8;
extern NJS_OBJECT dat_0ca46f7c;
extern NJS_OBJECT dat_0ca47340;
extern NJS_OBJECT dat_0ca47648;
/* the trees that draw in place of a Chao's shape beyond SHOW_FAR and
 * SHOW_MIDDLE, or DRAW_FAR and DRAW_MIDDLE */
extern NJS_OBJECT alife_far_object;
extern NJS_OBJECT alife_middle_object;
/* the models WINS_NODE wears, by the kind of race won, from 0. they wear
 * the textures alife_chao_texlist names pal, ame, saf, rub and eme */
extern NJS_OBJECT dat_0ca51fc8;
extern NJS_OBJECT dat_0ca5189c;
extern NJS_OBJECT dat_0ca52348;
extern NJS_OBJECT dat_0ca52188;
extern NJS_OBJECT dat_0ca51b8c;

/* clang-format off */
NJS_OBJECT *child_part_objects[ANIMAL_KINDS][CHAO_PART_SLOTS][2] = {
    /* ANIMAL_GOMA */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca1a3d8}, {NULL, &dat_0ca1a7b0},
     {NULL, NULL}, {NULL, NULL}, {NULL, &dat_0ca1abd0}},
    /* ANIMAL_PEN */
    {{NULL, &dat_0ca21064}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca213d8}, {NULL, &dat_0ca21740},
     {NULL, &dat_0ca21b20}, {NULL, &dat_0ca21f00}, {NULL, &dat_0ca221f8}},
    /* ANIMAL_RAKO */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca2c0c4}, {NULL, &dat_0ca2c38c}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca2c80c}, {NULL, &dat_0ca2cc8c},
     {NULL, &dat_0ca2d0a4}, {NULL, &dat_0ca2d4bc}, {NULL, &dat_0ca2d7d8}},
    /* ANIMAL_KUJA */
    {{NULL, &dat_0ca1cf18}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, &dat_0ca1d150},
     {NULL, &dat_0ca1d388}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca1d80c}, {NULL, &dat_0ca1dc90}, {NULL, &dat_0ca1e4d0}},
    /* ANIMAL_TUBA */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, &dat_0ca224dc},
     {NULL, &dat_0ca227c0}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca22b90}, {NULL, &dat_0ca22f70}, {NULL, &dat_0ca23348}},
    /* ANIMAL_OUM */
    {{NULL, &dat_0ca1fac0}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, &dat_0ca1fd5c},
     {NULL, &dat_0ca1fff8}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca20450}, {NULL, &dat_0ca2088c}, {NULL, &dat_0ca20bf0}},
    /* ANIMAL_BANB */
    {{NULL, NULL}, {NULL, &dat_0ca1ae80}, {NULL, &dat_0ca1b130},
     {NULL, &dat_0ca1b454}, {NULL, &dat_0ca1b778}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca1bbf0}, {NULL, &dat_0ca1c068},
     {NULL, &dat_0ca1c4a8}, {NULL, &dat_0ca1c8e8}, {NULL, &dat_0ca1cb84}},
    /* ANIMAL_USA */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca23688}, {NULL, &dat_0ca239c8}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca23d18}, {NULL, &dat_0ca24068},
     {NULL, &dat_0ca243f8}, {NULL, &dat_0ca24788}, {NULL, &dat_0ca24b08}},
    /* ANIMAL_WARA */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca24e1c}, {NULL, &dat_0ca25114}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca254c4}, {NULL, &dat_0ca25890},
     {NULL, &dat_0ca25d40}, {NULL, &dat_0ca261f0}, {NULL, &dat_0ca26564}},
    /* ANIMAL_GORI */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca26984}, {NULL, &dat_0ca26db0}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca27270}, {NULL, &dat_0ca27730},
     {NULL, &dat_0ca27b58}, {NULL, &dat_0ca27f8c}, {NULL, NULL}},
    /* ANIMAL_LION */
    {{NULL, &dat_0ca2a0d8}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca2a3f4}, {NULL, &dat_0ca2a710}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca2ac00}, {NULL, &dat_0ca2b0e4},
     {NULL, &dat_0ca2b5a0}, {NULL, &dat_0ca2ba5c}, {NULL, &dat_0ca2bdfc}},
    /* ANIMAL_ZOU */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca2f568}, {NULL, &dat_0ca2f89c}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca2fc94}, {NULL, &dat_0ca3008c},
     {NULL, &dat_0ca30450}, {NULL, &dat_0ca30814}, {NULL, &dat_0ca30b1c}},
    /* ANIMAL_MOGU */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca1e8e8}, {NULL, &dat_0ca1ecf4},
     {NULL, &dat_0ca1f0b0}, {NULL, &dat_0ca1f478}, {NULL, &dat_0ca1f75c}},
    /* ANIMAL_KOAR */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca28384}, {NULL, &dat_0ca2877c}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca28c18}, {NULL, &dat_0ca290b4},
     {NULL, &dat_0ca294e8}, {NULL, &dat_0ca2991c}, {NULL, &dat_0ca29c90}},
    /* ANIMAL_SUKA */
    {{NULL, &dat_0ca2db60}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca2e044}, {NULL, &dat_0ca2e528},
     {NULL, &dat_0ca2e990}, {NULL, &dat_0ca2edf8}, {NULL, &dat_0ca2f218}},
};
NJS_OBJECT *adult_part_objects[ANIMAL_KINDS][CHAO_PART_SLOTS][2] = {
    /* ANIMAL_GOMA */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca30ef4}, {NULL, &dat_0ca312cc},
     {NULL, NULL}, {NULL, NULL}, {NULL, &dat_0ca316ec}},
    /* ANIMAL_PEN */
    {{NULL, &dat_0ca37bac}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca37f20}, {NULL, &dat_0ca38288},
     {NULL, &dat_0ca38668}, {NULL, &dat_0ca38a48}, {NULL, &dat_0ca38cf8}},
    /* ANIMAL_RAKO */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca42c38}, {NULL, &dat_0ca42f00}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca43380}, {NULL, &dat_0ca43800},
     {NULL, &dat_0ca43c18}, {NULL, &dat_0ca44030}, {NULL, &dat_0ca4434c}},
    /* ANIMAL_KUJA */
    {{NULL, &dat_0ca33a34}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, &dat_0ca33c6c},
     {NULL, &dat_0ca33ea4}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca34328}, {NULL, &dat_0ca347ac}, {NULL, &dat_0ca35018}},
    /* ANIMAL_TUBA */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, &dat_0ca38fcc},
     {NULL, &dat_0ca392a0}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca39670}, {NULL, &dat_0ca39a50}, {NULL, &dat_0ca39e18}},
    /* ANIMAL_OUM */
    {{NULL, &dat_0ca365e8}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, &dat_0ca36884},
     {NULL, &dat_0ca36b20}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca36f88}, {NULL, &dat_0ca373c4}, {NULL, &dat_0ca37738}},
    /* ANIMAL_BANB */
    {{NULL, NULL}, {NULL, &dat_0ca1ae80}, {NULL, &dat_0ca1b130},
     {NULL, &dat_0ca31f70}, {NULL, &dat_0ca32294}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca3270c}, {NULL, &dat_0ca32b84},
     {NULL, &dat_0ca32fc4}, {NULL, &dat_0ca33404}, {NULL, &dat_0ca336a0}},
    /* ANIMAL_USA */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca3a168}, {NULL, &dat_0ca3a4b8}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca3a824}, {NULL, &dat_0ca3ab90},
     {NULL, &dat_0ca3af20}, {NULL, &dat_0ca3b2b0}, {NULL, &dat_0ca3b630}},
    /* ANIMAL_WARA */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca3b928}, {NULL, &dat_0ca3bc20}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca3bfd0}, {NULL, &dat_0ca3c39c},
     {NULL, &dat_0ca3c84c}, {NULL, &dat_0ca3ccfc}, {NULL, &dat_0ca3d060}},
    /* ANIMAL_GORI */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca3d480}, {NULL, &dat_0ca3d8ac}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca3dd6c}, {NULL, &dat_0ca3e22c},
     {NULL, &dat_0ca3e654}, {NULL, &dat_0ca3ea98}, {NULL, NULL}},
    /* ANIMAL_LION */
    {{NULL, &dat_0ca40c38}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca40f54}, {NULL, &dat_0ca41270}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca41754}, {NULL, &dat_0ca41c38},
     {NULL, &dat_0ca42104}, {NULL, &dat_0ca425d0}, {NULL, &dat_0ca42970}},
    /* ANIMAL_ZOU */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca46094}, {NULL, &dat_0ca463c8}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca467c0}, {NULL, &dat_0ca46bb8},
     {NULL, &dat_0ca46f7c}, {NULL, &dat_0ca47340}, {NULL, &dat_0ca47648}},
    /* ANIMAL_MOGU */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca35414}, {NULL, &dat_0ca35810},
     {NULL, &dat_0ca35bd8}, {NULL, &dat_0ca35fa0}, {NULL, &dat_0ca36284}},
    /* ANIMAL_KOAR */
    {{NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, &dat_0ca3eea0}, {NULL, &dat_0ca3f2a8}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca3f750}, {NULL, &dat_0ca3fbf8},
     {NULL, &dat_0ca4002c}, {NULL, &dat_0ca40460}, {NULL, &dat_0ca407d4}},
    /* ANIMAL_SUKA */
    {{NULL, &dat_0ca446c4}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
     {NULL, NULL}, {NULL, &dat_0ca44ba8}, {NULL, &dat_0ca4508c},
     {NULL, &dat_0ca454f4}, {NULL, &dat_0ca45940}, {NULL, &dat_0ca45d60}},
};
/* each type's six trees, by type from CHAO_CHILD to CHAO_CHAOS and by enum
 * shape_tree. a Chaos Chao has one tree for all six */
static NJS_OBJECT *shape_trees[CHAO_CHAOS + 1][SHAPE_TREES] = {
    /* CHAO_CHILD */
    {&dat_0c922420, &dat_0c928a80, &dat_0c94248c,
     &dat_0c935778, &dat_0c92f118, &dat_0c93be04},
    /* CHAO_STANDARD */
    {&dat_0c946d04, &dat_0c94b5bc, &dat_0c95d844,
     &dat_0c954708, &dat_0c94fe68, &dat_0c958f98},
    /* CHAO_SWIM */
    {&dat_0c962860, &dat_0c967878, &dat_0c97b8f0,
     &dat_0c971848, &dat_0c96c854, &dat_0c9768e8},
    /* CHAO_FLY */
    {&dat_0c9806cc, &dat_0c9854d4, &dat_0c998d6c,
     &dat_0c98f158, &dat_0c98a300, &dat_0c993f60},
    /* CHAO_RUN */
    {&dat_0c99ddd8, &dat_0c9a2e84, &dat_0c9b6fbc,
     &dat_0c9acf08, &dat_0c9a7efc, &dat_0c9b1f5c},
    /* CHAO_POWER */
    {&dat_0c9bbdb0, &dat_0c9c0ba8, &dat_0c9d4440,
     &dat_0c9ca818, &dat_0c9c59ec, &dat_0c9cf648},
    /* CHAO_CHAOS */
    {&dat_0c9d8f5c, &dat_0c9d8f5c, &dat_0c9d8f5c,
     &dat_0c9d8f5c, &dat_0c9d8f5c, &dat_0c9d8f5c},
};
/* clang-format on */

/* list the tree at object into objects from index, depth first: object,
 * then its child's tree, then its sibling's. the result is the index after
 * the last node listed */
static int tree_list_from(NJS_OBJECT *object, NJS_OBJECT **objects, int index)
{
    objects[index++] = object;
    if (object->child) {
        index = tree_list_from(object->child, objects, index);
    }
    if (object->sibling) {
        index = tree_list_from(object->sibling, objects, index);
    }
    return index;
}

/* list the tree at object into objects from 0, as tree_list_from does */
static void tree_list(NJS_OBJECT *object, NJS_OBJECT **objects)
{
    int index = 0;

    *objects = object;
    index++;
    if (object->child) {
        index = tree_list_from(object->child, objects, index);
    }
    if (object->sibling) {
        /* @hack redundant: index, set by this call and never read, which
         * keeps the call a BSR ahead of the shared return (0x0c90442e);
         * without it, SHC jumps to tree_list_from with a BRA, the function
         * runs 8 bytes longer, and 10 of 29 words differ */
        index = tree_list_from(object->sibling, objects, index);
    }
}

/* copy the tree at object, from the heap. each node and its model are
 * copied, with the model's points, normals, materials and meshsets. the
 * first meshset gets normals of its own too, nbMesh of them. all else the
 * copy shares with object, such as each meshset's strips. the result is
 * NULL when the heap fails for object's node, its model or any array copied
 * with them. below it, a node whose copy fails that way drops out, with its
 * children and its later siblings. object must not be NULL: it is read
 * before its test, and after it. alife_shape_init leaves the result
 * unchecked */
static NJS_OBJECT *tree_copy(NJS_OBJECT *object)
{
    NJS_OBJECT *copy;
    NJS_MODEL *source = object->model;
    NJS_MODEL *model;
    NJS_MESHSET *mesh;
    NJS_MESHSET *source_mesh;

    /* every caller passes a tree, so this test holds, and copy is set where
     * it is returned */
#ifdef GCC
    /* GCC can't see that every path sets copy: object->model's read above
     * already takes object as non-NULL, so the test holds */
    copy = NULL;
#endif
    if (object) { /* NOLINT(clang-diagnostic-sometimes-uninitialized) */
        if ((copy = heap_alloc(sizeof(NJS_OBJECT))) == NULL) {
            return NULL;
        }
        njMemCopy(copy, object, sizeof(NJS_OBJECT));
        if (source) {
            if ((model = heap_alloc(sizeof(NJS_MODEL))) == NULL) {
                heap_free(copy);
                return NULL;
            }
            njMemCopy(model, source, sizeof(NJS_MODEL));
            copy->model = model;
            if (source->points) {
                if ((model->points = heap_alloc(source->nbPoint *
                                                sizeof(NJS_POINT3))) == NULL) {
                    heap_free(model);
                    heap_free(copy);
                    return NULL;
                }
                njMemCopy(model->points, source->points,
                          source->nbPoint * sizeof(NJS_POINT3));
            }
            if (source->normals) {
                if ((model->normals = heap_alloc(source->nbPoint *
                                                 sizeof(NJS_VECTOR))) == NULL) {
                    if (model->points) {
                        heap_free(model->points);
                    }
                    heap_free(model);
                    heap_free(copy);
                    return NULL;
                }
                njMemCopy(model->normals, source->normals,
                          source->nbPoint * sizeof(NJS_VECTOR));
            }
            if (source->mats) {
                if ((model->mats = heap_alloc(source->nbMat *
                                              sizeof(NJS_MATERIAL))) == NULL) {
                    if (model->points) {
                        heap_free(model->points);
                    }
                    if (model->normals) {
                        heap_free(model->normals);
                    }
                    heap_free(model);
                    heap_free(copy);
                    return NULL;
                }
                njMemCopy(model->mats, source->mats,
                          source->nbMat * sizeof(NJS_MATERIAL));
            }
            if (source->meshsets) {
                if ((model->meshsets = heap_alloc(
                         source->nbMeshset * sizeof(NJS_MESHSET))) == NULL) {
                    if (model->points) {
                        heap_free(model->points);
                    }
                    if (model->normals) {
                        heap_free(model->normals);
                    }
                    if (model->mats) {
                        heap_free(model->mats);
                    }
                    heap_free(model);
                    heap_free(copy);
                    return NULL;
                }
                njMemCopy(model->meshsets, source->meshsets,
                          source->nbMeshset * sizeof(NJS_MESHSET));
                mesh = model->meshsets;
                source_mesh = source->meshsets;
                if (source_mesh->normals) {
                    if ((mesh->normals = heap_alloc(source_mesh->nbMesh *
                                                    sizeof(NJS_VECTOR))) ==
                        NULL) {
                        if (model->points) {
                            heap_free(model->points);
                        }
                        if (model->normals) {
                            heap_free(model->normals);
                        }
                        /* @bug memory: leaks the copied meshsets of a model
                         * with no materials when its first meshset's normals
                         * can't be allocated. their free tests mats a second
                         * time, not meshsets (0x0c904632, 0x0c90463c), where
                         * each other free tests the array it frees */
                        if (model->mats) {
                            heap_free(model->mats);
                            if (model->mats) {
                                heap_free(model->meshsets);
                            }
                        }
                        heap_free(model);
                        heap_free(copy);
                        return NULL;
                    }
                    njMemCopy(mesh->normals, source_mesh->normals,
                              source_mesh->nbMesh * sizeof(NJS_VECTOR));
                }
            }
        }
    }
    if (object->child) {
        copy->child = tree_copy(object->child);
    }
    if (object->sibling) {
        copy->sibling = tree_copy(object->sibling);
    }
    return copy;
}

/* free a tree that tree_copy made, and all it copied for each node's model.
 * each model left in the tree must be the copy's own. object must not be
 * NULL: its child and sibling are read before its test */
static void tree_free(NJS_OBJECT *object)
{
    NJS_MODEL *model;

    if (object->child) {
        tree_free(object->child);
        object->child = NULL;
    }
    if (object->sibling) {
        tree_free(object->sibling);
        object->sibling = NULL;
    }
    if (object) {
        model = object->model;
        if (model) {
            if (model->points) {
                heap_free(model->points);
                model->points = NULL;
            }
            if (model->normals) {
                heap_free(model->normals);
                model->normals = NULL;
            }
            if (model->mats) {
                heap_free(model->mats);
                model->mats = NULL;
            }
            if (model->meshsets) {
                if (model->meshsets->normals) {
                    heap_free(model->meshsets->normals);
                    model->meshsets->normals = NULL;
                }
                heap_free(model->meshsets);
                model->meshsets = NULL;
            }
            heap_free(model);
        }
        heap_free(object);
    }
}

/* give back what the shape borrowed, so that tree_free frees only the
 * copy's own. the nodes in models, whose models are NULL in every type's
 * trees, lose their models. each node in children takes back its own child,
 * the node after it */
static void shape_drop_borrowed(struct task_work *work, struct alife_chao *chao)
{
    short models[7] = {28, 29, 30, 33, WINS_NODE, 4, 7};
    short children[9] = {19, 21, 35, 37, 2, 5, 8, 10, 12};
    struct chao_shape *shape;
    int i;
    short index;

    shape = &chao->shape;
    for (i = 0; i < 7; ++i) {
        index = models[i];
        shape->objects[index]->model = NULL;
    }
    for (i = 0; i < 9; ++i) {
        index = children[i];
        shape->objects[index]->child = shape->objects[index + 1];
    }
}

void alife_shape_set_parts(struct task_work *work, struct alife_chao *chao)
{
    /* each part slot's node, and the enum chao_part of its part. the first
     * three slots' nodes take a model, and the other nine's a child. the last
     * two of model_nodes, 33 and 34, are no slot's and go unread. the image
     * copies all five to the stack (0x0c904830 to 0x0c904844) */
    short model_nodes[5] = {28, 29, 30, 33, 34};
    short child_nodes[9] = {19, 21, 35, 37, 2, 5, 8, 10, 12};
    short parts[CHAO_PART_SLOTS] = {
        CHAO_PART_FHEAD, CHAO_PART_HORN, CHAO_PART_HORN, CHAO_PART_EAR,
        CHAO_PART_EAR,   CHAO_PART_WING, CHAO_PART_WING, CHAO_PART_HAND,
        CHAO_PART_HAND,  CHAO_PART_FOOT, CHAO_PART_FOOT, CHAO_PART_TAIL,
    };
    struct chao *record = chao->record;
    Uint8 type = record->type;
    struct chao_shape *shape = &chao->shape;
    /* @hack type: i a Sint32, a long, which scales i + 3 whole: the second
     * loop's two table offsets start at 24 (0x0c90496e) and load from 4 past
     * them (0x0c9049c4); as an int, the function runs 32 bytes longer, and
     * 167 of 238 words and 2 of 9 pool words differ */
    Sint32 i;

    if (type == CHAO_CHAOS) {
        return;
    }
    /* @bug logic: slot 2, the second horn, takes slot 1's tree. the first three
     * slots read the tables at their part, not at their slot (0x0c9048e4 to
     * 0x0c9048ee). status.c's status_give_parts reads them at the slot, and so
     * reads ANIMAL_BANB's slot 2 tree, dat_0ca1b130, which this never does */
    for (i = 0; i < 3; ++i) {
        short index = model_nodes[i];
        short part = parts[i];
        /* the Sint8 widens with its sign, so -1, no part, stays -1 */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        short animal = record->parts[part];
        NJS_OBJECT *object;

        if (animal == -1) {
            continue;
        }
        if (type == CHAO_CHILD) {
            object = child_part_objects[animal][part][1];
        } else {
            object = adult_part_objects[animal][part][1];
        }
        if (object) {
            NJS_MATERIAL *material;
            unsigned short count;
            int j;

            shape->objects[index]->model = object->model;
            count = object->model->nbMat;
            material = object->model->mats;
            for (j = 0; j < count; ++j, ++material) {
                material->diffuse.argb.r = 255;
                material->diffuse.argb.g = 255;
                material->diffuse.argb.b = 255;
            }
        }
    }
    for (i = 0; i < 9; ++i) {
        short index = child_nodes[i];
        short part = parts[i + 3];
        /* the Sint8 widens with its sign, so -1, no part, stays -1 */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        short animal = record->parts[part];
        NJS_OBJECT *object;

        if (animal == -1) {
            continue;
        }
        if (type == CHAO_CHILD) {
            object = child_part_objects[animal][i + 3][1];
        } else {
            object = adult_part_objects[animal][i + 3][1];
        }
        if (object) {
            NJS_MATERIAL *material;
            unsigned short count;
            int j;

            shape->objects[index]->child = object->child;
            count = object->child->model->nbMat;
            material = object->child->model->mats;
            for (j = 0; j < count; ++j, ++material) {
                material->diffuse.argb.r = 255;
                material->diffuse.argb.g = 255;
                material->diffuse.argb.b = 255;
            }
        }
    }
}

/* coat the COAT_NODES' materials in the record's jewel, one of COAT_JEWELS,
 * mapped as an environment. a jewel past 3 counts as 0, ame */
static void shape_wear_jewel(struct task_work *work, struct alife_chao *chao)
{
    int nodes[14] = COAT_NODES;
    Uint16 textures[4] = COAT_JEWELS;
    int *node = nodes;
    struct chao_shape *shape = &chao->shape;
    Uint8 jewel = chao->record->jewel;

    if (jewel > 3) {
        jewel = 0;
    }
    while (*node != -1) {
        NJS_OBJECT *object = shape->objects[*node];
        NJS_MATERIAL *materials = object->model->mats;
        int count = object->model->nbMat;
        int i;

        for (i = 0; i < count; ++i) {
            materials[i].attr_texId = textures[jewel];
            materials[i].attrflags |= NJD_FLAG_USE_ENV;
        }
        ++node;
    }
}

/* coat the COAT_NODES' materials in COAT_GOLD, mapped as an environment */
static void shape_wear_gold(struct task_work *work, struct alife_chao *chao)
{
    int nodes[14] = COAT_NODES;
    struct chao_shape *shape = &chao->shape;
    int *node = nodes;

    while (*node != -1) {
        NJS_OBJECT *object = shape->objects[*node];
        NJS_MATERIAL *materials = object->model->mats;
        int count = object->model->nbMat;
        int i;

        for (i = 0; i < count; ++i) {
            materials[i].attr_texId = COAT_GOLD;
            materials[i].attrflags |= NJD_FLAG_USE_ENV;
        }
        ++node;
    }
}

/* coat the COAT_NODES' materials in COAT_PAL, mapped as an environment */
static void shape_wear_pal(struct task_work *work, struct alife_chao *chao)
{
    int nodes[14] = COAT_NODES;
    struct chao_shape *shape = &chao->shape;
    int *node = nodes;

    while (*node != -1) {
        NJS_OBJECT *object = shape->objects[*node];
        NJS_MATERIAL *materials = object->model->mats;
        int count = object->model->nbMat;
        int i;

        for (i = 0; i < count; ++i) {
            materials[i].attr_texId = COAT_PAL;
            materials[i].attrflags |= NJD_FLAG_USE_ENV;
        }
        ++node;
    }
}

void alife_shape_init(struct task_work *work, struct alife_chao *chao)
{
    struct chao_shape *shape = &chao->shape;
    /* @hack type: type a Sint8, which sign-extends the type byte (EXTS.B at
     * 0x0c904cbe). every chao_type is below 128, so type holds the record's
     * value either way; as the record's Uint8, 1 of 112 words differ */
    Sint8 type;

    chao->texlist = &alife_chao_texlist;
    type = chao->record->type;
    if (type == CHAO_EGG) {
        type = CHAO_CHILD;
    }
    chao->scale_speed = 0.0F;
    shape->model = tree_copy(shape_trees[type][TREE_START]);
    tree_list(shape->model, shape->objects);
    tree_list(shape_trees[type][TREE_START], shape->start);
    tree_list(shape_trees[type][TREE_CENTER], shape->center);
    tree_list(shape_trees[type][TREE_SWIM], shape->swim);
    tree_list(shape_trees[type][TREE_FLY], shape->fly);
    tree_list(shape_trees[type][TREE_RUN], shape->run);
    tree_list(shape_trees[type][TREE_POWER], shape->power);
    if (chao->record->flags & CHAO_JEWEL) {
        shape_wear_jewel(work, chao);
    }
    if (chao->record->flags & CHAO_GOLD) {
        shape_wear_gold(work, chao);
    } else if (chao->record->flags & CHAO_PAL) {
        shape_wear_pal(work, chao);
    }
}

void alife_shape_free(struct task_work *work, struct alife_chao *chao)
{
    fun_0c90ca28(work, chao);
    shape_drop_borrowed(work, chao);
    tree_free(chao->shape.model);
    chao->shape.model = NULL;
}

void alife_shape_blend(struct task_work *work, struct alife_chao *chao)
{
    Float h_pos = chao->record->h_pos;
    Float v_pos = chao->record->v_pos;
    Float growth = chao->record->growth;
    struct chao_shape *shape = &chao->shape;
    NJS_OBJECT **objects = shape->objects;
    NJS_OBJECT **start_nodes = shape->start;
    NJS_OBJECT **center_nodes = shape->center;
    NJS_OBJECT **h_nodes;
    NJS_OBJECT **v_nodes;
    Float h_rest;
    Float v_rest;
    Float young;

    /* a lean of 0 counts as 0.00001, which keeps h_pos + v_pos above 0 */
    if (h_pos == 0.0F) {
        h_pos = 0.00001F;
    }
    if (v_pos == 0.0F) {
        v_pos = 0.00001F;
    }
    if (h_pos > 0.0F) {
        h_nodes = shape->run;
    } else {
        h_nodes = shape->power;
        h_pos = -h_pos;
    }
    if (v_pos > 0.0F) {
        v_nodes = shape->fly;
    } else {
        v_nodes = shape->swim;
        v_pos = -v_pos;
    }
    h_rest = 1.0F - h_pos;
    v_rest = 1.0F - v_pos;
    young = 1.0F - growth;
    {
        int nodes[14] = {1, 15, 20, 22, 18,         25, 3,
                         6, 9,  11, 13, NODE_MOUTH, 36, 38};
        int node_count = 14;
        int i;

        if (chao->record->type == CHAO_CHILD) {
            node_count = 12;
        }
        for (i = 0; i < node_count; ++i) {
            Float scale;
            Float h_weight;
            Float v_weight;

            /* @hack spelling: scale takes the sum and then its reciprocal, in
             * two statements, which divide on each pass (FDIV at 0x0c904f26)
             * with the sum taken once before the loop (0x0c904f04); as
             * 1.0F / (h_pos + v_pos), the divide leaves the loop too, the
             * function runs 32 bytes longer, and 746 of 780 words and 12 of
             * 49 pool words differ */
            scale = h_pos + v_pos;
            scale = 1.0F / scale;
            h_weight = h_pos * scale;
            v_weight = v_pos * scale;
            if (scale != 0.0F) {
                int node = nodes[i];

                if (!objects[node]->model) {
                    return;
                }
                {
                    Uint32 count = objects[node]->model->nbPoint;
                    NJS_POINT3 *out = objects[node]->model->points;
                    NJS_POINT3 *center = center_nodes[node]->model->points;
                    NJS_POINT3 *h_side = h_nodes[node]->model->points;
                    NJS_POINT3 *v_side = v_nodes[node]->model->points;
                    NJS_POINT3 *start = start_nodes[node]->model->points;
                    Uint32 j;

                    for (j = 0; j < count;
                         ++j, ++out, ++center, ++h_side, ++v_side, ++start) {
                        out->x = (h_weight * ((center->x * h_rest) +
                                              (h_side->x * h_pos))) +
                                 (v_weight *
                                  ((center->x * v_rest) + (v_side->x * v_pos)));
                        out->y = (h_weight * ((center->y * h_rest) +
                                              (h_side->y * h_pos))) +
                                 (v_weight *
                                  ((center->y * v_rest) + (v_side->y * v_pos)));
                        out->z = (h_weight * ((center->z * h_rest) +
                                              (h_side->z * h_pos))) +
                                 (v_weight *
                                  ((center->z * v_rest) + (v_side->z * v_pos)));
                        out->x = (start->x * young) + (out->x * growth);
                        out->y = (start->y * young) + (out->y * growth);
                        out->z = (start->z * young) + (out->z * growth);
                    }
                }
                {
                    Uint32 count = objects[node]->model->nbPoint;
                    NJS_VECTOR *out = objects[node]->model->normals;
                    NJS_VECTOR *center = center_nodes[node]->model->normals;
                    NJS_VECTOR *h_side = h_nodes[node]->model->normals;
                    NJS_VECTOR *v_side = v_nodes[node]->model->normals;
                    NJS_VECTOR *start = start_nodes[node]->model->normals;
                    Uint32 j;

                    for (j = 0; j < count;
                         ++j, ++out, ++center, ++h_side, ++v_side, ++start) {
                        out->x = (h_weight * ((center->x * h_rest) +
                                              (h_side->x * h_pos))) +
                                 (v_weight *
                                  ((center->x * v_rest) + (v_side->x * v_pos)));
                        out->y = (h_weight * ((center->y * h_rest) +
                                              (h_side->y * h_pos))) +
                                 (v_weight *
                                  ((center->y * v_rest) + (v_side->y * v_pos)));
                        out->z = (h_weight * ((center->z * h_rest) +
                                              (h_side->z * h_pos))) +
                                 (v_weight *
                                  ((center->z * v_rest) + (v_side->z * v_pos)));
                        out->x = (start->x * young) + (out->x * growth);
                        out->y = (start->y * young) + (out->y * growth);
                        out->z = (start->z * young) + (out->z * growth);
                    }
                }
                if (node != NODE_MOUTH) {
                    Uint32 count = objects[node]->model->nbMat;
                    NJS_MATERIAL *out = objects[node]->model->mats;
                    NJS_MATERIAL *center = center_nodes[node]->model->mats;
                    NJS_MATERIAL *h_side = h_nodes[node]->model->mats;
                    NJS_MATERIAL *v_side = v_nodes[node]->model->mats;
                    NJS_MATERIAL *start = start_nodes[node]->model->mats;
                    Uint32 j;

                    for (j = 0; j < count;
                         ++j, ++out, ++center, ++h_side, ++v_side, ++start) {
                        Float red;
                        Float green;
                        Float blue;

                        if (chao->record->flags & CHAO_BEGG) {
                            red = 85.0F;
                            green = 85.0F;
                            blue = 85.0F;
                        } else {
                            Float blend_red;
                            Float blend_green;
                            Float blend_blue;
                            Float color_growth;
                            Float color_young;

                            blend_red =
                                (h_weight *
                                 ((center->diffuse.argb.r * h_rest) +
                                  (h_side->diffuse.argb.r * h_pos))) +
                                (v_weight * ((center->diffuse.argb.r * v_rest) +
                                             (v_side->diffuse.argb.r * v_pos)));
                            blend_green =
                                (h_weight *
                                 ((center->diffuse.argb.g * h_rest) +
                                  (h_side->diffuse.argb.g * h_pos))) +
                                (v_weight * ((center->diffuse.argb.g * v_rest) +
                                             (v_side->diffuse.argb.g * v_pos)));
                            blend_blue =
                                (h_weight *
                                 ((center->diffuse.argb.b * h_rest) +
                                  (h_side->diffuse.argb.b * h_pos))) +
                                (v_weight * ((center->diffuse.argb.b * v_rest) +
                                             (v_side->diffuse.argb.b * v_pos)));
                            if (blend_red > 255.0F) {
                                blend_red = 255.0F;
                            } else if (blend_red < 0.0F) {
                                blend_red = 0.0F;
                            }
                            if (blend_green > 255.0F) {
                                blend_green = 255.0F;
                            } else if (blend_green < 0.0F) {
                                blend_green = 0.0F;
                            }
                            if (blend_blue > 255.0F) {
                                blend_blue = 255.0F;
                            } else if (blend_blue < 0.0F) {
                                blend_blue = 0.0F;
                            }
                            color_growth = growth;
                            if (color_growth > 1.0F) {
                                color_growth = 1.0F;
                            }
                            color_young = 1.0F - color_growth;
                            red =
                                (Uint8)((start->diffuse.argb.r * color_young) +
                                        (blend_red * color_growth));
                            green =
                                (Uint8)((start->diffuse.argb.g * color_young) +
                                        (blend_green * color_growth));
                            blue =
                                (Uint8)((start->diffuse.argb.b * color_young) +
                                        (blend_blue * color_growth));
                            /* red, green and blue each hold a byte's value
                             * here, so these clamps change nothing */
                            if (red > 255.0F) {
                                red = 255.0F;
                            } else if (red < 0.0F) {
                                red = 0.0F;
                            }
                            if (green > 255.0F) {
                                green = 255.0F;
                            } else if (green < 0.0F) {
                                green = 0.0F;
                            }
                            if (blue > 255.0F) {
                                blue = 255.0F;
                            } else if (blue < 0.0F) {
                                blue = 0.0F;
                            }
                        }
                        out->diffuse.argb.r = red;
                        out->diffuse.argb.g = green;
                        out->diffuse.argb.b = blue;
                        if (j == 0 && i == 0) {
                            chao->icon.red = out->diffuse.argb.r;
                            chao->icon.green = out->diffuse.argb.g;
                            chao->icon.blue = out->diffuse.argb.b;
                            chao->icon.alpha = 255;
                        }
                    }
                }
            }
        }
    }
    {
        int nodes[22] = {0,  2,  4,  5,  7,  8,  10, 12, 14,        16, 19,
                         21, 23, 26, 28, 29, 30, 31, 33, WINS_NODE, 35, 37};
        int i;
        int node_count = 22;

        for (i = 0; i < node_count; ++i) {
            Float scale;
            Float h_weight;
            Float v_weight;

            /* @hack spelling: the sum and its reciprocal in two statements, as
             * in the loop above, which divide on each pass (FDIV at
             * 0x0c9053ba); as 1.0F / (h_pos + v_pos), the function runs 32
             * bytes longer, and 657 of 780 words and 12 of 49 pool words differ
             */
            scale = h_pos + v_pos;
            scale = 1.0F / scale;
            h_weight = h_pos * scale;
            v_weight = v_pos * scale;
            if (scale != 0.0F) {
                int node = nodes[i];
                Float *out = objects[node]->pos;
                Float *center = center_nodes[node]->pos;
                Float *h_side = h_nodes[node]->pos;
                Float *v_side = v_nodes[node]->pos;
                Float *start = start_nodes[node]->pos;

                *out = (h_weight * ((*center * h_rest) + (*h_side * h_pos))) +
                       (v_weight * ((*center * v_rest) + (*v_side * v_pos)));
                *out = (*start * young) + (*out * growth);
                ++out;
                ++center;
                ++h_side;
                ++v_side;
                ++start;
                *out = (h_weight * ((*center * h_rest) + (*h_side * h_pos))) +
                       (v_weight * ((*center * v_rest) + (*v_side * v_pos)));
                *out = (*start * young) + (*out * growth);
                ++out;
                ++center;
                ++h_side;
                ++v_side;
                ++start;
                *out = (h_weight * ((*center * h_rest) + (*h_side * h_pos))) +
                       (v_weight * ((*center * v_rest) + (*v_side * v_pos)));
                *out = (*start * young) + (*out * growth);
            }
        }
    }
}

void alife_shape_set_wins(struct task_work *work, struct alife_chao *chao)
{
    struct chao_shape *shape = &chao->shape;
    Uint8 wins = chao->record->race_wins;

    shape->objects[WINS_NODE]->scl[0] = 0.3F;
    shape->objects[WINS_NODE]->scl[1] = 0.3F;
    shape->objects[WINS_NODE]->scl[2] = 0.3F;
    /* @hack spelling: each model read through its object's address, so the pool
     * holds the objects and each load takes the model 4 past one (0x0c905506
     * to 0x0c905590); through .model, the function runs 10 bytes short, and
     * 56 of 84 words and 10 of 12 pool words differ */
    if (wins & 1 << 4) {
        shape->objects[WINS_NODE]->model = (&dat_0ca51b8c)->model;
        return;
    }
    if (wins & 1 << 3) {
        shape->objects[WINS_NODE]->model = (&dat_0ca52188)->model;
        return;
    }
    if (wins & 1 << 2) {
        shape->objects[WINS_NODE]->model = (&dat_0ca52348)->model;
        return;
    }
    if (wins & 1 << 1) {
        shape->objects[WINS_NODE]->model = (&dat_0ca5189c)->model;
        return;
    }
    if (wins & 1 << 0) {
        shape->objects[WINS_NODE]->model = (&dat_0ca51fc8)->model;
    }
}

void alife_squash_step(struct task_work *work, struct alife_chao *chao)
{
    Float displacement = work->scl.y - 1.0F;

    chao->scale_speed += -(chao->scale_speed * 0.15F) - (displacement * 0.28F);
    work->scl.y += chao->scale_speed;
    work->scl.x = 2.0F - work->scl.y;
}

void alife_grow_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao *record = chao->record;
    Bool changed = FALSE;

    if (record->h_pos > 1.5F) {
        record->h_pos = 1.5F;
    }
    if (record->h_pos < -1.5F) {
        record->h_pos = -1.5F;
    }
    if (record->v_pos > 1.5F) {
        record->v_pos = 1.5F;
    }
    if (record->v_pos < -1.5F) {
        record->v_pos = -1.5F;
    }
    if (chao->h_pos > record->h_pos) {
        record->h_pos += 0.003F;
        changed = TRUE;
    }
    if (chao->h_pos < record->h_pos) {
        record->h_pos -= 0.003F;
        changed = TRUE;
    }
    if (chao->v_pos > record->v_pos) {
        record->v_pos += 0.003F;
        changed = TRUE;
    }
    if (chao->v_pos < record->v_pos) {
        record->v_pos -= 0.003F;
        changed = TRUE;
    }
    if (record->flags & CHAO_GROWTH_DUE) {
        record->flags &= ~CHAO_GROWTH_DUE;
        if (record->type == CHAO_CHILD) {
            chao->growth += 0.008F;
        } else {
            chao->growth += 0.004F;
        }
        if (record->skills.swim < record->points.swim) {
            record->skills.swim += 10;
            chao->v_pos -= 0.0015F;
        }
        if (record->skills.fly < record->points.fly) {
            record->skills.fly += 10;
            chao->v_pos += 0.0015F;
        }
        if (record->skills.run < record->points.run) {
            record->skills.run += 10;
            chao->h_pos += 0.0015F;
        }
        if (record->skills.power < record->points.power) {
            record->skills.power += 10;
            chao->h_pos -= 0.0015F;
        }
        if (record->points.swim < record->skills.swim) {
            record->points.swim = record->skills.swim;
        }
        if (record->points.fly < record->skills.fly) {
            record->points.fly = record->skills.fly;
        }
        if (record->points.run < record->skills.run) {
            record->points.run = record->skills.run;
        }
        if (record->points.power < record->skills.power) {
            record->points.power = record->skills.power;
        }
    }
    if (chao->growth > 1.5F) {
        chao->growth = 1.5F;
    }
    if (chao->growth > record->growth) {
        record->growth += 0.003F;
        changed = TRUE;
    } else if (chao->growth < record->growth) {
        record->growth = chao->growth;
    }
    if (changed) {
        alife_shape_blend(work, chao);
    }
}

void alife_chao_show(struct task_work *work, struct alife_chao *chao)
{
    Float width = chao->scale * work->scl.x;
    Float height = chao->scale * work->scl.y;
    struct chao_motion *motion = &chao->motion;
    struct chao_motion_entry *current = &motion->table[motion->current];
    struct chao_motion_entry *next = &motion->table[motion->next];

    alife_squash_step(work, chao);
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
    if (chao->flags & CHAO_SHAPE_SHOWN) {
        struct task_work *camera;
        Angle angle;

        njSetTexture(chao->texlist);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, angle);
        TURN_VIA(njRotateX, work->ang.x, angle);
        TURN_VIA(njRotateY, work->ang.y, angle);
        njTranslate(NULL, 0.0F, 2.5F, 0.0F);
        TURN_VIA(njRotateX, chao->unk_080, angle);
        TURN_VIA(njRotateZ, chao->spin, angle);
        njTranslate(NULL, 0.0F, -2.5F, 0.0F);
        njScale(NULL, width, height, width);
        njRotateY(NULL, 0x4000);
        if (motion->flags & MOTION_BLEND) {
            Float distance;

            camera = camera_work;
            if (chao->flags & CHAO_NO_FAR_TREES) {
                distance = 0.0F;
            } else {
                distance = njDistanceP2P(&work->pos, &camera->pos);
            }
            if (chao->record->flags & CHAO_JEWEL) {
                NJS_MOTION_LINK link;
                NJS_ACTION_LINK action;

                link.motion[0] = current->motion;
                link.motion[1] = next->motion;
                link.frame[0] = motion->frame;
                link.frame[1] = motion->next_frame;
                action.motionlink = &link;
                action.object = chao->shape.model;
                draw_action_link(&action, motion->ratio);
            } else if (chao->record->flags & (CHAO_GOLD | CHAO_PAL)) {
                NJS_MOTION_LINK link;
                NJS_ACTION_LINK action;

                link.motion[0] = current->motion;
                link.motion[1] = next->motion;
                link.frame[0] = motion->frame;
                link.frame[1] = motion->next_frame;
                action.motionlink = &link;
                action.object = chao->shape.model;
                if (chao->record->flags & CHAO_BEGG) {
                    njSimpleActionLink(&action, motion->ratio);
                } else {
                    draw_action_link(&action, motion->ratio);
                }
            } else if (distance > SHOW_FAR) {
                Float red = chao->icon.red * (1.0F / 255.0F);
                Float green = chao->icon.green * (1.0F / 255.0F);
                Float blue = chao->icon.blue * (1.0F / 255.0F);
                /* @hack spelling: red, green and blue copied down chains of
                 * locals, whose reads count in SHC's weighing of the colors'
                 * spills: here they take sp+32, sp+28 and sp+24 (0x0c905bac to
                 * 0x0c905bdc); without the copies, the slots here and in the
                 * other arms move, and 16 of 828 words differ */
                Float red1 = red;
                Float green1 = green;
                Float green2 = green1;
                Float green3 = green2;
                Float green4 = green3;
                Float blue1 = blue;
                Float blue2 = blue1;
                Float blue3 = blue2;
                Float blue4 = blue3;
                Float blue5 = blue4;

                /* @hack redundant: each color less 0.0F, here and in the three
                 * arms below, which SHC writes as an FADD of -0.0F (0x0c905be2
                 * to 0x0c905be6); without them, 659 of 828 words differ */
                draw_set_material(0.0F, red1 - 0.0F, green4 - 0.0F,
                                  blue5 - 0.0F);
                njSimpleDrawMotion(&alife_far_object, current->motion,
                                   motion->frame);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            } else if (distance > SHOW_MIDDLE) {
                Float red = chao->icon.red * (1.0F / 255.0F);
                Float green = chao->icon.green * (1.0F / 255.0F);
                Float blue = chao->icon.blue * (1.0F / 255.0F);

                draw_set_material(0.0F, red - 0.0F, green - 0.0F, blue - 0.0F);
                njSimpleDrawMotion(&alife_middle_object, current->motion,
                                   motion->frame);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            } else {
                NJS_MOTION_LINK link;
                NJS_ACTION_LINK action;

                link.motion[0] = current->motion;
                link.motion[1] = next->motion;
                link.frame[0] = motion->frame;
                link.frame[1] = motion->next_frame;
                action.motionlink = &link;
                action.object = chao->shape.model;
                njSimpleActionLink(&action, motion->ratio);
            }
            if (!(chao->flags & CHAO_IN_COCOON)) {
                NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
                NJS_ACTION action;
                NJS_MATRIX matrix;
                NJS_POINT3 pos;
                NJS_POINT3 next_pos;
                Float ratio = motion->ratio;
                Float rest = 1.0F - ratio;

                action.object = chao->shape.model;
                action.motion = current->motion;
                pose_action(NULL, &action, motion->frame, NODE_ICON, &matrix);
                njCalcPoint(&matrix, &origin, &pos);
                action.motion = next->motion;
                pose_action(NULL, &action, motion->next_frame, NODE_ICON,
                            &matrix);
                njCalcPoint(&matrix, &origin, &next_pos);
                chao->icon.pos.x = (pos.x * rest) + (next_pos.x * ratio);
                chao->icon.pos.y = (pos.y * rest) + (next_pos.y * ratio);
                chao->icon.pos.z = (pos.z * rest) + (next_pos.z * ratio);
                alife_icon_step(work, chao);
            }
            motion->ratio += next->blend;
            if (motion->ratio > 1.0F) {
                motion->ratio = 0.0F;
                motion->frame = motion->next_frame;
                motion->current = motion->next;
                motion->end = next->end;
                motion->flags &= ~MOTION_BLEND;
                motion->flags |= MOTION_CHANGED;
                motion->rate = 1.0F;
            }
        } else {
            Float distance;

            camera = camera_work;
            if (chao->flags & CHAO_NO_FAR_TREES) {
                distance = 0.0F;
            } else {
                distance = njDistanceP2P(&work->pos, &camera->pos);
            }
            if (chao->record->flags & CHAO_JEWEL) {
                draw_motion(chao->shape.model, current->motion, motion->frame);
            } else if (chao->record->flags & (CHAO_GOLD | CHAO_PAL)) {
                if (chao->record->flags & CHAO_BEGG) {
                    njSimpleDrawMotion(chao->shape.model, current->motion,
                                       motion->frame);
                } else {
                    draw_motion(chao->shape.model, current->motion,
                                motion->frame);
                }
            } else if (distance > SHOW_FAR) {
                Float red = chao->icon.red * (1.0F / 255.0F);
                Float green = chao->icon.green * (1.0F / 255.0F);
                Float blue = chao->icon.blue * (1.0F / 255.0F);
                /* @hack spelling: as above, the copies give these colors
                 * sp+12, sp+16 and sp+36 (0x0c905eac to 0x0c905edc); without
                 * them, 17 of 828 words differ */
                Float red1 = red;
                Float red2 = red1;
                Float red3 = red2;
                Float red4 = red3;
                Float red5 = red4;
                Float red6 = red5;
                Float green1 = green;
                Float green2 = green1;
                Float green3 = green2;
                Float green4 = green3;
                Float green5 = green4;
                Float green6 = green5;
                Float blue1 = blue;

                draw_set_material(0.0F, red6 - 0.0F, green6 - 0.0F,
                                  blue1 - 0.0F);
                njSimpleDrawMotion(&alife_far_object, current->motion,
                                   motion->frame);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            } else if (distance > SHOW_MIDDLE) {
                Float red = chao->icon.red * (1.0F / 255.0F);
                Float green = chao->icon.green * (1.0F / 255.0F);
                Float blue = chao->icon.blue * (1.0F / 255.0F);
                /* @hack spelling: as above, the copies give these colors
                 * sp+20, sp+52 and sp+8 (0x0c905f02 to 0x0c905f32); without
                 * them, 15 of 828 words differ */
                Float red1 = red;
                Float red2 = red1;
                Float red3 = red2;
                Float red4 = red3;
                Float red5 = red4;
                Float blue1 = blue;
                Float blue2 = blue1;
                Float blue3 = blue2;
                Float blue4 = blue3;
                Float blue5 = blue4;
                Float blue6 = blue5;

                draw_set_material(0.0F, red5 - 0.0F, green - 0.0F,
                                  blue6 - 0.0F);
                njSimpleDrawMotion(&alife_middle_object, current->motion,
                                   motion->frame);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            } else {
                njSimpleDrawMotion(chao->shape.model, current->motion,
                                   motion->frame);
            }
            if (!(chao->flags & CHAO_IN_COCOON)) {
                NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
                NJS_ACTION action;
                NJS_MATRIX matrix;

                action.object = chao->shape.model;
                action.motion = current->motion;
                pose_action(NULL, &action, motion->frame, NODE_ICON, &matrix);
                njCalcPoint(&matrix, &origin, &chao->icon.pos);
                alife_icon_step(work, chao);
                /* @hack redundant: an empty test of flag 0x1, which keeps the
                 * image's TST #1 at 0x0c905fe6; without it, 104 of 828 words
                 * differ */
                if (chao->flags & 0x1) {
                }
            }
        }
        njPopMatrix(1);
    }
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    if (chao->flags & CHAO_IN_COCOON) {
        Float size = chao->cocoon_scale * 1.45F;
        Float alpha = chao->cocoon_alpha - 1.0F;
        Angle angle;
        /* @hack spelling: as above, the copies give alpha sp+4 (0x0c90601a,
         * 0x0c90602a); without them, 15 of 828 words differ */
        Float alpha1 = alpha;
        Float alpha2 = alpha1;
        Float alpha3 = alpha2;
        Float alpha4 = alpha3;
        Float alpha5 = alpha4;
        Float alpha6 = alpha5;

        njSetTexture(&alife_chao_texlist);
        njPushMatrix(NULL);
        draw_set_material(alpha6, 1.0F, 1.0F, 1.0F);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y + 0x4000, angle);
        njScale(NULL, size, size, size);
        njSimpleDrawObject(&alife_cocoon_object);
        njPopMatrix(1);
    }
    draw_attr_restore();
    chao_name_step(work, chao);
    draw_control_set(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* an inline identity, which returns value unchanged. alife_chao_draw's far
 * blend passes its blue through it */
#pragma inline(same)
static Float same(Float value)
{
    return value;
}

void alife_chao_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    Float width = chao->scale * work->scl.x;
    Float height = chao->scale * work->scl.y;
    struct chao_motion *motion = &chao->motion;
    struct chao_motion_entry *current = &motion->table[motion->current];
    struct chao_motion_entry *next = &motion->table[motion->next];

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
    if (chao->flags & CHAO_SHAPE_SHOWN) {
        Angle angle;
        struct task_work *camera;

        njSetTexture(chao->texlist);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, angle);
        TURN_VIA(njRotateX, work->ang.x, angle);
        TURN_VIA(njRotateY, work->ang.y, angle);
        njTranslate(NULL, 0.0F, 2.5F, 0.0F);
        TURN_VIA(njRotateX, chao->unk_080, angle);
        TURN_VIA(njRotateZ, chao->spin, angle);
        njTranslate(NULL, 0.0F, -2.5F, 0.0F);
        njScale(NULL, width, height, width);
        njRotateY(NULL, 0x4000);
        if (motion->flags & MOTION_BLEND) {
            Float distance;

            camera = camera_work;
            if (chao->flags & CHAO_NO_FAR_TREES) {
                distance = 0.0F;
            } else {
                distance = njDistanceP2P(&work->pos, &camera->pos);
            }
            if (chao->record->flags & CHAO_JEWEL) {
                NJS_MOTION_LINK link;
                NJS_ACTION_LINK action;

                link.motion[0] = current->motion;
                link.motion[1] = next->motion;
                link.frame[0] = motion->frame;
                link.frame[1] = motion->next_frame;
                action.motionlink = &link;
                action.object = chao->shape.model;
                draw_action_link(&action, motion->ratio);
            } else if (chao->record->flags & (CHAO_GOLD | CHAO_PAL)) {
                NJS_MOTION_LINK link;
                NJS_ACTION_LINK action;

                link.motion[0] = current->motion;
                link.motion[1] = next->motion;
                link.frame[0] = motion->frame;
                link.frame[1] = motion->next_frame;
                action.motionlink = &link;
                action.object = chao->shape.model;
                if (chao->record->flags & CHAO_BEGG) {
                    njSimpleActionLink(&action, motion->ratio);
                } else {
                    draw_action_link(&action, motion->ratio);
                }
            } else if (distance > DRAW_FAR) {
                Float red = chao->icon.red * (1.0F / 255.0F);
                Float green = chao->icon.green * (1.0F / 255.0F);
                /* @hack redundant: blue passed through same, an inline
                 * identity, which counts in SHC's weighing of the colors'
                 * spills and constants: blue takes sp+12 (0x0c90648c), and
                 * -0.0F FR13 and 1.0F / 255.0F FR14 (0x0c906304,
                 * 0x0c906308); without it, all four color arms change
                 * (0x0c906474 to 0x0c9067dc), and 27 of 798 words and 2 of
                 * 144 pool words differ */
                Float blue = same(chao->icon.blue * (1.0F / 255.0F));

                /* @hack redundant: each color less 0.0F, here and in the three
                 * arms below, which SHC writes as an FADD of -0.0F (0x0c9064a2
                 * to 0x0c9064a6); without them, 782 of 798 words differ */
                draw_set_material(0.0F, red - 0.0F, green - 0.0F, blue - 0.0F);
                njSimpleDrawMotion(&alife_far_object, current->motion,
                                   motion->frame);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            } else if (distance > DRAW_MIDDLE) {
                Float red = chao->icon.red * (1.0F / 255.0F);
                Float green = chao->icon.green * (1.0F / 255.0F);
                Float blue = chao->icon.blue * (1.0F / 255.0F);

                draw_set_material(0.0F, red - 0.0F, green - 0.0F, blue - 0.0F);
                njSimpleDrawMotion(&alife_middle_object, current->motion,
                                   motion->frame);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            } else {
                NJS_MOTION_LINK link;
                NJS_ACTION_LINK action;

                link.motion[0] = current->motion;
                link.motion[1] = next->motion;
                link.frame[0] = motion->frame;
                link.frame[1] = motion->next_frame;
                action.motionlink = &link;
                action.object = chao->shape.model;
                njSimpleActionLink(&action, motion->ratio);
            }
            if (!(chao->flags & CHAO_IN_COCOON)) {
                NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
                NJS_ACTION action;
                NJS_MATRIX matrix;
                NJS_POINT3 pos;
                NJS_POINT3 next_pos;
                Float ratio = motion->ratio;
                Float rest = 1.0F - ratio;

                action.object = chao->shape.model;
                action.motion = current->motion;
                pose_action(NULL, &action, motion->frame, NODE_ICON, &matrix);
                njCalcPoint(&matrix, &origin, &pos);
                action.motion = next->motion;
                pose_action(NULL, &action, motion->next_frame, NODE_ICON,
                            &matrix);
                njCalcPoint(&matrix, &origin, &next_pos);
                /* @hack spelling: 1.0F - ratio taken again for x, the value
                 * rest holds, which counts in SHC's weighing of the spills:
                 * the plain arms' colors take sp+52, sp+48, sp+44 and sp+40
                 * (0x0c90674c to 0x0c9067c2); without it, 6 of 798 words
                 * differ */
                chao->icon.pos.x =
                    (pos.x * (1.0F - ratio)) + (next_pos.x * ratio);
                chao->icon.pos.y = (pos.y * rest) + (next_pos.y * ratio);
                chao->icon.pos.z = (pos.z * rest) + (next_pos.z * ratio);
                alife_icon_draw(work, chao);
            }
        } else {
            Float distance;

            camera = camera_work;
            if (chao->flags & CHAO_NO_FAR_TREES) {
                distance = 0.0F;
            } else {
                distance = njDistanceP2P(&work->pos, &camera->pos);
            }
            if (chao->record->flags & CHAO_JEWEL) {
                draw_motion(chao->shape.model, current->motion, motion->frame);
            } else if (chao->record->flags & (CHAO_GOLD | CHAO_PAL)) {
                if (chao->record->flags & CHAO_BEGG) {
                    njSimpleDrawMotion(chao->shape.model, current->motion,
                                       motion->frame);
                } else {
                    draw_motion(chao->shape.model, current->motion,
                                motion->frame);
                }
            } else if (distance > DRAW_FAR) {
                Float red = chao->icon.red * (1.0F / 255.0F);
                Float green = chao->icon.green * (1.0F / 255.0F);
                Float blue = chao->icon.blue * (1.0F / 255.0F);

                /* @hack redundant: a label nothing jumps to, which counts in
                 * SHC's weighing of the spills: height takes sp+56 (0x0c906172,
                 * 0x0c906264), and red and blue here sp+52 and sp+44
                 * (0x0c90674c to 0x0c90677c); without it, 5 of 798 words
                 * differ */
            far_arm: /* NOLINT(clang-diagnostic-unused-label) */
                draw_set_material(0.0F, red - 0.0F, green - 0.0F, blue - 0.0F);
                njSimpleDrawMotion(&alife_far_object, current->motion,
                                   motion->frame);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            } else if (distance > DRAW_MIDDLE) {
                Float red = chao->icon.red * (1.0F / 255.0F);
                Float green = chao->icon.green * (1.0F / 255.0F);
                Float blue = chao->icon.blue * (1.0F / 255.0F);
                Float red1;
                Float red2;

                /* @hack redundant: a label nothing jumps to, as far_arm, which
                 * gives the far arm's red sp+52 (0x0c90674c, 0x0c90677c) and
                 * blue here sp+40 (0x0c9067c2); without it, 3 of 798 words
                 * differ */
            middle_arm: /* NOLINT(clang-diagnostic-unused-label) */
                /* @hack spelling: red copied twice in the call, which counts
                 * in SHC's weighing of the spills: red takes sp+8 (0x0c9067a2,
                 * 0x0c9067d2), and the far blend's blue sp+12 (0x0c90648c);
                 * without the copies, 3 of 798 words differ */
                draw_set_material(0.0F, (red1 = red, red2 = red1, red2) - 0.0F,
                                  green - 0.0F, blue - 0.0F);
                njSimpleDrawMotion(&alife_middle_object, current->motion,
                                   motion->frame);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            } else {
                njSimpleDrawMotion(chao->shape.model, current->motion,
                                   motion->frame);
            }
            if (!(chao->flags & CHAO_IN_COCOON)) {
                NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
                NJS_ACTION action;
                NJS_MATRIX matrix;

                action.object = chao->shape.model;
                action.motion = current->motion;
                pose_action(NULL, &action, motion->frame, NODE_ICON, &matrix);
                njCalcPoint(&matrix, &origin, &chao->icon.pos);
                alife_icon_draw(work, chao);
                /* @hack redundant: an empty test of flag 0x1, which keeps the
                 * image's TST #1 at 0x0c906886; without it, 121 of 798 words
                 * differ */
                if (chao->flags & 0x1) {
                }
            }
        }
        njPopMatrix(1);
    }
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    if (chao->flags & CHAO_IN_COCOON) {
        Float size = chao->cocoon_scale * 1.45F;
        Float alpha = chao->cocoon_alpha - 1.0F;
        Angle angle;
        /* @hack spelling: alpha copied twice, which counts in SHC's
         * weighing of the spills: alpha takes sp+4 (0x0c9068ba, 0x0c9068ca),
         * and the colors at 0x0c90648c, 0x0c9067a2 and 0x0c9067d2 their
         * slots; without the copies, 5 of 798 words differ */
        Float alpha1 = alpha;
        Float alpha2 = alpha1;

        njSetTexture(&alife_chao_texlist);
        njPushMatrix(NULL);
        draw_set_material(alpha2, 1.0F, 1.0F, 1.0F);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y + 0x4000, angle);
        njScale(NULL, size, size, size);
        njSimpleDrawObject(&alife_cocoon_object);
        njPopMatrix(1);
    }
    draw_attr_restore();
    chao_name_draw(work, chao);
    draw_control_set(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
