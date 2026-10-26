/* the stage's land: its table, its entries, and its textures' names and
 * texlist. the table names the pvm the textures load from */
#include <ninja.h>

/* only the full build's entries name the objects it declares */
#include "b_egm2/dat_0c9462c0.h" /* NOLINT(misc-include-cleaner) */
#include "b_egm2/land.h"
#include "collision/world.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 12
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 6
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 10
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 10
#define TEXT_21_LENGTH 10
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 10
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 10
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 11
#define TEXT_31_LENGTH 11
#define TEXT_32_LENGTH 11
#define TEXT_33_LENGTH 9
#define TEXT_34_LENGTH 12
#define TEXT_35_LENGTH 12
#define TEXT_36_LENGTH 14
#define TEXT_37_LENGTH 10
#define TEXT_38_LENGTH 8
#define TEXT_39_LENGTH 10
#define TEXT_40_LENGTH 10
#define TEXT_41_LENGTH 10
#define TEXT_42_LENGTH 10
#define TEXT_43_LENGTH 10
#define TEXT_44_LENGTH 10
#define TEXT_45_LENGTH 10
#define TEXT_46_LENGTH 10
#define TEXT_47_LENGTH 10
#define TEXT_48_LENGTH 10
#define TEXT_49_LENGTH 12
#define TEXT_50_LENGTH 10
#define TEXT_51_LENGTH 10
#define TEXT_52_LENGTH 10
#define TEXT_53_LENGTH 8
#define TEXT_54_LENGTH 7
#define TEXT_55_LENGTH 15
#define TEXT_56_LENGTH 12
#define TEXT_57_LENGTH 10
#define TEXT_58_LENGTH 10
#define TEXT_59_LENGTH 10
#define TEXT_60_LENGTH 10
#define TEXT_61_LENGTH 10
#define TEXT_62_LENGTH 10
#define TEXT_63_LENGTH 12
#define TEXT_64_LENGTH 11
#define TEXT_65_LENGTH 11
#define TEXT_66_LENGTH 7
#define TEXT_67_LENGTH 6
#define TEXT_68_LENGTH 9
#define TEXT_69_LENGTH 9
#define TEXT_70_LENGTH 9
#define TEXT_71_LENGTH 9
#define TEXT_72_LENGTH 7
#define TEXT_73_LENGTH 9
#define TEXT_74_LENGTH 9
#define TEXT_75_LENGTH 9
#define TEXT_76_LENGTH 6
#define TEXT_77_LENGTH 6
#define TEXT_78_LENGTH 10
#define TEXT_79_LENGTH 8
#define TEXT_80_LENGTH 8
#define TEXT_81_LENGTH 8
#define TEXT_82_LENGTH 9
#define TEXT_83_LENGTH 7
#define TEXT_84_LENGTH 7
#define TEXT_85_LENGTH 9
#define TEXT_86_LENGTH 8
#define TEXT_87_LENGTH 10
#define TEXT_88_LENGTH 10
#define TEXT_89_LENGTH 10
#define TEXT_90_LENGTH 10
#define TEXT_91_LENGTH 10
#define TEXT_92_LENGTH 10
#define TEXT_93_LENGTH 10
#define TEXT_94_LENGTH 10
#define TEXT_95_LENGTH 10
#define TEXT_96_LENGTH 10
#define TEXT_97_LENGTH 10
#define TEXT_98_LENGTH 10
#define TEXT_99_LENGTH 10
#define TEXT_100_LENGTH 10
#define TEXT_101_LENGTH 10
#define TEXT_102_LENGTH 10
#define TEXT_103_LENGTH 10
#define TEXT_104_LENGTH 10
#define TEXT_105_LENGTH 10
#define TEXT_106_LENGTH 10
#define TEXT_107_LENGTH 10
#define TEXT_108_LENGTH 10
#define TEXT_109_LENGTH 10
#define TEXT_110_LENGTH 10
#define TEXT_111_LENGTH 10
#define TEXT_112_LENGTH 10
#define TEXT_113_LENGTH 10
#define TEXT_114_LENGTH 10
#define TEXT_115_LENGTH 10
#define TEXT_116_LENGTH 10
#define TEXT_117_LENGTH 10
#define TEXT_118_LENGTH 10
#define TEXT_119_LENGTH 10
#define TEXT_120_LENGTH 10
#define TEXT_121_LENGTH 10
#define TEXT_122_LENGTH 10
#define TEXT_123_LENGTH 10
#define TEXT_124_LENGTH 10
#define TEXT_125_LENGTH 9
#define TEXT_126_LENGTH 6
#define TEXT_127_LENGTH 9
#define TEXT_128_LENGTH 6
#define TEXT_129_LENGTH 6
#define TEXT_130_LENGTH 6
#define TEXT_131_LENGTH 9
#define TEXT_132_LENGTH 8
#define TEXT_133_LENGTH 7
#define TEXT_134_LENGTH 8
#define TEXT_135_LENGTH 7
#define TEXT_136_LENGTH 9
#define TEXT_137_LENGTH 6
#define TEXT_138_LENGTH 9
#define TEXT_139_LENGTH 10
#define TEXT_140_LENGTH 9
#define TEXT_141_LENGTH 8
#define TEXT_142_LENGTH 9
#define TEXT_143_LENGTH 6
#define TEXT_144_LENGTH 9
#define TEXT_145_LENGTH 6
#define TEXT_146_LENGTH 9
#define TEXT_147_LENGTH 6
#define TEXT_148_LENGTH 9
#define TEXT_149_LENGTH 10
#define TEXT_150_LENGTH 10
#define TEXT_151_LENGTH 10
#define TEXT_152_LENGTH 10
#define TEXT_153_LENGTH 10
#define TEXT_154_LENGTH 10
#define TEXT_155_LENGTH 10
#define TEXT_156_LENGTH 10
#define TEXT_157_LENGTH 12
#define TEXT_158_LENGTH 12
#define TEXT_159_LENGTH 12
#define TEXT_160_LENGTH 10
#define TEXT_161_LENGTH 10
#define TEXT_162_LENGTH 12
#define TEXT_163_LENGTH 10
#define TEXT_164_LENGTH 10
#define TEXT_165_LENGTH 10
#define TEXT_166_LENGTH 8
#define TEXT_167_LENGTH 10
#define TEXT_168_LENGTH 12
#define TEXT_169_LENGTH 12
#define TEXT_170_LENGTH 14
#define TEXT_171_LENGTH 13
#define TEXT_172_LENGTH 10
#define TEXT_173_LENGTH 12
#define TEXT_174_LENGTH 12
#define TEXT_175_LENGTH 9
#define TEXT_176_LENGTH 9
#define TEXT_177_LENGTH 6
#define TEXT_178_LENGTH 6
#define TEXT_179_LENGTH 6
#define TEXT_180_LENGTH 8
#define TEXT_181_LENGTH 9
#define TEXT_182_LENGTH 9
#define TEXT_183_LENGTH 8
#define TEXT_184_LENGTH 8
#define TEXT_185_LENGTH 9
#define TEXT_186_LENGTH 9
#define TEXT_187_LENGTH 9
#define TEXT_188_LENGTH 7
#define TEXT_189_LENGTH 8
#define TEXT_190_LENGTH 7
#define TEXT_191_LENGTH 7
#define TEXT_192_LENGTH 7
#define TEXT_193_LENGTH 9
#define TEXT_194_LENGTH 6
#define TEXT_195_LENGTH 9
#define TEXT_196_LENGTH 9
#define TEXT_197_LENGTH 13
#define TEXT_198_LENGTH 13
#define TEXT_199_LENGTH 5
#define TEXT_200_LENGTH 10
#define TEXT_201_LENGTH 12
#define TEXT_202_LENGTH 10
#define TEXT_203_LENGTH 10
#define TEXT_204_LENGTH 10
#define TEXT_205_LENGTH 11
#define TEXT_206_LENGTH 7
#define TEXT_207_LENGTH 7
#define TEXT_208_LENGTH 7
#define TEXT_209_LENGTH 8
#define TEXT_210_LENGTH 7
#define TEXT_211_LENGTH 7
#define TEXT_212_LENGTH 7
#define TEXT_213_LENGTH 7
#define TEXT_214_LENGTH 7
#define TEXT_215_LENGTH 7
#define TEXT_216_LENGTH 9
#define TEXT_217_LENGTH 8
#define TEXT_218_LENGTH 7
#define TEXT_219_LENGTH 7
#define TEXT_220_LENGTH 9
#define TEXT_221_LENGTH 6
#define TEXT_222_LENGTH 9
#define TEXT_223_LENGTH 7
#define TEXT_224_LENGTH 7
#define TEXT_225_LENGTH 8
#define TEXT_226_LENGTH 8
#define TEXT_227_LENGTH 9
#define TEXT_228_LENGTH 8
#define TEXT_229_LENGTH 10
#define TEXT_230_LENGTH 7
#define TEXT_231_LENGTH 12
#include "b_egm2/land/text.inc"

