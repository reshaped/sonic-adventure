/* each character's tutorial pages, and the page task that shows and turns
 * them. a page draws its pictures from up to two texlists. the second is
 * the character's own, and the first, for every character but Sonic, is
 * Sonic's */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/screen.h"
#include "progress/progress.h"
#include "summary/tutorial.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 7
#define TEXT_16_LENGTH 7
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 7
#define TEXT_20_LENGTH 7
#define TEXT_21_LENGTH 7
#define TEXT_22_LENGTH 7
#define TEXT_23_LENGTH 7
#define TEXT_24_LENGTH 7
#define TEXT_25_LENGTH 7
#define TEXT_26_LENGTH 7
#define TEXT_27_LENGTH 7
#define TEXT_28_LENGTH 7
#define TEXT_29_LENGTH 7
#define TEXT_30_LENGTH 7
#define TEXT_31_LENGTH 7
#define TEXT_32_LENGTH 7
#define TEXT_33_LENGTH 7
#define TEXT_34_LENGTH 7
#define TEXT_35_LENGTH 7
#define TEXT_36_LENGTH 7
#define TEXT_37_LENGTH 7
#define TEXT_38_LENGTH 7
#define TEXT_39_LENGTH 7
#define TEXT_40_LENGTH 7
#define TEXT_41_LENGTH 7
#define TEXT_42_LENGTH 7
#define TEXT_43_LENGTH 7
#define TEXT_44_LENGTH 7
#define TEXT_45_LENGTH 7
#define TEXT_46_LENGTH 7
#define TEXT_47_LENGTH 7
#define TEXT_48_LENGTH 7
#define TEXT_49_LENGTH 9
#define TEXT_50_LENGTH 7
#define TEXT_51_LENGTH 7
#define TEXT_52_LENGTH 7
#define TEXT_53_LENGTH 7
#define TEXT_54_LENGTH 7
#define TEXT_55_LENGTH 7
#define TEXT_56_LENGTH 7
#define TEXT_57_LENGTH 7
#define TEXT_58_LENGTH 7
#define TEXT_59_LENGTH 7
#define TEXT_60_LENGTH 7
#define TEXT_61_LENGTH 7
#define TEXT_62_LENGTH 7
#define TEXT_63_LENGTH 7
#define TEXT_64_LENGTH 7
#define TEXT_65_LENGTH 7
#define TEXT_66_LENGTH 7
#define TEXT_67_LENGTH 7
#define TEXT_68_LENGTH 7
#define TEXT_69_LENGTH 7
#define TEXT_70_LENGTH 7
#define TEXT_71_LENGTH 7
#define TEXT_72_LENGTH 7
#define TEXT_73_LENGTH 7
#define TEXT_74_LENGTH 7
#define TEXT_75_LENGTH 7
#define TEXT_76_LENGTH 7
#define TEXT_77_LENGTH 7
#define TEXT_78_LENGTH 7
#define TEXT_79_LENGTH 7
#define TEXT_80_LENGTH 7
#define TEXT_81_LENGTH 7
#define TEXT_82_LENGTH 7
#define TEXT_83_LENGTH 7
#define TEXT_84_LENGTH 7
#define TEXT_85_LENGTH 7
#define TEXT_86_LENGTH 7
#define TEXT_87_LENGTH 7
#define TEXT_88_LENGTH 7
#define TEXT_89_LENGTH 7
#define TEXT_90_LENGTH 7
#define TEXT_91_LENGTH 7
#define TEXT_92_LENGTH 7
#define TEXT_93_LENGTH 7
#define TEXT_94_LENGTH 7
#define TEXT_95_LENGTH 7
#define TEXT_96_LENGTH 6
#define TEXT_97_LENGTH 8
#define TEXT_98_LENGTH 7
#define TEXT_99_LENGTH 7
#define TEXT_100_LENGTH 7
#define TEXT_101_LENGTH 7
#define TEXT_102_LENGTH 7
#define TEXT_103_LENGTH 7
#define TEXT_104_LENGTH 7
#define TEXT_105_LENGTH 7
#define TEXT_106_LENGTH 7
#define TEXT_107_LENGTH 7
#define TEXT_108_LENGTH 7
#define TEXT_109_LENGTH 7
#define TEXT_110_LENGTH 7
#define TEXT_111_LENGTH 7
#define TEXT_112_LENGTH 7
#define TEXT_113_LENGTH 7
#define TEXT_114_LENGTH 7
#define TEXT_115_LENGTH 7
#define TEXT_116_LENGTH 7
#define TEXT_117_LENGTH 7
#define TEXT_118_LENGTH 7
#define TEXT_119_LENGTH 7
#define TEXT_120_LENGTH 7
#define TEXT_121_LENGTH 12
#define TEXT_122_LENGTH 12
#define TEXT_123_LENGTH 7
#define TEXT_124_LENGTH 7
#define TEXT_125_LENGTH 7
#define TEXT_126_LENGTH 7
#define TEXT_127_LENGTH 7
#define TEXT_128_LENGTH 7
#define TEXT_129_LENGTH 7
#define TEXT_130_LENGTH 7
#define TEXT_131_LENGTH 7
#define TEXT_132_LENGTH 7
#define TEXT_133_LENGTH 7
#define TEXT_134_LENGTH 7
#define TEXT_135_LENGTH 7
#define TEXT_136_LENGTH 7
#define TEXT_137_LENGTH 7
#define TEXT_138_LENGTH 7
#define TEXT_139_LENGTH 7
#define TEXT_140_LENGTH 7
#define TEXT_141_LENGTH 7
#define TEXT_142_LENGTH 7
#define TEXT_143_LENGTH 7
#define TEXT_144_LENGTH 7
#define TEXT_145_LENGTH 7
#define TEXT_146_LENGTH 7
#define TEXT_147_LENGTH 7
#define TEXT_148_LENGTH 7
#define TEXT_149_LENGTH 7
#define TEXT_150_LENGTH 7
#define TEXT_151_LENGTH 7
#define TEXT_152_LENGTH 7
#define TEXT_153_LENGTH 7
#include "summary/tutorial/text.inc"

