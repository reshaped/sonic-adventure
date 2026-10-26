/* the sound test's music: each kind's tracks, and the table of the kinds'
 * lists */
#include "advertise/sound_test_music.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 23
#define TEXT_3_LENGTH 23
#define TEXT_4_LENGTH 26
#define TEXT_5_LENGTH 20
#define TEXT_6_LENGTH 34
#define TEXT_7_LENGTH 16
#define TEXT_8_LENGTH 20
#define TEXT_9_LENGTH 22
#define TEXT_10_LENGTH 16
#define TEXT_11_LENGTH 15
#define TEXT_12_LENGTH 20
#define TEXT_13_LENGTH 20
#define TEXT_14_LENGTH 25
#define TEXT_15_LENGTH 21
#define TEXT_16_LENGTH 21
#define TEXT_17_LENGTH 16
#define TEXT_18_LENGTH 23
#define TEXT_19_LENGTH 20
#define TEXT_20_LENGTH 20
#define TEXT_21_LENGTH 22
#define TEXT_22_LENGTH 32
#define TEXT_23_LENGTH 25
#define TEXT_24_LENGTH 15
#define TEXT_25_LENGTH 15
#define TEXT_26_LENGTH 18
#define TEXT_27_LENGTH 18
#define TEXT_28_LENGTH 27
#define TEXT_29_LENGTH 24
#define TEXT_30_LENGTH 21
#define TEXT_31_LENGTH 21
#define TEXT_32_LENGTH 27
#define TEXT_33_LENGTH 25
#define TEXT_34_LENGTH 26
#define TEXT_35_LENGTH 25
#define TEXT_36_LENGTH 28
#define TEXT_37_LENGTH 28
#define TEXT_38_LENGTH 19
#define TEXT_39_LENGTH 19
#define TEXT_40_LENGTH 16
#define TEXT_41_LENGTH 16
#define TEXT_42_LENGTH 11
#define TEXT_43_LENGTH 17
#define TEXT_44_LENGTH 24
#define TEXT_45_LENGTH 24
#define TEXT_46_LENGTH 20
#define TEXT_47_LENGTH 18
#define TEXT_48_LENGTH 10
#define TEXT_49_LENGTH 12
#define TEXT_50_LENGTH 11
#define TEXT_51_LENGTH 14
#define TEXT_52_LENGTH 14
#define TEXT_53_LENGTH 11
#define TEXT_54_LENGTH 11
#define TEXT_55_LENGTH 23
#define TEXT_56_LENGTH 26
#define TEXT_57_LENGTH 25
#define TEXT_58_LENGTH 19
#define TEXT_59_LENGTH 25
#define TEXT_60_LENGTH 30
#define TEXT_61_LENGTH 19
#define TEXT_62_LENGTH 26
#define TEXT_63_LENGTH 7
#define TEXT_64_LENGTH 6
#define TEXT_65_LENGTH 7
#define TEXT_66_LENGTH 9
#define TEXT_67_LENGTH 8
#define TEXT_68_LENGTH 21
#define TEXT_69_LENGTH 16
#define TEXT_70_LENGTH 16
#define TEXT_71_LENGTH 19
#define TEXT_72_LENGTH 14
#define TEXT_73_LENGTH 16
#define TEXT_74_LENGTH 14
#define TEXT_75_LENGTH 5
#define TEXT_76_LENGTH 10
#define TEXT_77_LENGTH 18
#define TEXT_78_LENGTH 5
#define TEXT_79_LENGTH 14
#define TEXT_80_LENGTH 14
#define TEXT_81_LENGTH 17
#define TEXT_82_LENGTH 12
#define TEXT_83_LENGTH 14
#define TEXT_84_LENGTH 12
#define TEXT_85_LENGTH 13
#define TEXT_86_LENGTH 8
#define TEXT_87_LENGTH 8
#define TEXT_88_LENGTH 9
#define TEXT_89_LENGTH 8
#define TEXT_90_LENGTH 10
#define TEXT_91_LENGTH 11
#define TEXT_92_LENGTH 6
#define TEXT_93_LENGTH 4
#define TEXT_94_LENGTH 5
#define TEXT_95_LENGTH 9
#define TEXT_96_LENGTH 20
#define TEXT_97_LENGTH 17
#define TEXT_98_LENGTH 24
#define TEXT_99_LENGTH 17
#define TEXT_100_LENGTH 8
#define TEXT_101_LENGTH 8
#define TEXT_102_LENGTH 8
#define TEXT_103_LENGTH 8
#define TEXT_104_LENGTH 8
#include "advertise/sound_test_music/text.inc"
#include "advertise/sound_test_music/values.inc"