/* the objects of the land's other entries, one to each, in their order.
 * dat_0c9462c0.h declares the first two entries' */
extern NJS_OBJECT dat_0c946948;
extern NJS_OBJECT dat_0c946d44;
extern NJS_OBJECT dat_0c946f60;
extern NJS_OBJECT dat_0c94712c;
extern NJS_OBJECT dat_0c947744;
extern NJS_OBJECT dat_0c947d5c;
extern NJS_OBJECT dat_0c948374;
extern NJS_OBJECT dat_0c94898c;
extern NJS_OBJECT dat_0c948f14;
extern NJS_OBJECT dat_0c9490ec;
extern NJS_OBJECT dat_0c949924;
extern NJS_OBJECT dat_0c949d38;
extern NJS_OBJECT dat_0c94a17c;
extern NJS_OBJECT dat_0c94a45c;
extern NJS_OBJECT dat_0c94a73c;
extern NJS_OBJECT dat_0c94aa1c;
extern NJS_OBJECT dat_0c94acfc;
extern NJS_OBJECT dat_0c94ae74;
extern NJS_OBJECT dat_0c94b624;
extern NJS_OBJECT dat_0c94bc44;
extern NJS_OBJECT dat_0c94be44;
extern NJS_OBJECT dat_0c94c190;
extern NJS_OBJECT dat_0c94ca3c;
extern NJS_OBJECT dat_0c94d30c;
extern NJS_OBJECT dat_0c94dd7c;
extern NJS_OBJECT dat_0c94df50;
extern NJS_OBJECT dat_0c94e19c;
extern NJS_OBJECT dat_0c94e5b8;
extern NJS_OBJECT dat_0c94f07c;
extern NJS_OBJECT dat_0c94f324;
extern NJS_OBJECT dat_0c94f638;
extern NJS_OBJECT dat_0c94f94c;
extern NJS_OBJECT dat_0c9500ec;
extern NJS_OBJECT dat_0c950544;
extern NJS_OBJECT dat_0c950648;
extern NJS_OBJECT dat_0c95074c;
extern NJS_OBJECT dat_0c950850;
extern NJS_OBJECT dat_0c950954;
extern NJS_OBJECT dat_0c950a58;
extern NJS_OBJECT dat_0c95110c;
extern NJS_OBJECT dat_0c9520a8;
extern NJS_OBJECT dat_0c9521ac;
extern NJS_OBJECT dat_0c9522b0;
extern NJS_OBJECT dat_0c9524a0;
extern NJS_OBJECT dat_0c952690;
extern NJS_OBJECT dat_0c952880;
extern NJS_OBJECT dat_0c952a70;
extern NJS_OBJECT dat_0c952d7c;
extern NJS_OBJECT dat_0c952f7c;
extern NJS_OBJECT dat_0c9530f8;
extern NJS_OBJECT dat_0c953548;
extern NJS_OBJECT dat_0c953920;
extern NJS_OBJECT dat_0c953cf8;
extern NJS_OBJECT dat_0c9540d0;
extern NJS_OBJECT dat_0c9544a8;
extern NJS_OBJECT dat_0c954880;
extern NJS_OBJECT dat_0c954c58;
extern NJS_OBJECT dat_0c955030;
extern NJS_OBJECT dat_0c9551f4;
extern NJS_OBJECT dat_0c9553b8;
extern NJS_OBJECT dat_0c95557c;
extern NJS_OBJECT dat_0c955740;
extern NJS_OBJECT dat_0c955904;
extern NJS_OBJECT dat_0c955ac8;
extern NJS_OBJECT dat_0c955c8c;
extern NJS_OBJECT dat_0c955e50;
extern NJS_OBJECT dat_0c956014;
extern NJS_OBJECT dat_0c9561d8;
extern NJS_OBJECT dat_0c95639c;
extern NJS_OBJECT dat_0c956560;
extern NJS_OBJECT dat_0c956724;
extern NJS_OBJECT dat_0c9568e8;
extern NJS_OBJECT dat_0c956aac;
extern NJS_OBJECT dat_0c956c70;
extern NJS_OBJECT dat_0c956e34;
extern NJS_OBJECT dat_0c956ff8;
extern NJS_OBJECT dat_0c9571bc;
extern NJS_OBJECT dat_0c957344;
extern NJS_OBJECT dat_0c9574cc;
extern NJS_OBJECT dat_0c957654;
extern NJS_OBJECT dat_0c9577dc;
extern NJS_OBJECT dat_0c957964;
extern NJS_OBJECT dat_0c957aec;
extern NJS_OBJECT dat_0c957c74;
extern NJS_OBJECT dat_0c957dfc;
extern NJS_OBJECT dat_0c957f84;
extern NJS_OBJECT dat_0c95810c;
extern NJS_OBJECT dat_0c9583c0;
extern NJS_OBJECT dat_0c958658;
extern NJS_OBJECT dat_0c958b48;
extern NJS_OBJECT dat_0c958df8;
extern NJS_OBJECT dat_0c959040;
extern NJS_OBJECT dat_0c9592c4;
extern NJS_OBJECT dat_0c959b74;
extern NJS_OBJECT dat_0c959df0;
extern NJS_OBJECT dat_0c959fc0;
extern NJS_OBJECT dat_0c95a190;
extern NJS_OBJECT dat_0c95a65c;
extern NJS_OBJECT dat_0c95a8c4;
extern NJS_OBJECT dat_0c95ab58;
extern NJS_OBJECT dat_0c95adec;
extern NJS_OBJECT dat_0c95afe0;
extern NJS_OBJECT dat_0c95b2cc;
extern NJS_OBJECT dat_0c95b530;
extern NJS_OBJECT dat_0c95b794;
extern NJS_OBJECT dat_0c95b9f8;
extern NJS_OBJECT dat_0c95bd1c;
extern NJS_OBJECT dat_0c95bee0;
extern NJS_OBJECT dat_0c95c0a4;
extern NJS_OBJECT dat_0c95c268;
extern NJS_OBJECT dat_0c95c42c;
extern NJS_OBJECT dat_0c95c5f0;
extern NJS_OBJECT dat_0c95c7b4;
extern NJS_OBJECT dat_0c95c978;
extern NJS_OBJECT dat_0c95cb3c;
extern NJS_OBJECT dat_0c95cd00;
extern NJS_OBJECT dat_0c95cec4;
extern NJS_OBJECT dat_0c95d088;
extern NJS_OBJECT dat_0c95d418;
extern NJS_OBJECT dat_0c95d7a8;
extern NJS_OBJECT dat_0c95db38;
extern NJS_OBJECT dat_0c95dd30;
extern NJS_OBJECT dat_0c95def4;
extern NJS_OBJECT dat_0c95e0b8;
extern NJS_OBJECT dat_0c95e27c;
extern NJS_OBJECT dat_0c95e440;
extern NJS_OBJECT dat_0c95e604;
extern NJS_OBJECT dat_0c95e7c8;
extern NJS_OBJECT dat_0c95ea64;
extern NJS_OBJECT dat_0c95ed00;
extern NJS_OBJECT dat_0c95ef9c;
extern NJS_OBJECT dat_0c95f238;
extern NJS_OBJECT dat_0c95f4d4;
extern NJS_OBJECT dat_0c95f770;
extern NJS_OBJECT dat_0c95fa0c;
extern NJS_OBJECT dat_0c95fca8;
extern NJS_OBJECT dat_0c95ff44;
extern NJS_OBJECT dat_0c9601e0;
extern NJS_OBJECT dat_0c96047c;
extern NJS_OBJECT dat_0c960718;
extern NJS_OBJECT dat_0c9609b4;
extern NJS_OBJECT dat_0c960c50;
extern NJS_OBJECT dat_0c960eec;
extern NJS_OBJECT dat_0c961188;
extern NJS_OBJECT dat_0c961424;
extern NJS_OBJECT dat_0c9616c0;
extern NJS_OBJECT dat_0c9620f0;
extern NJS_OBJECT dat_0c9625a8;
extern NJS_OBJECT dat_0c9628a4;
extern NJS_OBJECT dat_0c962d74;
extern NJS_OBJECT dat_0c96389c;
extern NJS_OBJECT dat_0c963fbc;
extern NJS_OBJECT dat_0c964498;
extern NJS_OBJECT dat_0c964788;
extern NJS_OBJECT dat_0c964e10;
extern NJS_OBJECT dat_0c96504c;
extern NJS_OBJECT dat_0c9651b8;
extern NJS_OBJECT dat_0c96551c;
extern NJS_OBJECT dat_0c96588c;
extern NJS_OBJECT dat_0c965d14;
extern NJS_OBJECT dat_0c966260;
extern NJS_OBJECT dat_0c9665c4;
extern NJS_OBJECT dat_0c966928;
extern NJS_OBJECT dat_0c966c8c;
extern NJS_OBJECT dat_0c966ff0;
extern NJS_OBJECT dat_0c967354;
extern NJS_OBJECT dat_0c9674cc;
extern NJS_OBJECT dat_0c967644;
extern NJS_OBJECT dat_0c9677bc;
extern NJS_OBJECT dat_0c967c84;
extern NJS_OBJECT dat_0c967e00;
extern NJS_OBJECT dat_0c967fb4;
extern NJS_OBJECT dat_0c9684b8;
extern NJS_OBJECT dat_0c9689bc;
extern NJS_OBJECT dat_0c968e68;
extern NJS_OBJECT dat_0c96930c;
extern NJS_OBJECT dat_0c96a184;
extern NJS_OBJECT dat_0c96a61c;
extern NJS_OBJECT dat_0c96a840;
extern NJS_OBJECT dat_0c96a944;
extern NJS_OBJECT dat_0c96ab38;
extern NJS_OBJECT dat_0c96ad10;
extern NJS_OBJECT dat_0c96aee8;
extern NJS_OBJECT dat_0c96b0c0;
extern NJS_OBJECT dat_0c96b298;
extern NJS_OBJECT dat_0c96b470;
extern NJS_OBJECT dat_0c96b648;
extern NJS_OBJECT dat_0c96b820;
extern NJS_OBJECT dat_0c96b9f8;
extern NJS_OBJECT dat_0c96bbd0;
extern NJS_OBJECT dat_0c96bda8;
extern NJS_OBJECT dat_0c96bf80;
extern NJS_OBJECT dat_0c96c158;
extern NJS_OBJECT dat_0c96c330;
extern NJS_OBJECT dat_0c96c508;
extern NJS_OBJECT dat_0c96c6e0;
extern NJS_OBJECT dat_0c96c8b8;
extern NJS_OBJECT dat_0c96caa8;
extern NJS_OBJECT dat_0c96cc98;
extern NJS_OBJECT dat_0c96ce88;
extern NJS_OBJECT dat_0c96d078;
extern NJS_OBJECT dat_0c96d268;
extern NJS_OBJECT dat_0c96d458;
extern NJS_OBJECT dat_0c96d648;
extern NJS_OBJECT dat_0c96d838;
extern NJS_OBJECT dat_0c96da28;
extern NJS_OBJECT dat_0c96dc18;
extern NJS_OBJECT dat_0c96de08;
extern NJS_OBJECT dat_0c96dff8;
extern NJS_OBJECT dat_0c96e1e8;
extern NJS_OBJECT dat_0c96e3d8;
extern NJS_OBJECT dat_0c96e5c8;
extern NJS_OBJECT dat_0c96e7b8;
extern NJS_OBJECT dat_0c96e9a8;
extern NJS_OBJECT dat_0c96eb98;
extern NJS_OBJECT dat_0c96ed88;
extern NJS_OBJECT dat_0c96ef78;
extern NJS_OBJECT dat_0c96f18c;
extern NJS_OBJECT dat_0c96f3a0;
extern NJS_OBJECT dat_0c970314;
extern NJS_OBJECT dat_0c9705c8;
extern NJS_OBJECT dat_0c970844;
extern NJS_OBJECT dat_0c9718f8;
extern NJS_OBJECT dat_0c971a70;
extern NJS_OBJECT dat_0c971be8;
extern NJS_OBJECT dat_0c971dac;
extern NJS_OBJECT dat_0c971f70;
extern NJS_OBJECT dat_0c972134;
extern NJS_OBJECT dat_0c9722f8;
extern NJS_OBJECT dat_0c972b30;
extern NJS_OBJECT dat_0c972e7c;
extern NJS_OBJECT dat_0c9731c8;
extern NJS_OBJECT dat_0c973514;
extern NJS_OBJECT dat_0c973860;
extern NJS_OBJECT dat_0c973bac;
extern NJS_OBJECT dat_0c973ef8;
extern NJS_OBJECT dat_0c97454c;
extern NJS_OBJECT dat_0c974b50;
extern NJS_OBJECT dat_0c9755dc;
extern NJS_OBJECT dat_0c9757d0;
extern NJS_OBJECT dat_0c975b44;
extern NJS_OBJECT dat_0c975d38;
extern NJS_OBJECT dat_0c975f2c;
extern NJS_OBJECT dat_0c976158;
extern NJS_OBJECT dat_0c9764a0;
extern NJS_OBJECT dat_0c9767e8;
extern NJS_OBJECT dat_0c976b30;
extern NJS_OBJECT dat_0c976e78;
extern NJS_OBJECT dat_0c976f7c;
extern NJS_OBJECT dat_0c977080;
extern NJS_OBJECT dat_0c977184;
extern NJS_OBJECT dat_0c977288;
extern NJS_OBJECT dat_0c97738c;
extern NJS_OBJECT dat_0c977a9c;
extern NJS_OBJECT dat_0c977fd4;
extern NJS_OBJECT dat_0c9785a0;
extern NJS_OBJECT dat_0c979150;
extern NJS_OBJECT dat_0c97938c;
extern NJS_OBJECT dat_0c9796fc;
extern NJS_OBJECT dat_0c979800;
extern NJS_OBJECT dat_0c97a1cc;
extern NJS_OBJECT dat_0c97a3f0;
extern NJS_OBJECT dat_0c97a5e8;
extern NJS_OBJECT dat_0c97a6ec;
extern NJS_OBJECT dat_0c97a8fc;
extern NJS_OBJECT dat_0c97aac0;
extern NJS_OBJECT dat_0c97d84c;
extern NJS_OBJECT dat_0c97dad0;
extern NJS_OBJECT dat_0c97dd54;
extern NJS_OBJECT dat_0c97dfd8;
extern NJS_OBJECT dat_0c97e21c;
extern NJS_OBJECT dat_0c97e5b8;
extern NJS_OBJECT dat_0c97e9d0;
extern NJS_OBJECT dat_0c97ebac;
extern NJS_OBJECT dat_0c97ed88;
extern NJS_OBJECT dat_0c97ef64;
extern NJS_OBJECT dat_0c97f140;
extern NJS_OBJECT dat_0c97f31c;
extern NJS_OBJECT dat_0c97f4f8;
extern NJS_OBJECT dat_0c97f6d4;
extern NJS_OBJECT dat_0c97f8b0;
extern NJS_OBJECT dat_0c97fa8c;
extern NJS_OBJECT dat_0c97fc68;
extern NJS_OBJECT dat_0c97fe44;
extern NJS_OBJECT dat_0c980020;
extern NJS_OBJECT dat_0c982440;
extern NJS_OBJECT dat_0c9827a0;
extern NJS_OBJECT dat_0c982cac;
extern NJS_OBJECT dat_0c9831b8;
extern NJS_OBJECT dat_0c9836c4;
extern NJS_OBJECT dat_0c9838b4;
extern NJS_OBJECT dat_0c984024;
extern NJS_OBJECT dat_0c9845e0;
extern NJS_OBJECT dat_0c984b40;
extern NJS_OBJECT dat_0c984f0c;
extern NJS_OBJECT dat_0c986f60;
extern NJS_OBJECT dat_0c987374;
extern NJS_OBJECT dat_0c987798;
extern NJS_OBJECT dat_0c987c20;
extern NJS_OBJECT dat_0c98861c;
extern NJS_OBJECT dat_0c988c2c;
extern NJS_OBJECT dat_0c988ee4;
extern NJS_OBJECT dat_0c98919c;
extern NJS_OBJECT dat_0c989454;