/* a page's flag when it always shows */
#define PAGE_ALWAYS 0xffff

struct tutorial_page {
    Uint16 flag; /* the event flag that shows the page, or PAGE_ALWAYS */
    /* the page's size in dots, unscaled. its box reaches 16 dots past each
     * edge */
    Uint16 width, height;
    Uint16 unk_06;
    /* the page's picture from each of the tutorial's texlists, or null */
    struct screen_parts *parts[2];
};

/* a story's tutorial: count pages, the two texlists they draw from, and
 * the PVM each texlist loads from. a null texlist draws nothing, and a
 * texlist loads only with its file */
struct tutorial {
    Uint16 count;
    /* how many of the pages, from the first, are controls pages */
    Uint16 controls;
    struct tutorial_page *pages;
    NJS_TEXLIST *texlists[2];
    char *files[2];
};

/* the page task's record, in task->extra */
struct tutorial_pager {
    int unk_00;
    enum tutorial_edge edge;
    int page; /* the page shown */
    int next; /* the page a turn shows once the page shown has closed */
    struct tutorial *tutorial;
    /* the part of its height a page loses or gains each step of a turn */
    Float speed;
    Sint8 past_controls; /* 1 while the page shown is a game page, else 0 */
    Sint8 unk_19;
};

/* the colors of the translucent blue box behind the page, at its top and
 * at its bottom */
#define BOX_TOP 0xc0002e67
#define BOX_BOTTOM 0xc0117bff

/* where the pages show when tutorial_create gets no pos */
static const NJS_POINT3 tutorial_pos = {200.0F, 96.0F, 10000.0F};

/* the page task's tutorial, which tutorial_create keeps in the work's
 * vars.p[2] as well as in the pager */
#define TUTORIAL(work) ((struct tutorial *)(work)->vars.p[2])

