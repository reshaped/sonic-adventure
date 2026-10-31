/* the names of the textures the deck's objects load, their texlist,
 * and the material after it */
#include <ninja.h>

#include "stg06/models.h"
#include "stg06/obj_skydeck_textures.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 5
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 10
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 9
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 9
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 9
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 9
#define TEXT_34_LENGTH 9
#define TEXT_35_LENGTH 9
#define TEXT_36_LENGTH 9
#define TEXT_37_LENGTH 9
#define TEXT_38_LENGTH 9
#define TEXT_39_LENGTH 9
#define TEXT_40_LENGTH 9
#define TEXT_41_LENGTH 10
#define TEXT_42_LENGTH 9
#define TEXT_43_LENGTH 9
#define TEXT_44_LENGTH 9
#define TEXT_45_LENGTH 11
#define TEXT_46_LENGTH 11
#define TEXT_47_LENGTH 11
#define TEXT_48_LENGTH 11
#define TEXT_49_LENGTH 11
#define TEXT_50_LENGTH 11
#define TEXT_51_LENGTH 16
#define TEXT_52_LENGTH 11
#define TEXT_53_LENGTH 10
#define TEXT_54_LENGTH 12
#define TEXT_55_LENGTH 12
#define TEXT_56_LENGTH 11
#define TEXT_57_LENGTH 11
#define TEXT_58_LENGTH 13
#define TEXT_59_LENGTH 13
#define TEXT_60_LENGTH 13
#define TEXT_61_LENGTH 13
#define TEXT_62_LENGTH 13
#define TEXT_63_LENGTH 13
#define TEXT_64_LENGTH 13
#define TEXT_65_LENGTH 13
#define TEXT_66_LENGTH 13
#define TEXT_67_LENGTH 13
#define TEXT_68_LENGTH 12
#define TEXT_69_LENGTH 11
#define TEXT_70_LENGTH 11
#define TEXT_71_LENGTH 11
#define TEXT_72_LENGTH 11
#define TEXT_73_LENGTH 13
#define TEXT_74_LENGTH 13
#define TEXT_75_LENGTH 11
#define TEXT_76_LENGTH 11
#define TEXT_77_LENGTH 12
#define TEXT_78_LENGTH 7
#define TEXT_79_LENGTH 10
#define TEXT_80_LENGTH 10
#define TEXT_81_LENGTH 10
#define TEXT_82_LENGTH 10
#define TEXT_83_LENGTH 10
#define TEXT_84_LENGTH 10
#define TEXT_85_LENGTH 10
#define TEXT_86_LENGTH 9
#define TEXT_87_LENGTH 9
#define TEXT_88_LENGTH 9
#define TEXT_89_LENGTH 8
#define TEXT_90_LENGTH 10
#define TEXT_91_LENGTH 10
#define TEXT_92_LENGTH 10
#define TEXT_93_LENGTH 10
#define TEXT_94_LENGTH 10
#define TEXT_95_LENGTH 10
#define TEXT_96_LENGTH 10
#define TEXT_97_LENGTH 10
#define TEXT_98_LENGTH 11
#define TEXT_99_LENGTH 11
#define TEXT_100_LENGTH 11
#define TEXT_101_LENGTH 11
#define TEXT_102_LENGTH 11
#define TEXT_103_LENGTH 11
#define TEXT_104_LENGTH 11
#define TEXT_105_LENGTH 13
#define TEXT_106_LENGTH 10
#define TEXT_107_LENGTH 10
#define TEXT_108_LENGTH 10
#define TEXT_109_LENGTH 10
#define TEXT_110_LENGTH 10
#define TEXT_111_LENGTH 10
#define TEXT_112_LENGTH 10
#define TEXT_113_LENGTH 10
#define TEXT_114_LENGTH 12
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
#define TEXT_125_LENGTH 10
#define TEXT_126_LENGTH 10
#define TEXT_127_LENGTH 10
#define TEXT_128_LENGTH 10
#define TEXT_129_LENGTH 10
#define TEXT_130_LENGTH 10
#define TEXT_131_LENGTH 10
#define TEXT_132_LENGTH 10
#define TEXT_133_LENGTH 10
#define TEXT_134_LENGTH 10
#define TEXT_135_LENGTH 10
#define TEXT_136_LENGTH 10
#define TEXT_137_LENGTH 10
#define TEXT_138_LENGTH 10
#define TEXT_139_LENGTH 10
#define TEXT_140_LENGTH 10
#define TEXT_141_LENGTH 10
#define TEXT_142_LENGTH 10
#define TEXT_143_LENGTH 10
#define TEXT_144_LENGTH 10
#define TEXT_145_LENGTH 10
#define TEXT_146_LENGTH 10
#define TEXT_147_LENGTH 12
#define TEXT_148_LENGTH 12
#define TEXT_149_LENGTH 12
#define TEXT_150_LENGTH 10
#define TEXT_151_LENGTH 10
#define TEXT_152_LENGTH 10
#define TEXT_153_LENGTH 10
#define TEXT_154_LENGTH 10
#define TEXT_155_LENGTH 10
#define TEXT_156_LENGTH 10
#define TEXT_157_LENGTH 8
#define TEXT_158_LENGTH 8
#define TEXT_159_LENGTH 8
#define TEXT_160_LENGTH 8
#define TEXT_161_LENGTH 7
#define TEXT_162_LENGTH 12
#define TEXT_163_LENGTH 12
#define TEXT_164_LENGTH 12
#define TEXT_165_LENGTH 12
#define TEXT_166_LENGTH 13
#define TEXT_167_LENGTH 10
#define TEXT_168_LENGTH 11
#define TEXT_169_LENGTH 10
#define TEXT_170_LENGTH 16
#define TEXT_171_LENGTH 10
#define TEXT_172_LENGTH 10
#define TEXT_173_LENGTH 9
#define TEXT_174_LENGTH 9
#define TEXT_175_LENGTH 11
#define TEXT_176_LENGTH 9
#define TEXT_177_LENGTH 11
#define TEXT_178_LENGTH 10
#define TEXT_179_LENGTH 9
#define TEXT_180_LENGTH 11
#define TEXT_181_LENGTH 13
#define TEXT_182_LENGTH 8
#define TEXT_183_LENGTH 9
#define TEXT_184_LENGTH 12
#define TEXT_185_LENGTH 10
#define TEXT_186_LENGTH 9
#define TEXT_187_LENGTH 9
#define TEXT_188_LENGTH 9
#define TEXT_189_LENGTH 9
#define TEXT_190_LENGTH 9
#define TEXT_191_LENGTH 9
#define TEXT_192_LENGTH 9
#define TEXT_193_LENGTH 9
#define TEXT_194_LENGTH 9
#define TEXT_195_LENGTH 9
#define TEXT_196_LENGTH 10
#define TEXT_197_LENGTH 9
#define TEXT_198_LENGTH 12
#define TEXT_199_LENGTH 12
#define TEXT_200_LENGTH 8
#define TEXT_201_LENGTH 12
#define TEXT_202_LENGTH 12
#define TEXT_203_LENGTH 13
#define TEXT_204_LENGTH 7
#define TEXT_205_LENGTH 11
#define TEXT_206_LENGTH 12
#define TEXT_207_LENGTH 8
#define TEXT_208_LENGTH 10
#define TEXT_209_LENGTH 11
#define TEXT_210_LENGTH 13
#define TEXT_211_LENGTH 11
#define TEXT_212_LENGTH 11
#define TEXT_213_LENGTH 9
#include "stg06/obj_skydeck_textures/text.inc"

static NJS_TEXNAME obj_skydeck_textures[213] = {
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
    {TEXT_213, 0, 0},
};
NJS_TEXLIST obj_skydeck_texlist = {
#include "stg06/obj_skydeck_textures/obj_skydeck_texlist.inc"
};
NJS_MATERIAL dat_0c9ff58c[1] = {
#include "stg06/obj_skydeck_textures/dat_0c9ff58c.inc"
};