static NJS_TEXNAME egm2_land_textures[231] = {
    {TEXT_1, 0, 0},   {TEXT_2, 0, 0},   {TEXT_3, 0, 0},   {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},   {TEXT_6, 0, 0},   {TEXT_7, 0, 0},   {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},   {TEXT_10, 0, 0},  {TEXT_11, 0, 0},  {TEXT_12, 0, 0},
    {TEXT_13, 0, 0},  {TEXT_14, 0, 0},  {TEXT_15, 0, 0},  {TEXT_16, 0, 0},
    {TEXT_17, 0, 0},  {TEXT_18, 0, 0},  {TEXT_19, 0, 0},  {TEXT_20, 0, 0},
    {TEXT_21, 0, 0},  {TEXT_22, 0, 0},  {TEXT_23, 0, 0},  {TEXT_24, 0, 0},
    {TEXT_25, 0, 0},  {TEXT_26, 0, 0},  {TEXT_27, 0, 0},  {TEXT_28, 0, 0},
    {TEXT_29, 0, 0},  {TEXT_30, 0, 0},  {TEXT_31, 0, 0},  {TEXT_32, 0, 0},
    {TEXT_33, 0, 0},  {TEXT_34, 0, 0},  {TEXT_35, 0, 0},  {TEXT_36, 0, 0},
    {TEXT_37, 0, 0},  {TEXT_38, 0, 0},  {TEXT_39, 0, 0},  {TEXT_40, 0, 0},
    {TEXT_41, 0, 0},  {TEXT_42, 0, 0},  {TEXT_43, 0, 0},  {TEXT_44, 0, 0},
    {TEXT_45, 0, 0},  {TEXT_46, 0, 0},  {TEXT_47, 0, 0},  {TEXT_48, 0, 0},
    {TEXT_49, 0, 0},  {TEXT_50, 0, 0},  {TEXT_51, 0, 0},  {TEXT_52, 0, 0},
    {TEXT_53, 0, 0},  {TEXT_54, 0, 0},  {TEXT_55, 0, 0},  {TEXT_56, 0, 0},
    {TEXT_57, 0, 0},  {TEXT_58, 0, 0},  {TEXT_59, 0, 0},  {TEXT_60, 0, 0},
    {TEXT_61, 0, 0},  {TEXT_62, 0, 0},  {TEXT_63, 0, 0},  {TEXT_64, 0, 0},
    {TEXT_65, 0, 0},  {TEXT_66, 0, 0},  {TEXT_67, 0, 0},  {TEXT_68, 0, 0},
    {TEXT_69, 0, 0},  {TEXT_70, 0, 0},  {TEXT_71, 0, 0},  {TEXT_72, 0, 0},
    {TEXT_73, 0, 0},  {TEXT_74, 0, 0},  {TEXT_75, 0, 0},  {TEXT_76, 0, 0},
    {TEXT_77, 0, 0},  {TEXT_78, 0, 0},  {TEXT_79, 0, 0},  {TEXT_80, 0, 0},
    {TEXT_81, 0, 0},  {TEXT_82, 0, 0},  {TEXT_83, 0, 0},  {TEXT_84, 0, 0},
    {TEXT_85, 0, 0},  {TEXT_86, 0, 0},  {TEXT_87, 0, 0},  {TEXT_88, 0, 0},
    {TEXT_89, 0, 0},  {TEXT_90, 0, 0},  {TEXT_91, 0, 0},  {TEXT_92, 0, 0},
    {TEXT_93, 0, 0},  {TEXT_94, 0, 0},  {TEXT_95, 0, 0},  {TEXT_96, 0, 0},
    {TEXT_97, 0, 0},  {TEXT_98, 0, 0},  {TEXT_99, 0, 0},  {TEXT_100, 0, 0},
    {TEXT_101, 0, 0}, {TEXT_102, 0, 0}, {TEXT_103, 0, 0}, {TEXT_104, 0, 0},
    {TEXT_105, 0, 0}, {TEXT_106, 0, 0}, {TEXT_107, 0, 0}, {TEXT_108, 0, 0},
    {TEXT_109, 0, 0}, {TEXT_110, 0, 0}, {TEXT_111, 0, 0}, {TEXT_112, 0, 0},
    {TEXT_113, 0, 0}, {TEXT_114, 0, 0}, {TEXT_115, 0, 0}, {TEXT_116, 0, 0},
    {TEXT_117, 0, 0}, {TEXT_118, 0, 0}, {TEXT_119, 0, 0}, {TEXT_120, 0, 0},
    {TEXT_121, 0, 0}, {TEXT_122, 0, 0}, {TEXT_123, 0, 0}, {TEXT_124, 0, 0},
    {TEXT_125, 0, 0}, {TEXT_126, 0, 0}, {TEXT_127, 0, 0}, {TEXT_128, 0, 0},
    {TEXT_129, 0, 0}, {TEXT_130, 0, 0}, {TEXT_131, 0, 0}, {TEXT_132, 0, 0},
    {TEXT_133, 0, 0}, {TEXT_134, 0, 0}, {TEXT_135, 0, 0}, {TEXT_136, 0, 0},
    {TEXT_137, 0, 0}, {TEXT_138, 0, 0}, {TEXT_139, 0, 0}, {TEXT_140, 0, 0},
    {TEXT_141, 0, 0}, {TEXT_142, 0, 0}, {TEXT_143, 0, 0}, {TEXT_144, 0, 0},
    {TEXT_145, 0, 0}, {TEXT_146, 0, 0}, {TEXT_147, 0, 0}, {TEXT_148, 0, 0},
    {TEXT_149, 0, 0}, {TEXT_150, 0, 0}, {TEXT_151, 0, 0}, {TEXT_152, 0, 0},
    {TEXT_153, 0, 0}, {TEXT_154, 0, 0}, {TEXT_155, 0, 0}, {TEXT_156, 0, 0},
    {TEXT_157, 0, 0}, {TEXT_158, 0, 0}, {TEXT_159, 0, 0}, {TEXT_160, 0, 0},
    {TEXT_161, 0, 0}, {TEXT_162, 0, 0}, {TEXT_163, 0, 0}, {TEXT_164, 0, 0},
    {TEXT_165, 0, 0}, {TEXT_166, 0, 0}, {TEXT_167, 0, 0}, {TEXT_168, 0, 0},
    {TEXT_169, 0, 0}, {TEXT_170, 0, 0}, {TEXT_171, 0, 0}, {TEXT_172, 0, 0},
    {TEXT_173, 0, 0}, {TEXT_174, 0, 0}, {TEXT_175, 0, 0}, {TEXT_176, 0, 0},
    {TEXT_177, 0, 0}, {TEXT_178, 0, 0}, {TEXT_179, 0, 0}, {TEXT_180, 0, 0},
    {TEXT_181, 0, 0}, {TEXT_182, 0, 0}, {TEXT_183, 0, 0}, {TEXT_184, 0, 0},
    {TEXT_185, 0, 0}, {TEXT_186, 0, 0}, {TEXT_187, 0, 0}, {TEXT_188, 0, 0},
    {TEXT_189, 0, 0}, {TEXT_190, 0, 0}, {TEXT_191, 0, 0}, {TEXT_192, 0, 0},
    {TEXT_193, 0, 0}, {TEXT_194, 0, 0}, {TEXT_195, 0, 0}, {TEXT_196, 0, 0},
    {TEXT_197, 0, 0}, {TEXT_198, 0, 0}, {TEXT_199, 0, 0}, {TEXT_200, 0, 0},
    {TEXT_201, 0, 0}, {TEXT_202, 0, 0}, {TEXT_203, 0, 0}, {TEXT_204, 0, 0},
    {TEXT_205, 0, 0}, {TEXT_206, 0, 0}, {TEXT_207, 0, 0}, {TEXT_208, 0, 0},
    {TEXT_209, 0, 0}, {TEXT_210, 0, 0}, {TEXT_211, 0, 0}, {TEXT_212, 0, 0},
    {TEXT_213, 0, 0}, {TEXT_214, 0, 0}, {TEXT_215, 0, 0}, {TEXT_216, 0, 0},
    {TEXT_217, 0, 0}, {TEXT_218, 0, 0}, {TEXT_219, 0, 0}, {TEXT_220, 0, 0},
    {TEXT_221, 0, 0}, {TEXT_222, 0, 0}, {TEXT_223, 0, 0}, {TEXT_224, 0, 0},
    {TEXT_225, 0, 0}, {TEXT_226, 0, 0}, {TEXT_227, 0, 0}, {TEXT_228, 0, 0},
    {TEXT_229, 0, 0}, {TEXT_230, 0, 0}, {TEXT_231, 0, 0},
};
static NJS_TEXLIST egm2_land_texlist = {
#include "b_egm2/land/egm2_land_texlist.inc"
};
static struct land_entry egm2_land_entries[306] = {
#include "b_egm2/land/egm2_land_entries.inc"
};

#pragma section HEAD
/* the pvm's name is an array, since the table's generated body points only
 * at symbols. SHC aligns the array to 1 and a string literal to 4. as the
 * object's only CHEAD constant, the array starts where the literal would */
static const char egm2_land_pvm[11] = {
#include "b_egm2/land/egm2_land_pvm.inc"
};
struct land_table egm2_land = {
#include "b_egm2/land/egm2_land.inc"
};
#pragma section