/* each character's own PVM, by story. nothing reads them */
static char *tutorial_files[TUTORIALS] = {
    "tutomsg_sonic", "tutomsg_tails", "tutomsg_knuckles",
    "tutomsg_amy",   "tutomsg_big",   "tutomsg_e102",
};
static NJS_TEXNAME tutomsg_sonic_textures[31] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0},
};
static NJS_TEXLIST tutomsg_sonic_texlist = {
#include "summary/tutorial/tutomsg_sonic_texlist.inc"
};
static NJS_TEXNAME tutomsg_tails_textures[17] = {
    {TEXT_32, 0, 0}, {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0},
    {TEXT_36, 0, 0}, {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0},
    {TEXT_40, 0, 0}, {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0},
    {TEXT_44, 0, 0}, {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0},
    {TEXT_48, 0, 0},
};
static NJS_TEXLIST tutomsg_tails_texlist = {
#include "summary/tutorial/tutomsg_tails_texlist.inc"
};
static NJS_TEXNAME tutomsg_knuckles_textures[27] = {
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0}, {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
    {TEXT_57, 0, 0}, {TEXT_58, 0, 0}, {TEXT_59, 0, 0}, {TEXT_60, 0, 0},
    {TEXT_61, 0, 0}, {TEXT_62, 0, 0}, {TEXT_63, 0, 0}, {TEXT_64, 0, 0},
    {TEXT_65, 0, 0}, {TEXT_66, 0, 0}, {TEXT_67, 0, 0}, {TEXT_68, 0, 0},
    {TEXT_69, 0, 0}, {TEXT_70, 0, 0}, {TEXT_71, 0, 0}, {TEXT_72, 0, 0},
    {TEXT_73, 0, 0}, {TEXT_74, 0, 0}, {TEXT_75, 0, 0},
};
static NJS_TEXLIST tutomsg_knuckles_texlist = {
#include "summary/tutorial/tutomsg_knuckles_texlist.inc"
};
static NJS_TEXNAME tutomsg_amy_textures[20] = {
    {TEXT_76, 0, 0}, {TEXT_77, 0, 0}, {TEXT_78, 0, 0}, {TEXT_79, 0, 0},
    {TEXT_80, 0, 0}, {TEXT_81, 0, 0}, {TEXT_82, 0, 0}, {TEXT_83, 0, 0},
    {TEXT_84, 0, 0}, {TEXT_85, 0, 0}, {TEXT_86, 0, 0}, {TEXT_87, 0, 0},
    {TEXT_88, 0, 0}, {TEXT_89, 0, 0}, {TEXT_90, 0, 0}, {TEXT_91, 0, 0},
    {TEXT_92, 0, 0}, {TEXT_93, 0, 0}, {TEXT_94, 0, 0}, {TEXT_95, 0, 0},
};
static NJS_TEXLIST tutomsg_amy_texlist = {
#include "summary/tutorial/tutomsg_amy_texlist.inc"
};
static NJS_TEXNAME tutomsg_e102_textures[25] = {
    {TEXT_96, 0, 0},  {TEXT_97, 0, 0},  {TEXT_98, 0, 0},  {TEXT_99, 0, 0},
    {TEXT_100, 0, 0}, {TEXT_101, 0, 0}, {TEXT_102, 0, 0}, {TEXT_103, 0, 0},
    {TEXT_104, 0, 0}, {TEXT_105, 0, 0}, {TEXT_106, 0, 0}, {TEXT_107, 0, 0},
    {TEXT_108, 0, 0}, {TEXT_109, 0, 0}, {TEXT_110, 0, 0}, {TEXT_111, 0, 0},
    {TEXT_112, 0, 0}, {TEXT_113, 0, 0}, {TEXT_114, 0, 0}, {TEXT_115, 0, 0},
    {TEXT_116, 0, 0}, {TEXT_117, 0, 0}, {TEXT_118, 0, 0}, {TEXT_119, 0, 0},
    {TEXT_120, 0, 0},
};
static NJS_TEXLIST tutomsg_e102_texlist = {
#include "summary/tutorial/tutomsg_e102_texlist.inc"
};
static NJS_TEXNAME tutomsg_big_textures[33] = {
    {TEXT_121, 0, 0}, {TEXT_122, 0, 0}, {TEXT_123, 0, 0}, {TEXT_124, 0, 0},
    {TEXT_125, 0, 0}, {TEXT_126, 0, 0}, {TEXT_127, 0, 0}, {TEXT_128, 0, 0},
    {TEXT_129, 0, 0}, {TEXT_130, 0, 0}, {TEXT_131, 0, 0}, {TEXT_132, 0, 0},
    {TEXT_133, 0, 0}, {TEXT_134, 0, 0}, {TEXT_135, 0, 0}, {TEXT_136, 0, 0},
    {TEXT_137, 0, 0}, {TEXT_138, 0, 0}, {TEXT_139, 0, 0}, {TEXT_140, 0, 0},
    {TEXT_141, 0, 0}, {TEXT_142, 0, 0}, {TEXT_143, 0, 0}, {TEXT_144, 0, 0},
    {TEXT_145, 0, 0}, {TEXT_146, 0, 0}, {TEXT_147, 0, 0}, {TEXT_148, 0, 0},
    {TEXT_149, 0, 0}, {TEXT_150, 0, 0}, {TEXT_151, 0, 0}, {TEXT_152, 0, 0},
    {TEXT_153, 0, 0},
};
static NJS_TEXLIST tutomsg_big_texlist = {
#include "summary/tutorial/tutomsg_big_texlist.inc"
};
/* each character's own texlist, by story. nothing reads them */
static NJS_TEXLIST *tutorial_texlists[TUTORIALS] = {
    &tutomsg_sonic_texlist, &tutomsg_tails_texlist, &tutomsg_knuckles_texlist,
    &tutomsg_amy_texlist,   &tutomsg_big_texlist,   &tutomsg_e102_texlist,
};
/* clang-format off */
static Sint16 sonic_layout_0[] = {
    5,
    0, 0, 8,
    1, 136, 0,
    2, 136, 24,
    3, 136, 64,
    4, 136, 96,
};
static Sint16 sonic_layout_1[] = {
    4,
    5, 0, 0,
    6, 0, 32,
    7, 0, 64,
    8, 0, 96,
};
static Sint16 sonic_layout_2[] = {
    5,
    9, 0, 0,
    10, 0, 32,
    11, 0, 64,
    12, 0, 96,
    13, 0, 128,
};
static Sint16 sonic_layout_3[] = {
    5,
    14, 0, 0,
    15, 0, 32,
    16, 0, 64,
    17, 0, 96,
    18, 0, 128,
};
static Sint16 sonic_layout_4[] = {
    5,
    19, 0, 0,
    20, 0, 32,
    21, 0, 64,
    22, 0, 96,
    23, 0, 128,
};
static Sint16 sonic_layout_5[] = {
    3,
    24, 0, 0,
    25, 0, 32,
    26, 0, 64,
};
static Sint16 sonic_layout_6[] = {
    2,
    27, 0, 0,
    28, 0, 32,
};
/* clang-format on */
static struct tutorial_page sonic_pages[7] = {
    {PAGE_ALWAYS, 400, 144, 0, {NULL, (struct screen_parts *)sonic_layout_0}},
    {PAGE_ALWAYS, 400, 128, 0, {NULL, (struct screen_parts *)sonic_layout_1}},
    {141, 400, 160, 0, {NULL, (struct screen_parts *)sonic_layout_2}},
    {142, 400, 160, 0, {NULL, (struct screen_parts *)sonic_layout_3}},
    {PAGE_ALWAYS, 400, 160, 0, {NULL, (struct screen_parts *)sonic_layout_4}},
    {PAGE_ALWAYS, 400, 96, 0, {NULL, (struct screen_parts *)sonic_layout_5}},
    {PAGE_ALWAYS, 400, 64, 0, {NULL, (struct screen_parts *)sonic_layout_6}},
};
/* clang-format off */
static Sint16 tails_layout_0[] = {
    4,
    0, 0, 8,
    1, 136, 0,
    2, 136, 24,
    4, 136, 96,
};
static Sint16 tails_layout_1[] = {
    1,
    2, 136, 64,
};
static Sint16 tails_layout_2[] = {
    2,
    6, 0, 32,
    7, 0, 64,
};
static Sint16 tails_layout_3[] = {
    4,
    4, 0, 0,
    5, 0, 96,
    6, 0, 128,
    7, 0, 160,
};
static Sint16 tails_layout_4[] = {
    2,
    8, 0, 0,
    9, 0, 32,
};
static Sint16 tails_layout_5[] = {
    1,
    10, 0, 32,
};
static Sint16 tails_layout_6[] = {
    3,
    10, 0, 0,
    11, 0, 64,
    12, 0, 96,
};
static Sint16 tails_layout_7[] = {
    1,
    24, 0, 0,
};
static Sint16 tails_layout_8[] = {
    2,
    13, 0, 32,
    14, 0, 64,
};
static Sint16 tails_layout_9[] = {
    2,
    15, 0, 0,
    16, 0, 32,
};
/* clang-format on */
static struct tutorial_page tails_pages[8] = {
    {PAGE_ALWAYS,
     400,
     144,
     0,
     {(struct screen_parts *)tails_layout_0,
      (struct screen_parts *)tails_layout_1}},
    {PAGE_ALWAYS,
     400,
     192,
     0,
     {(struct screen_parts *)tails_layout_2,
      (struct screen_parts *)tails_layout_3}},
    {PAGE_ALWAYS, 400, 64, 0, {NULL, (struct screen_parts *)tails_layout_4}},
    {219,
     400,
     128,
     0,
     {(struct screen_parts *)tails_layout_5,
      (struct screen_parts *)tails_layout_6}},
    {PAGE_ALWAYS, 400, 160, 0, {(struct screen_parts *)sonic_layout_4, NULL}},
    {PAGE_ALWAYS,
     400,
     96,
     0,
     {(struct screen_parts *)tails_layout_7,
      (struct screen_parts *)tails_layout_8}},
    {PAGE_ALWAYS, 400, 64, 0, {NULL, (struct screen_parts *)tails_layout_9}},
    {PAGE_ALWAYS, 400, 64, 0, {(struct screen_parts *)sonic_layout_6, NULL}},
};
/* clang-format off */
static Sint16 knuckles_layout_0[] = {
    1,
    1, 136, 64,
};
static Sint16 knuckles_layout_1[] = {
    1,
    6, 0, 32,
};
static Sint16 knuckles_layout_2[] = {
    4,
    2, 0, 0,
    26, 0, 64,
    4, 0, 96,
    5, 0, 128,
};
static Sint16 knuckles_layout_3[] = {
    4,
    6, 0, 0,
    7, 0, 32,
    8, 0, 64,
    9, 0, 96,
};
static Sint16 knuckles_layout_4[] = {
    4,
    10, 0, 0,
    11, 0, 32,
    12, 0, 64,
    13, 0, 96,
};
static Sint16 knuckles_layout_5[] = {
    2,
    10, 0, 32,
    11, 0, 64,
};
static Sint16 knuckles_layout_6[] = {
    3,
    14, 0, 0,
    15, 0, 96,
    16, 0, 128,
};
static Sint16 knuckles_layout_7[] = {
    1,
    24, 0, 0,
};
static Sint16 knuckles_layout_8[] = {
    2,
    17, 0, 32,
    18, 0, 64,
};
static Sint16 knuckles_layout_9[] = {
    6,
    19, 0, 0,
    20, 0, 32,
    21, 0, 64,
    22, 0, 128,
    23, 320, 128,
    0, 64, 120,
};
static Sint16 knuckles_layout_10[] = {
    2,
    24, 0, 0,
    25, 0, 32,
};
/* clang-format on */
static struct tutorial_page knuckles_pages[9] = {
    {PAGE_ALWAYS,
     400,
     144,
     0,
     {(struct screen_parts *)tails_layout_0,
      (struct screen_parts *)knuckles_layout_0}},
    {PAGE_ALWAYS,
     400,
     160,
     0,
     {(struct screen_parts *)knuckles_layout_1,
      (struct screen_parts *)knuckles_layout_2}},
    {PAGE_ALWAYS,
     400,
     128,
     0,
     {NULL, (struct screen_parts *)knuckles_layout_3}},
    {281, 400, 128, 0, {NULL, (struct screen_parts *)knuckles_layout_4}},
    {282,
     400,
     160,
     0,
     {(struct screen_parts *)knuckles_layout_5,
      (struct screen_parts *)knuckles_layout_6}},
    {PAGE_ALWAYS, 400, 160, 0, {(struct screen_parts *)sonic_layout_4, NULL}},
    {PAGE_ALWAYS,
     400,
     96,
     0,
     {(struct screen_parts *)knuckles_layout_7,
      (struct screen_parts *)knuckles_layout_8}},
    {PAGE_ALWAYS,
     400,
     192,
     0,
     {NULL, (struct screen_parts *)knuckles_layout_9}},
    {PAGE_ALWAYS,
     400,
     64,
     0,
     {NULL, (struct screen_parts *)knuckles_layout_10}},
};
/* clang-format off */
static Sint16 amy_layout_0[] = {
    3,
    0, 0, 8,
    1, 136, 0,
    2, 136, 24,
};
static Sint16 amy_layout_1[] = {
    2,
    0, 136, 64,
    1, 136, 96,
};
static Sint16 amy_layout_2[] = {
    4,
    2, 0, 0,
    3, 0, 32,
    4, 0, 64,
    5, 0, 96,
};
static Sint16 amy_layout_3[] = {
    4,
    6, 0, 0,
    7, 0, 32,
    8, 0, 64,
    9, 0, 96,
};
static Sint16 amy_layout_4[] = {
    4,
    10, 0, 0,
    11, 0, 32,
    12, 0, 64,
    13, 0, 96,
};
static Sint16 amy_layout_5[] = {
    1,
    24, 0, 0,
};
static Sint16 amy_layout_6[] = {
    2,
    15, 0, 32,
    16, 0, 64,
};
static Sint16 amy_layout_7[] = {
    3,
    17, 0, 0,
    18, 0, 32,
    19, 0, 64,
};
/* clang-format on */
static struct tutorial_page amy_pages[7] = {
    {PAGE_ALWAYS,
     400,
     144,
     0,
     {(struct screen_parts *)amy_layout_0,
      (struct screen_parts *)amy_layout_1}},
    {PAGE_ALWAYS, 400, 128, 0, {NULL, (struct screen_parts *)amy_layout_2}},
    {PAGE_ALWAYS, 400, 128, 0, {NULL, (struct screen_parts *)amy_layout_3}},
    {338, 400, 128, 0, {NULL, (struct screen_parts *)amy_layout_4}},
    {PAGE_ALWAYS, 400, 160, 0, {(struct screen_parts *)sonic_layout_4, NULL}},
    {PAGE_ALWAYS,
     400,
     96,
     0,
     {(struct screen_parts *)amy_layout_5,
      (struct screen_parts *)amy_layout_6}},
    {PAGE_ALWAYS, 400, 96, 0, {NULL, (struct screen_parts *)amy_layout_7}},
};
/* clang-format off */
static Sint16 e102_layout_0[] = {
    2,
    2, 136, 64,
    3, 136, 96,
};
static Sint16 e102_layout_1[] = {
    1,
    10, 0, 32,
};
static Sint16 e102_layout_2[] = {
    6,
    4, 0, 0,
    5, 0, 64,
    6, 0, 96,
    7, 0, 128,
    8, 0, 160,
    9, 0, 192,
};
static Sint16 e102_layout_3[] = {
    4,
    10, 0, 0,
    11, 0, 32,
    12, 0, 64,
    13, 0, 96,
};
static Sint16 e102_layout_4[] = {
    5,
    14, 0, 0,
    15, 0, 32,
    16, 0, 64,
    17, 0, 96,
    18, 0, 128,
};
static Sint16 e102_layout_5[] = {
    1,
    24, 0, 0,
};
static Sint16 e102_layout_6[] = {
    2,
    19, 0, 32,
    20, 0, 64,
};
static Sint16 e102_layout_7[] = {
    3,
    21, 0, 0,
    22, 0, 32,
    0, 288, 64,
};
static Sint16 e102_layout_8[] = {
    3,
    23, 0, 0,
    24, 0, 32,
    1, 96, 64,
};
/* clang-format on */
static struct tutorial_page e102_pages[8] = {
    {PAGE_ALWAYS,
     400,
     144,
     0,
     {(struct screen_parts *)amy_layout_0,
      (struct screen_parts *)e102_layout_0}},
    {PAGE_ALWAYS,
     400,
     224,
     0,
     {(struct screen_parts *)e102_layout_1,
      (struct screen_parts *)e102_layout_2}},
    {PAGE_ALWAYS, 400, 128, 0, {NULL, (struct screen_parts *)e102_layout_3}},
    {393, 400, 160, 0, {NULL, (struct screen_parts *)e102_layout_4}},
    {PAGE_ALWAYS, 400, 160, 0, {(struct screen_parts *)sonic_layout_4, NULL}},
    {PAGE_ALWAYS,
     400,
     96,
     0,
     {(struct screen_parts *)e102_layout_5,
      (struct screen_parts *)e102_layout_6}},
    {PAGE_ALWAYS, 400, 128, 0, {NULL, (struct screen_parts *)e102_layout_7}},
    {PAGE_ALWAYS, 400, 128, 0, {NULL, (struct screen_parts *)e102_layout_8}},
};
/* clang-format off */
static Sint16 big_layout_0[] = {
    1,
    2, 136, 64,
};
static Sint16 big_layout_1[] = {
    1,
    10, 0, 32,
};
static Sint16 big_layout_2[] = {
    5,
    4, 0, 0,
    5, 0, 64,
    6, 0, 96,
    7, 0, 128,
    8, 0, 160,
};
static Sint16 big_layout_3[] = {
    1,
    10, 0, 32,
};
static Sint16 big_layout_4[] = {
    3,
    9, 0, 0,
    10, 0, 64,
    11, 0, 96,
};
static Sint16 big_layout_5[] = {
    3,
    12, 0, 0,
    13, 0, 32,
    14, 0, 64,
};
static Sint16 big_layout_6[] = {
    5,
    15, 0, 0,
    16, 0, 32,
    17, 0, 64,
    18, 0, 96,
    32, 0, 128,
};
static Sint16 big_layout_7[] = {
    1,
    19, 0, 160,
};
static Sint16 big_layout_8[] = {
    2,
    20, 0, 32,
    21, 0, 64,
};
static Sint16 big_layout_9[] = {
    2,
    22, 0, 0,
    23, 0, 32,
};
static Sint16 big_layout_10[] = {
    3,
    24, 0, 0,
    25, 0, 32,
    26, 0, 64,
};
static Sint16 big_layout_11[] = {
    6,
    27, 0, 0,
    28, 0, 32,
    29, 0, 64,
    30, 0, 96,
    31, 0, 128,
    0, 304, 0,
};
/* clang-format on */
static struct tutorial_page big_pages[10] = {
    {PAGE_ALWAYS,
     400,
     144,
     0,
     {(struct screen_parts *)tails_layout_0,
      (struct screen_parts *)big_layout_0}},
    {PAGE_ALWAYS,
     400,
     192,
     0,
     {(struct screen_parts *)big_layout_1,
      (struct screen_parts *)big_layout_2}},
    {PAGE_ALWAYS,
     400,
     128,
     0,
     {(struct screen_parts *)big_layout_3,
      (struct screen_parts *)big_layout_4}},
    {464, 400, 96, 0, {NULL, (struct screen_parts *)big_layout_5}},
    {PAGE_ALWAYS, 400, 160, 0, {NULL, (struct screen_parts *)big_layout_6}},
    {PAGE_ALWAYS,
     400,
     192,
     0,
     {(struct screen_parts *)sonic_layout_4,
      (struct screen_parts *)big_layout_7}},
    {PAGE_ALWAYS,
     400,
     96,
     0,
     {(struct screen_parts *)tails_layout_7,
      (struct screen_parts *)big_layout_8}},
    {PAGE_ALWAYS, 400, 64, 0, {NULL, (struct screen_parts *)big_layout_9}},
    {PAGE_ALWAYS, 400, 96, 0, {NULL, (struct screen_parts *)big_layout_10}},
    {PAGE_ALWAYS, 400, 208, 0, {NULL, (struct screen_parts *)big_layout_11}},
};
static struct tutorial tutorials[TUTORIALS] = {
    {7,
     5,
     sonic_pages,
     {NULL, &tutomsg_sonic_texlist},
     {NULL, "tutomsg_sonic"}},
    {8,
     5,
     tails_pages,
     {&tutomsg_sonic_texlist, &tutomsg_tails_texlist},
     {"tutomsg_sonic", "tutomsg_tails"}},
    {9,
     6,
     knuckles_pages,
     {&tutomsg_sonic_texlist, &tutomsg_knuckles_texlist},
     {"tutomsg_sonic", "tutomsg_knuckles"}},
    {7,
     5,
     amy_pages,
     {&tutomsg_sonic_texlist, &tutomsg_amy_texlist},
     {"tutomsg_sonic", "tutomsg_amy"}},
    {10,
     6,
     big_pages,
     {&tutomsg_sonic_texlist, &tutomsg_big_texlist},
     {"tutomsg_sonic", "tutomsg_big"}},
    {8,
     5,
     e102_pages,
     {&tutomsg_sonic_texlist, &tutomsg_e102_texlist},
     {"tutomsg_sonic", "tutomsg_e102"}},
};