static const struct sound_test_track character_tracks[11] = {
    {TEXT_1, VALUE_1},   {TEXT_2, VALUE_2},   {TEXT_3, VALUE_3},
    {TEXT_4, VALUE_4},   {TEXT_5, VALUE_5},   {TEXT_6, VALUE_6},
    {TEXT_7, VALUE_7},   {TEXT_8, VALUE_8},   {TEXT_9, VALUE_9},
    {TEXT_10, VALUE_10}, {TEXT_11, VALUE_11},
};
static const struct sound_test_track action_tracks[40] = {
    {TEXT_12, VALUE_12}, {TEXT_13, VALUE_13}, {TEXT_14, VALUE_14},
    {TEXT_15, VALUE_15}, {TEXT_16, VALUE_16}, {TEXT_17, VALUE_17},
    {TEXT_18, VALUE_18}, {TEXT_19, VALUE_19}, {TEXT_20, VALUE_20},
    {TEXT_21, VALUE_21}, {TEXT_22, VALUE_22}, {TEXT_23, VALUE_23},
    {TEXT_24, VALUE_24}, {TEXT_25, VALUE_25}, {TEXT_26, VALUE_26},
    {TEXT_27, VALUE_27}, {TEXT_28, VALUE_28}, {TEXT_29, VALUE_29},
    {TEXT_30, VALUE_30}, {TEXT_31, VALUE_31}, {TEXT_32, VALUE_32},
    {TEXT_33, VALUE_33}, {TEXT_34, VALUE_34}, {TEXT_35, VALUE_35},
    {TEXT_36, VALUE_36}, {TEXT_37, VALUE_37}, {TEXT_38, VALUE_38},
    {TEXT_39, VALUE_39}, {TEXT_40, VALUE_40}, {TEXT_41, VALUE_41},
    {TEXT_42, VALUE_42}, {TEXT_43, VALUE_43}, {TEXT_44, VALUE_44},
    {TEXT_45, VALUE_45}, {TEXT_46, VALUE_46}, {TEXT_47, VALUE_47},
    {TEXT_48, VALUE_48}, {TEXT_49, VALUE_49}, {TEXT_50, VALUE_50},
    {TEXT_51, VALUE_51},
};
static const struct sound_test_track adventure_tracks[11] = {
    {TEXT_52, VALUE_52}, {TEXT_53, VALUE_53}, {TEXT_54, VALUE_54},
    {TEXT_55, VALUE_55}, {TEXT_56, VALUE_56}, {TEXT_57, VALUE_57},
    {TEXT_58, VALUE_58}, {TEXT_59, VALUE_59}, {TEXT_60, VALUE_60},
    {TEXT_61, VALUE_61}, {TEXT_62, VALUE_62},
};
static const struct sound_test_track event_tracks[12] = {
    {TEXT_63, VALUE_63}, {TEXT_64, VALUE_64}, {TEXT_65, VALUE_65},
    {TEXT_66, VALUE_66}, {TEXT_67, VALUE_67}, {TEXT_68, VALUE_68},
    {TEXT_69, VALUE_69}, {TEXT_70, VALUE_70}, {TEXT_71, VALUE_71},
    {TEXT_72, VALUE_72}, {TEXT_73, VALUE_73}, {TEXT_74, VALUE_74},
};
static const struct sound_test_track other_tracks[10] = {
    {TEXT_75, VALUE_75}, {TEXT_76, VALUE_76}, {TEXT_77, VALUE_77},
    {TEXT_78, VALUE_78}, {TEXT_79, VALUE_79}, {TEXT_80, VALUE_80},
    {TEXT_81, VALUE_81}, {TEXT_82, VALUE_82}, {TEXT_83, VALUE_83},
    {TEXT_84, VALUE_84},
};
static const struct sound_test_track jingle_tracks[20] = {
    {TEXT_85, VALUE_85},   {TEXT_86, VALUE_86},   {TEXT_87, VALUE_87},
    {TEXT_88, VALUE_88},   {TEXT_89, VALUE_89},   {TEXT_90, VALUE_90},
    {TEXT_91, VALUE_91},   {TEXT_92, VALUE_92},   {TEXT_93, VALUE_93},
    {TEXT_94, VALUE_94},   {TEXT_95, VALUE_95},   {TEXT_96, VALUE_96},
    {TEXT_97, VALUE_97},   {TEXT_98, VALUE_98},   {TEXT_99, VALUE_99},
    {TEXT_100, VALUE_100}, {TEXT_101, VALUE_101}, {TEXT_102, VALUE_102},
    {TEXT_103, VALUE_103}, {TEXT_104, VALUE_104},
};
const struct sound_test_list sound_test_music[SOUND_TEST_LISTS] = {
#include "advertise/sound_test_music/sound_test_music.inc"
};