static void tutorial_draw(struct task *task);
static void tutorial_step(struct task *task);
/* @hack spelling: tutorial_step's call to tutorial_controls expands inline
 * (0x8c9018da to 0x8c9018e0). SHC still writes tutorial_controls' body,
 * since it has external linkage. as a call, 80 of tutorial_step's 80 words
 * differ */
#pragma inline(tutorial_controls)

void tutorial_load_textures(int story)
{
    char *files[2];
    NJS_TEXLIST *texlists[2];
    int i;

    for (i = 0; i < 2; i++) {
        files[i] = tutorials[story].files[i];
        texlists[i] = tutorials[story].texlists[i];
        if (files[i] && texlists[i]) {
            texture_load_pvm(files[i], texlists[i]);
        }
    }
}

void tutorial_release_textures(int story)
{
    char *files[2];
    NJS_TEXLIST *texlists[2];
    int i;

    for (i = 0; i < 2; i++) {
        files[i] = tutorials[story].files[i];
        texlists[i] = tutorials[story].texlists[i];
        if (files[i] && texlists[i]) {
            njReleaseTexture(texlists[i]);
        }
    }
}

void tutorial_parts_draw(struct screen_parts *picture, int unused, Float x,
                         Float y, Float z, Float scale_x, Float scale_y)
{
    struct screen_part *part = picture->parts;
    int i;
    Float part_x;
    Float part_y;

    for (i = 0; i < picture->count; i++) {
        part_x = (part->x * scale_x) + x;
        part_y = y + (part->y * scale_y);
        screen_quad_draw_scaled(part->texture, part_x, part_y, z, scale_x,
                                scale_y);
        part++;
    }
}

static void tutorial_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct tutorial_pager *pager = task->extra;
    struct tutorial *tutorial = TUTORIAL(work);
    struct tutorial_page *page;
    NJS_TEXLIST *texlist;
    struct screen_parts *parts;
    int i;

    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    page = &tutorial->pages[pager->page];
    for (i = 0; i < 2; i++) {
        texlist = tutorial->texlists[i];
        if (texlist) {
            njSetTexture(texlist);
            parts = page->parts[i];
            if (parts) {
                tutorial_parts_draw(parts, 0, work->pos.x, work->pos.y,
                                    work->pos.z - 10.0F, work->scl.x,
                                    work->scl.y);
            }
        }
    }
    screen_quad_colors(BOX_TOP, BOX_BOTTOM, BOX_TOP, BOX_BOTTOM);
    screen_box_draw(work->pos.x - 16.0F, work->pos.y - 16.0F, work->pos.z,
                    (page->width * work->scl.x) + 32.0F,
                    (page->height * work->scl.y) + 32.0F);
}

static void tutorial_step(struct task *task)
{
    struct task_work *work = task->work;
    struct tutorial_pager *pager = task->extra;
    Sint8 was_past;

    switch (work->mode) {
    case TUTORIAL_SHOWN:
        work->scl.y = 1.0F;
        work->scl.x = 1.0F;
        break;
    case TUTORIAL_TURN:
        work->scl.y -= pager->speed;
        if (work->scl.y <= 0.0F) {
            work->scl.y = 0.0F;
            work->mode = TUTORIAL_OPEN;
            pager->page = pager->next;
        }
        break;
    case TUTORIAL_OPEN:
        work->scl.y += pager->speed;
        if (work->scl.y >= 1.0F) {
            work->scl.y = 1.0F;
            work->mode = TUTORIAL_SHOWN;
        }
        break;
    }
    work->flags &= ~TUTORIAL_CROSSED;
    was_past = pager->past_controls;
    if (pager->page < tutorial_controls(task)) {
        pager->past_controls = 0;
    } else {
        pager->past_controls = 1;
    }
    if (was_past != pager->past_controls) {
        work->flags |= TUTORIAL_CROSSED;
    }
}

struct task *tutorial_create(int level, int story, NJS_POINT3 *pos)
{
    NJS_POINT3 at;
    struct task *task;
    struct tutorial_pager *pager;

    at = tutorial_pos;
    task = task_create(TASK_WORK, level, tutorial_step);
    if (task) {
        task->extra = heap_calloc(1, sizeof(struct tutorial_pager));
        pager = task->extra;
        if (pos) {
            task->work->pos = *pos;
        } else {
            task->work->pos = at;
        }
        task->work->scl.x = task->work->scl.y = 1.0F;
        pager->tutorial = task->work->vars.p[2] = &tutorials[story];
        pager->speed = 0.1F;
        pager->past_controls = pager->unk_19 = 0;
        task->draw = tutorial_draw;
    }
    return task;
}

int tutorial_find_page(struct task *task, int direction)
{
    struct task_work *work = task->work;
    struct tutorial_pager *pager = task->extra;
    struct tutorial *tutorial;
    struct tutorial_page *page;
    int found;
    int next;

    pager->edge = work->id = TUTORIAL_INSIDE;
    tutorial = pager->tutorial;
    found = pager->page;
    switch (direction) {
    case 1:
        for (next = found + 1; next < tutorial->count; next++) {
            page = &tutorial->pages[next];
            /* @hack spelling: this loop's test of the page's flag, split into
             * an if and an else if that each break. with the other loop's
             * split, it keeps pager in R10 and event_flag_check's address in R9
             * (0x8c901a12 and 0x8c901a20); as a single test with ||, 52 of 92
             * words differ */
            if (page->flag == PAGE_ALWAYS) {
                break;
            } else if (event_flag_check(page->flag)) {
                break;
            }
        }
        if (next < tutorial->count) {
            found = next;
        } else {
            pager->edge = TUTORIAL_LAST;
        }
        break;
    case -1:
        for (next = found - 1; next >= 0; next--) {
            page = &tutorial->pages[next];
            /* @hack spelling: this loop's test of the page's flag, split into
             * an if and an else if that each break. with the other loop's
             * split, it keeps pager in R10 and event_flag_check's address in R9
             * (0x8c901a12 and 0x8c901a20); as a single test with ||, 34 of 92
             * words differ */
            if (page->flag == PAGE_ALWAYS) {
                break;
            } else if (event_flag_check(page->flag)) {
                break;
            }
        }
        if (next >= 0) {
            found = next;
        } else {
            pager->edge = TUTORIAL_FIRST;
        }
        break;
    }
    return found;
}

int tutorial_controls(struct task *task)
{
    struct tutorial *tutorial = TUTORIAL(task->work);

    return tutorial->controls;
}

int tutorial_get_page(struct task *task)
{
    struct tutorial_pager *pager = task->extra;

    return pager->page;
}

void tutorial_set_page(struct task *task, int page)
{
    struct tutorial_pager *pager = task->extra;

    pager->page = page;
}

void tutorial_set_next(struct task *task, int page)
{
    struct tutorial_pager *pager = task->extra;

    pager->next = page;
}

int tutorial_get_mode(struct task *task)
{
    return task->work->mode;
}

void tutorial_set_mode(struct task *task, int mode)
{
    task->work->mode = mode;
}

enum tutorial_edge tutorial_get_edge(struct task *task)
{
    struct tutorial_pager *pager = task->extra;

    return pager->edge;
}

void tutorial_turn(struct task *task, int page)
{
    struct tutorial_pager *pager = task->extra;

    task->work->mode = TUTORIAL_TURN;
    pager->next = page;
}
