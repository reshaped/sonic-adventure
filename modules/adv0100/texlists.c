/* the texture lists of the Egg Carrier's outside acts, and the pvm lists
 * that load them */
#include <ninja.h>

#include "adv0100/texlists.h"
#include "adv0100/transform_textures.h"
#include "progress/emblem.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 13
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 11
#define TEXT_24_LENGTH 10
#define TEXT_25_LENGTH 11
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 11
#define TEXT_30_LENGTH 11
#define TEXT_31_LENGTH 11
#define TEXT_32_LENGTH 11
#define TEXT_33_LENGTH 11
#define TEXT_34_LENGTH 10
#define TEXT_35_LENGTH 11
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 11
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 9
#define TEXT_42_LENGTH 12
#define TEXT_43_LENGTH 13
#define TEXT_44_LENGTH 12
#define TEXT_45_LENGTH 12
#define TEXT_46_LENGTH 12
#define TEXT_47_LENGTH 11
#define TEXT_48_LENGTH 12
#define TEXT_49_LENGTH 12
#define TEXT_50_LENGTH 11
#define TEXT_51_LENGTH 11
#define TEXT_52_LENGTH 12
#define TEXT_53_LENGTH 12
#define TEXT_54_LENGTH 12
#define TEXT_55_LENGTH 12
#define TEXT_56_LENGTH 12
#define TEXT_57_LENGTH 12
#define TEXT_58_LENGTH 12
#define TEXT_59_LENGTH 11
#define TEXT_60_LENGTH 11
#define TEXT_61_LENGTH 11
#define TEXT_62_LENGTH 11
#define TEXT_63_LENGTH 11
#define TEXT_64_LENGTH 11
#define TEXT_65_LENGTH 11
#define TEXT_66_LENGTH 12
#define TEXT_67_LENGTH 12
#define TEXT_68_LENGTH 12
#define TEXT_69_LENGTH 12
#define TEXT_70_LENGTH 12
#define TEXT_71_LENGTH 12
#define TEXT_72_LENGTH 12
#define TEXT_73_LENGTH 12
#define TEXT_74_LENGTH 12
#define TEXT_75_LENGTH 12
#define TEXT_76_LENGTH 11
#define TEXT_77_LENGTH 11
#define TEXT_78_LENGTH 11
#define TEXT_79_LENGTH 11
#define TEXT_80_LENGTH 11
#define TEXT_81_LENGTH 11
#define TEXT_82_LENGTH 11
#define TEXT_83_LENGTH 11
#define TEXT_84_LENGTH 11
#define TEXT_85_LENGTH 11
#define TEXT_86_LENGTH 11
#define TEXT_87_LENGTH 13
#define TEXT_88_LENGTH 13
#define TEXT_89_LENGTH 13
#define TEXT_90_LENGTH 16
#define TEXT_91_LENGTH 14
#define TEXT_92_LENGTH 16
#define TEXT_93_LENGTH 11
#define TEXT_94_LENGTH 14
#define TEXT_95_LENGTH 10
#define TEXT_96_LENGTH 11
#define TEXT_97_LENGTH 12
#define TEXT_98_LENGTH 12
#define TEXT_99_LENGTH 11
#define TEXT_100_LENGTH 13
#define TEXT_101_LENGTH 12
#define TEXT_102_LENGTH 12
#define TEXT_103_LENGTH 11
#define TEXT_104_LENGTH 11
#define TEXT_105_LENGTH 11
#define TEXT_106_LENGTH 13
#define TEXT_107_LENGTH 13
#define TEXT_108_LENGTH 13
#define TEXT_109_LENGTH 13
#define TEXT_110_LENGTH 13
#define TEXT_111_LENGTH 13
#define TEXT_112_LENGTH 11
#define TEXT_113_LENGTH 11
#define TEXT_114_LENGTH 11
#define TEXT_115_LENGTH 9
#define TEXT_116_LENGTH 10
#define TEXT_117_LENGTH 11
#define TEXT_118_LENGTH 12
#define TEXT_119_LENGTH 10
#define TEXT_120_LENGTH 12
#define TEXT_121_LENGTH 10
#define TEXT_122_LENGTH 8
#define TEXT_123_LENGTH 13
#define TEXT_124_LENGTH 12
#define TEXT_125_LENGTH 13
#define TEXT_126_LENGTH 15
#define TEXT_127_LENGTH 16
#define TEXT_128_LENGTH 11
#define TEXT_129_LENGTH 10
#define TEXT_130_LENGTH 10
#define TEXT_131_LENGTH 10
#define TEXT_132_LENGTH 10
#define TEXT_133_LENGTH 10
#define TEXT_134_LENGTH 17
#define TEXT_135_LENGTH 17
#define TEXT_136_LENGTH 11
#define TEXT_137_LENGTH 12
#define TEXT_138_LENGTH 12
#define TEXT_139_LENGTH 11
#define TEXT_140_LENGTH 12
#define TEXT_141_LENGTH 10
#define TEXT_142_LENGTH 7
#define TEXT_143_LENGTH 9
#define TEXT_144_LENGTH 7
#define TEXT_145_LENGTH 10
#define TEXT_146_LENGTH 10
#define TEXT_147_LENGTH 10
#define TEXT_148_LENGTH 10
#define TEXT_149_LENGTH 10
#define TEXT_150_LENGTH 10
#define TEXT_151_LENGTH 10
#define TEXT_152_LENGTH 10
#define TEXT_153_LENGTH 7
#define TEXT_154_LENGTH 10
#define TEXT_155_LENGTH 10
#define TEXT_156_LENGTH 10
#define TEXT_157_LENGTH 10
#define TEXT_158_LENGTH 10
#define TEXT_159_LENGTH 10
#define TEXT_160_LENGTH 10
#define TEXT_161_LENGTH 10
#define TEXT_162_LENGTH 10
#define TEXT_163_LENGTH 10
#define TEXT_164_LENGTH 9
#define TEXT_165_LENGTH 9
#define TEXT_166_LENGTH 10
#define TEXT_167_LENGTH 11
#define TEXT_168_LENGTH 10
#define TEXT_169_LENGTH 10
#define TEXT_170_LENGTH 10
#define TEXT_171_LENGTH 10
#define TEXT_172_LENGTH 10
#define TEXT_173_LENGTH 10
#define TEXT_174_LENGTH 10
#define TEXT_175_LENGTH 10
#define TEXT_176_LENGTH 10
#define TEXT_177_LENGTH 10
#define TEXT_178_LENGTH 11
#define TEXT_179_LENGTH 9
#define TEXT_180_LENGTH 9
#define TEXT_181_LENGTH 9
#define TEXT_182_LENGTH 9
#define TEXT_183_LENGTH 10
#define TEXT_184_LENGTH 10
#define TEXT_185_LENGTH 10
#define TEXT_186_LENGTH 11
#define TEXT_187_LENGTH 10
#define TEXT_188_LENGTH 10
#define TEXT_189_LENGTH 13
#define TEXT_190_LENGTH 10
#define TEXT_191_LENGTH 12
#define TEXT_192_LENGTH 10
#define TEXT_193_LENGTH 12
#define TEXT_194_LENGTH 8
#define TEXT_195_LENGTH 12
#define TEXT_196_LENGTH 7
#define TEXT_197_LENGTH 11
#define TEXT_198_LENGTH 11
#define TEXT_199_LENGTH 7
#define TEXT_200_LENGTH 11
#define TEXT_201_LENGTH 4
#define TEXT_202_LENGTH 6
#define TEXT_203_LENGTH 6
#define TEXT_204_LENGTH 8
#define TEXT_205_LENGTH 8
#define TEXT_206_LENGTH 8
#define TEXT_207_LENGTH 10
#define TEXT_208_LENGTH 10
#define TEXT_209_LENGTH 10
#define TEXT_210_LENGTH 10
#define TEXT_211_LENGTH 10
#define TEXT_212_LENGTH 10
#define TEXT_213_LENGTH 10
#define TEXT_214_LENGTH 10
#define TEXT_215_LENGTH 10
#define TEXT_216_LENGTH 10
#define TEXT_217_LENGTH 10
#define TEXT_218_LENGTH 10
#define TEXT_219_LENGTH 10
#define TEXT_220_LENGTH 10
#define TEXT_221_LENGTH 10
#define TEXT_222_LENGTH 9
#define TEXT_223_LENGTH 9
#define TEXT_224_LENGTH 10
#define TEXT_225_LENGTH 10
#define TEXT_226_LENGTH 10
#define TEXT_227_LENGTH 10
#define TEXT_228_LENGTH 9
#define TEXT_229_LENGTH 9
#define TEXT_230_LENGTH 9
#define TEXT_231_LENGTH 9
#define TEXT_232_LENGTH 10
#define TEXT_233_LENGTH 10
#define TEXT_234_LENGTH 9
#define TEXT_235_LENGTH 12
#define TEXT_236_LENGTH 11
#define TEXT_237_LENGTH 11
#define TEXT_238_LENGTH 11
#define TEXT_239_LENGTH 10
#define TEXT_240_LENGTH 10
#define TEXT_241_LENGTH 10
#define TEXT_242_LENGTH 8
#define TEXT_243_LENGTH 9
#define TEXT_244_LENGTH 10
#define TEXT_245_LENGTH 10
#define TEXT_246_LENGTH 9
#define TEXT_247_LENGTH 9
#define TEXT_248_LENGTH 9
#define TEXT_249_LENGTH 9
#define TEXT_250_LENGTH 9
#define TEXT_251_LENGTH 9
#define TEXT_252_LENGTH 9
#define TEXT_253_LENGTH 10
#define TEXT_254_LENGTH 9
#define TEXT_255_LENGTH 9
#define TEXT_256_LENGTH 9
#define TEXT_257_LENGTH 10
#define TEXT_258_LENGTH 13
#define TEXT_259_LENGTH 12
#define TEXT_260_LENGTH 13
#define TEXT_261_LENGTH 11
#define TEXT_262_LENGTH 11
#define TEXT_263_LENGTH 12
#define TEXT_264_LENGTH 12
#define TEXT_265_LENGTH 10
#define TEXT_266_LENGTH 10
#define TEXT_267_LENGTH 10
#define TEXT_268_LENGTH 10
#define TEXT_269_LENGTH 11
#define TEXT_270_LENGTH 8
#define TEXT_271_LENGTH 6
#define TEXT_272_LENGTH 10
#define TEXT_273_LENGTH 12
#define TEXT_274_LENGTH 9
#define TEXT_275_LENGTH 9
#define TEXT_276_LENGTH 9
#define TEXT_277_LENGTH 10
#define TEXT_278_LENGTH 10
#define TEXT_279_LENGTH 10
#define TEXT_280_LENGTH 12
#define TEXT_281_LENGTH 10
#define TEXT_282_LENGTH 7
#define TEXT_283_LENGTH 9
#define TEXT_284_LENGTH 10
#define TEXT_285_LENGTH 10
#define TEXT_286_LENGTH 9
#define TEXT_287_LENGTH 10
#define TEXT_288_LENGTH 10
#define TEXT_289_LENGTH 10
#define TEXT_290_LENGTH 10
#define TEXT_291_LENGTH 11
#define TEXT_292_LENGTH 9
#define TEXT_293_LENGTH 9
#define TEXT_294_LENGTH 10
#define TEXT_295_LENGTH 10
#define TEXT_296_LENGTH 10
#define TEXT_297_LENGTH 10
#define TEXT_298_LENGTH 10
#define TEXT_299_LENGTH 10
#define TEXT_300_LENGTH 12
#define TEXT_301_LENGTH 10
#define TEXT_302_LENGTH 10
#define TEXT_303_LENGTH 13
#define TEXT_304_LENGTH 11
#define TEXT_305_LENGTH 12
#define TEXT_306_LENGTH 12
#define TEXT_307_LENGTH 12
#define TEXT_308_LENGTH 11
#define TEXT_309_LENGTH 12
#define TEXT_310_LENGTH 12
#define TEXT_311_LENGTH 11
#define TEXT_312_LENGTH 11
#define TEXT_313_LENGTH 11
#define TEXT_314_LENGTH 11
#define TEXT_315_LENGTH 11
#define TEXT_316_LENGTH 11
#define TEXT_317_LENGTH 13
#define TEXT_318_LENGTH 13
#define TEXT_319_LENGTH 13
#define TEXT_320_LENGTH 12
#define TEXT_321_LENGTH 12
#define TEXT_322_LENGTH 12
#define TEXT_323_LENGTH 12
#define TEXT_324_LENGTH 12
#define TEXT_325_LENGTH 12
#define TEXT_326_LENGTH 12
#define TEXT_327_LENGTH 12
#define TEXT_328_LENGTH 12
#define TEXT_329_LENGTH 12
#define TEXT_330_LENGTH 12
#define TEXT_331_LENGTH 12
#define TEXT_332_LENGTH 12
#define TEXT_333_LENGTH 12
#define TEXT_334_LENGTH 13
#define TEXT_335_LENGTH 13
#define TEXT_336_LENGTH 13
#define TEXT_337_LENGTH 13
#define TEXT_338_LENGTH 11
#define TEXT_339_LENGTH 15
#define TEXT_340_LENGTH 15
#define TEXT_341_LENGTH 12
#define TEXT_342_LENGTH 9
#define TEXT_343_LENGTH 9
#define TEXT_344_LENGTH 10
#define TEXT_345_LENGTH 11
#define TEXT_346_LENGTH 11
#define TEXT_347_LENGTH 11
#define TEXT_348_LENGTH 11
#define TEXT_349_LENGTH 11
#define TEXT_350_LENGTH 11
#define TEXT_351_LENGTH 11
#define TEXT_352_LENGTH 12
#define TEXT_353_LENGTH 9
#define TEXT_354_LENGTH 10
#define TEXT_355_LENGTH 7
#define TEXT_356_LENGTH 12
#define TEXT_357_LENGTH 8
#define TEXT_358_LENGTH 8
#define TEXT_359_LENGTH 8
#define TEXT_360_LENGTH 8
#define TEXT_361_LENGTH 8
#define TEXT_362_LENGTH 6
#define TEXT_363_LENGTH 8
#define TEXT_364_LENGTH 8
#define TEXT_365_LENGTH 8
#define TEXT_366_LENGTH 11
#define TEXT_367_LENGTH 11
#define TEXT_368_LENGTH 11
#define TEXT_369_LENGTH 11
#define TEXT_370_LENGTH 11
#define TEXT_371_LENGTH 13
#define TEXT_372_LENGTH 13
#define TEXT_373_LENGTH 10
#define TEXT_374_LENGTH 10
#define TEXT_375_LENGTH 10
#define TEXT_376_LENGTH 11
#define TEXT_377_LENGTH 10
#define TEXT_378_LENGTH 10
#define TEXT_379_LENGTH 10
#define TEXT_380_LENGTH 10
#define TEXT_381_LENGTH 10
#define TEXT_382_LENGTH 10
#define TEXT_383_LENGTH 10
#define TEXT_384_LENGTH 9
#define TEXT_385_LENGTH 10
#define TEXT_386_LENGTH 10
#define TEXT_387_LENGTH 10
#define TEXT_388_LENGTH 10
#define TEXT_389_LENGTH 10
#define TEXT_390_LENGTH 10
#define TEXT_391_LENGTH 10
#define TEXT_392_LENGTH 9
#define TEXT_393_LENGTH 9
#define TEXT_394_LENGTH 9
#define TEXT_395_LENGTH 9
#define TEXT_396_LENGTH 8
#define TEXT_397_LENGTH 8
#define TEXT_398_LENGTH 10
#define TEXT_399_LENGTH 10
#define TEXT_400_LENGTH 10
#define TEXT_401_LENGTH 14
#define TEXT_402_LENGTH 15
#define TEXT_403_LENGTH 15
#define TEXT_404_LENGTH 15
#define TEXT_405_LENGTH 11
#define TEXT_406_LENGTH 11
#define TEXT_407_LENGTH 11
#define TEXT_408_LENGTH 11
#define TEXT_409_LENGTH 11
#define TEXT_410_LENGTH 11
#define TEXT_411_LENGTH 11
#define TEXT_412_LENGTH 11
#define TEXT_413_LENGTH 11
#define TEXT_414_LENGTH 10
#define TEXT_415_LENGTH 5
#define TEXT_416_LENGTH 5
#define TEXT_417_LENGTH 6
#define TEXT_418_LENGTH 9
#define TEXT_419_LENGTH 9
#define TEXT_420_LENGTH 10
#define TEXT_421_LENGTH 9
#define TEXT_422_LENGTH 8
#define TEXT_423_LENGTH 12
#define TEXT_424_LENGTH 8
#define TEXT_425_LENGTH 6
#define TEXT_426_LENGTH 10
#define TEXT_427_LENGTH 10
#define TEXT_428_LENGTH 6
#define TEXT_429_LENGTH 11
#define TEXT_430_LENGTH 6
#define TEXT_431_LENGTH 7
#define TEXT_432_LENGTH 13
#define TEXT_433_LENGTH 8
#define TEXT_434_LENGTH 4
#define TEXT_435_LENGTH 6
#define TEXT_436_LENGTH 14
#define TEXT_437_LENGTH 14
#define TEXT_438_LENGTH 14
#define TEXT_439_LENGTH 14
#define TEXT_440_LENGTH 14
#define TEXT_441_LENGTH 14
#define TEXT_442_LENGTH 14
#define TEXT_443_LENGTH 14
#define TEXT_444_LENGTH 8
#define TEXT_445_LENGTH 6
#define TEXT_446_LENGTH 10
#define TEXT_447_LENGTH 6
#define TEXT_448_LENGTH 6
#define TEXT_449_LENGTH 7
#define TEXT_450_LENGTH 8
#define TEXT_451_LENGTH 8
#define TEXT_452_LENGTH 8
#define TEXT_453_LENGTH 8
#define TEXT_454_LENGTH 8
#define TEXT_455_LENGTH 8
#define TEXT_456_LENGTH 8
#define TEXT_457_LENGTH 8
#define TEXT_458_LENGTH 12
#include "adv0100/texlists/text.inc"

/* the lists below and their textures' names lie in the head sections, which
 * lead the module. the pvm lists come last, and 1ST_READ.BIN holds their
 * addresses there */
#pragma section HEAD
static NJS_TEXNAME ec_objects_textures[] = {
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
    {TEXT_205, 0, 0}, {TEXT_206, 0, 0},
};
NJS_TEXLIST ec_objects_texlist = {
#include "adv0100/texlists/ec_objects_texlist.inc"
};

static NJS_TEXNAME ec_water_textures[] = {
    {TEXT_207, 0, 0}, {TEXT_208, 0, 0}, {TEXT_209, 0, 0}, {TEXT_210, 0, 0},
    {TEXT_211, 0, 0}, {TEXT_212, 0, 0}, {TEXT_213, 0, 0}, {TEXT_214, 0, 0},
    {TEXT_215, 0, 0}, {TEXT_216, 0, 0}, {TEXT_217, 0, 0}, {TEXT_218, 0, 0},
    {TEXT_219, 0, 0}, {TEXT_220, 0, 0},
};
NJS_TEXLIST ec_water_texlist = {
#include "adv0100/texlists/ec_water_texlist.inc"
};

static NJS_TEXNAME ec_act_0_textures[] = {
    {TEXT_1, 0, 0},   {TEXT_2, 0, 0},   {TEXT_221, 0, 0}, {TEXT_222, 0, 0},
    {TEXT_223, 0, 0}, {TEXT_224, 0, 0}, {TEXT_225, 0, 0}, {TEXT_226, 0, 0},
    {TEXT_227, 0, 0}, {TEXT_3, 0, 0},   {TEXT_4, 0, 0},   {TEXT_228, 0, 0},
    {TEXT_229, 0, 0}, {TEXT_230, 0, 0}, {TEXT_231, 0, 0}, {TEXT_5, 0, 0},
    {TEXT_6, 0, 0},   {TEXT_7, 0, 0},   {TEXT_8, 0, 0},   {TEXT_9, 0, 0},
    {TEXT_232, 0, 0}, {TEXT_10, 0, 0},  {TEXT_233, 0, 0}, {TEXT_234, 0, 0},
    {TEXT_235, 0, 0}, {TEXT_236, 0, 0}, {TEXT_237, 0, 0}, {TEXT_238, 0, 0},
    {TEXT_11, 0, 0},  {TEXT_12, 0, 0},  {TEXT_239, 0, 0}, {TEXT_13, 0, 0},
    {TEXT_240, 0, 0}, {TEXT_241, 0, 0}, {TEXT_242, 0, 0}, {TEXT_243, 0, 0},
    {TEXT_244, 0, 0}, {TEXT_245, 0, 0}, {TEXT_246, 0, 0}, {TEXT_247, 0, 0},
    {TEXT_248, 0, 0}, {TEXT_15, 0, 0},  {TEXT_249, 0, 0}, {TEXT_250, 0, 0},
    {TEXT_251, 0, 0}, {TEXT_252, 0, 0}, {TEXT_16, 0, 0},  {TEXT_253, 0, 0},
    {TEXT_17, 0, 0},  {TEXT_254, 0, 0}, {TEXT_255, 0, 0}, {TEXT_256, 0, 0},
    {TEXT_257, 0, 0}, {TEXT_258, 0, 0}, {TEXT_87, 0, 0},  {TEXT_90, 0, 0},
    {TEXT_92, 0, 0},  {TEXT_259, 0, 0}, {TEXT_93, 0, 0},  {TEXT_95, 0, 0},
    {TEXT_260, 0, 0}, {TEXT_97, 0, 0},  {TEXT_98, 0, 0},  {TEXT_99, 0, 0},
    {TEXT_261, 0, 0}, {TEXT_262, 0, 0}, {TEXT_104, 0, 0}, {TEXT_105, 0, 0},
    {TEXT_112, 0, 0}, {TEXT_113, 0, 0}, {TEXT_263, 0, 0}, {TEXT_138, 0, 0},
    {TEXT_264, 0, 0}, {TEXT_141, 0, 0}, {TEXT_148, 0, 0}, {TEXT_161, 0, 0},
    {TEXT_162, 0, 0}, {TEXT_163, 0, 0}, {TEXT_166, 0, 0}, {TEXT_265, 0, 0},
    {TEXT_171, 0, 0}, {TEXT_266, 0, 0}, {TEXT_267, 0, 0}, {TEXT_177, 0, 0},
    {TEXT_179, 0, 0}, {TEXT_181, 0, 0}, {TEXT_182, 0, 0}, {TEXT_268, 0, 0},
    {TEXT_187, 0, 0}, {TEXT_269, 0, 0}, {TEXT_270, 0, 0}, {TEXT_189, 0, 0},
    {TEXT_271, 0, 0}, {TEXT_190, 0, 0}, {TEXT_272, 0, 0}, {TEXT_273, 0, 0},
    {TEXT_194, 0, 0}, {TEXT_274, 0, 0},
};
NJS_TEXLIST ec_act_0_texlist = {
#include "adv0100/texlists/ec_act_0_texlist.inc"
};

static NJS_TEXNAME ec_act_1_textures[] = {
    {TEXT_1, 0, 0},   {TEXT_2, 0, 0},   {TEXT_222, 0, 0}, {TEXT_225, 0, 0},
    {TEXT_226, 0, 0}, {TEXT_227, 0, 0}, {TEXT_3, 0, 0},   {TEXT_4, 0, 0},
    {TEXT_228, 0, 0}, {TEXT_229, 0, 0}, {TEXT_230, 0, 0}, {TEXT_231, 0, 0},
    {TEXT_5, 0, 0},   {TEXT_6, 0, 0},   {TEXT_7, 0, 0},   {TEXT_8, 0, 0},
    {TEXT_10, 0, 0},  {TEXT_233, 0, 0}, {TEXT_235, 0, 0}, {TEXT_236, 0, 0},
    {TEXT_237, 0, 0}, {TEXT_238, 0, 0}, {TEXT_11, 0, 0},  {TEXT_12, 0, 0},
    {TEXT_239, 0, 0}, {TEXT_13, 0, 0},  {TEXT_240, 0, 0}, {TEXT_241, 0, 0},
    {TEXT_242, 0, 0}, {TEXT_243, 0, 0}, {TEXT_244, 0, 0}, {TEXT_245, 0, 0},
    {TEXT_246, 0, 0}, {TEXT_247, 0, 0}, {TEXT_248, 0, 0}, {TEXT_15, 0, 0},
    {TEXT_249, 0, 0}, {TEXT_275, 0, 0}, {TEXT_250, 0, 0}, {TEXT_251, 0, 0},
    {TEXT_252, 0, 0}, {TEXT_16, 0, 0},  {TEXT_253, 0, 0}, {TEXT_17, 0, 0},
    {TEXT_254, 0, 0}, {TEXT_255, 0, 0}, {TEXT_256, 0, 0}, {TEXT_257, 0, 0},
    {TEXT_258, 0, 0}, {TEXT_87, 0, 0},  {TEXT_90, 0, 0},  {TEXT_92, 0, 0},
    {TEXT_259, 0, 0}, {TEXT_93, 0, 0},  {TEXT_95, 0, 0},  {TEXT_260, 0, 0},
    {TEXT_97, 0, 0},  {TEXT_99, 0, 0},  {TEXT_261, 0, 0}, {TEXT_262, 0, 0},
    {TEXT_104, 0, 0}, {TEXT_105, 0, 0}, {TEXT_112, 0, 0}, {TEXT_113, 0, 0},
    {TEXT_138, 0, 0}, {TEXT_264, 0, 0}, {TEXT_141, 0, 0}, {TEXT_148, 0, 0},
    {TEXT_161, 0, 0}, {TEXT_166, 0, 0}, {TEXT_265, 0, 0}, {TEXT_171, 0, 0},
    {TEXT_266, 0, 0}, {TEXT_267, 0, 0}, {TEXT_177, 0, 0}, {TEXT_276, 0, 0},
    {TEXT_179, 0, 0}, {TEXT_181, 0, 0}, {TEXT_182, 0, 0}, {TEXT_277, 0, 0},
    {TEXT_268, 0, 0}, {TEXT_187, 0, 0}, {TEXT_269, 0, 0}, {TEXT_270, 0, 0},
    {TEXT_189, 0, 0}, {TEXT_190, 0, 0}, {TEXT_272, 0, 0}, {TEXT_273, 0, 0},
    {TEXT_194, 0, 0}, {TEXT_274, 0, 0},
};
NJS_TEXLIST ec_act_1_texlist = {
#include "adv0100/texlists/ec_act_1_texlist.inc"
};

static NJS_TEXNAME ec_act_2_textures[] = {
    {TEXT_2, 0, 0},   {TEXT_221, 0, 0}, {TEXT_222, 0, 0}, {TEXT_223, 0, 0},
    {TEXT_224, 0, 0}, {TEXT_3, 0, 0},   {TEXT_228, 0, 0}, {TEXT_229, 0, 0},
    {TEXT_231, 0, 0}, {TEXT_5, 0, 0},   {TEXT_8, 0, 0},   {TEXT_9, 0, 0},
    {TEXT_232, 0, 0}, {TEXT_10, 0, 0},  {TEXT_233, 0, 0}, {TEXT_234, 0, 0},
    {TEXT_236, 0, 0}, {TEXT_237, 0, 0}, {TEXT_238, 0, 0}, {TEXT_11, 0, 0},
    {TEXT_12, 0, 0},  {TEXT_239, 0, 0}, {TEXT_13, 0, 0},  {TEXT_240, 0, 0},
    {TEXT_241, 0, 0}, {TEXT_242, 0, 0}, {TEXT_243, 0, 0}, {TEXT_244, 0, 0},
    {TEXT_245, 0, 0}, {TEXT_246, 0, 0}, {TEXT_247, 0, 0}, {TEXT_248, 0, 0},
    {TEXT_15, 0, 0},  {TEXT_249, 0, 0}, {TEXT_250, 0, 0}, {TEXT_251, 0, 0},
    {TEXT_252, 0, 0}, {TEXT_16, 0, 0},  {TEXT_253, 0, 0}, {TEXT_17, 0, 0},
    {TEXT_278, 0, 0}, {TEXT_254, 0, 0}, {TEXT_255, 0, 0}, {TEXT_256, 0, 0},
    {TEXT_257, 0, 0}, {TEXT_279, 0, 0}, {TEXT_95, 0, 0},  {TEXT_98, 0, 0},
    {TEXT_104, 0, 0}, {TEXT_263, 0, 0}, {TEXT_280, 0, 0}, {TEXT_142, 0, 0},
    {TEXT_161, 0, 0}, {TEXT_162, 0, 0}, {TEXT_166, 0, 0}, {TEXT_281, 0, 0},
    {TEXT_267, 0, 0}, {TEXT_177, 0, 0}, {TEXT_179, 0, 0}, {TEXT_181, 0, 0},
    {TEXT_182, 0, 0}, {TEXT_187, 0, 0}, {TEXT_269, 0, 0}, {TEXT_271, 0, 0},
    {TEXT_192, 0, 0}, {TEXT_194, 0, 0},
};
NJS_TEXLIST ec_act_2_texlist = {
#include "adv0100/texlists/ec_act_2_texlist.inc"
};

static NJS_TEXNAME ec_act_3_textures[] = {
    {TEXT_142, 0, 0}, {TEXT_282, 0, 0}, {TEXT_143, 0, 0}, {TEXT_283, 0, 0},
    {TEXT_144, 0, 0}, {TEXT_284, 0, 0}, {TEXT_285, 0, 0}, {TEXT_145, 0, 0},
    {TEXT_151, 0, 0}, {TEXT_153, 0, 0}, {TEXT_161, 0, 0}, {TEXT_162, 0, 0},
    {TEXT_286, 0, 0}, {TEXT_164, 0, 0}, {TEXT_165, 0, 0}, {TEXT_265, 0, 0},
    {TEXT_171, 0, 0}, {TEXT_281, 0, 0}, {TEXT_287, 0, 0}, {TEXT_288, 0, 0},
    {TEXT_289, 0, 0}, {TEXT_290, 0, 0}, {TEXT_291, 0, 0}, {TEXT_177, 0, 0},
    {TEXT_178, 0, 0}, {TEXT_276, 0, 0}, {TEXT_179, 0, 0}, {TEXT_180, 0, 0},
    {TEXT_292, 0, 0}, {TEXT_181, 0, 0}, {TEXT_182, 0, 0}, {TEXT_293, 0, 0},
    {TEXT_183, 0, 0}, {TEXT_184, 0, 0}, {TEXT_294, 0, 0}, {TEXT_295, 0, 0},
    {TEXT_296, 0, 0}, {TEXT_185, 0, 0}, {TEXT_277, 0, 0}, {TEXT_268, 0, 0},
    {TEXT_187, 0, 0}, {TEXT_297, 0, 0}, {TEXT_269, 0, 0}, {TEXT_298, 0, 0},
    {TEXT_299, 0, 0}, {TEXT_300, 0, 0},
};
NJS_TEXLIST ec_act_3_texlist = {
#include "adv0100/texlists/ec_act_3_texlist.inc"
};

static NJS_TEXNAME ec_act_4_textures[] = {
    {TEXT_3, 0, 0},   {TEXT_6, 0, 0},   {TEXT_10, 0, 0},  {TEXT_15, 0, 0},
    {TEXT_22, 0, 0},  {TEXT_301, 0, 0}, {TEXT_24, 0, 0},  {TEXT_302, 0, 0},
    {TEXT_34, 0, 0},  {TEXT_35, 0, 0},  {TEXT_36, 0, 0},  {TEXT_37, 0, 0},
    {TEXT_38, 0, 0},  {TEXT_41, 0, 0},  {TEXT_42, 0, 0},  {TEXT_303, 0, 0},
    {TEXT_44, 0, 0},  {TEXT_304, 0, 0}, {TEXT_48, 0, 0},  {TEXT_49, 0, 0},
    {TEXT_305, 0, 0}, {TEXT_306, 0, 0}, {TEXT_307, 0, 0}, {TEXT_308, 0, 0},
    {TEXT_59, 0, 0},  {TEXT_309, 0, 0}, {TEXT_310, 0, 0}, {TEXT_76, 0, 0},
    {TEXT_77, 0, 0},  {TEXT_78, 0, 0},  {TEXT_79, 0, 0},  {TEXT_80, 0, 0},
    {TEXT_311, 0, 0}, {TEXT_312, 0, 0}, {TEXT_313, 0, 0}, {TEXT_314, 0, 0},
    {TEXT_315, 0, 0}, {TEXT_81, 0, 0},  {TEXT_316, 0, 0}, {TEXT_82, 0, 0},
    {TEXT_317, 0, 0}, {TEXT_318, 0, 0}, {TEXT_319, 0, 0}, {TEXT_258, 0, 0},
    {TEXT_320, 0, 0}, {TEXT_321, 0, 0}, {TEXT_322, 0, 0}, {TEXT_323, 0, 0},
    {TEXT_324, 0, 0}, {TEXT_325, 0, 0}, {TEXT_326, 0, 0}, {TEXT_327, 0, 0},
    {TEXT_328, 0, 0}, {TEXT_329, 0, 0}, {TEXT_330, 0, 0}, {TEXT_331, 0, 0},
    {TEXT_332, 0, 0}, {TEXT_333, 0, 0}, {TEXT_334, 0, 0}, {TEXT_335, 0, 0},
    {TEXT_336, 0, 0}, {TEXT_337, 0, 0}, {TEXT_85, 0, 0},  {TEXT_87, 0, 0},
    {TEXT_104, 0, 0}, {TEXT_338, 0, 0}, {TEXT_105, 0, 0}, {TEXT_112, 0, 0},
    {TEXT_339, 0, 0}, {TEXT_340, 0, 0}, {TEXT_341, 0, 0}, {TEXT_137, 0, 0},
    {TEXT_177, 0, 0}, {TEXT_180, 0, 0}, {TEXT_190, 0, 0}, {TEXT_342, 0, 0},
    {TEXT_343, 0, 0},
};
NJS_TEXLIST ec_act_4_texlist = {
#include "adv0100/texlists/ec_act_4_texlist.inc"
};

static NJS_TEXNAME ec_act_5_textures[] = {
    {TEXT_225, 0, 0}, {TEXT_10, 0, 0},  {TEXT_344, 0, 0}, {TEXT_345, 0, 0},
    {TEXT_346, 0, 0}, {TEXT_347, 0, 0}, {TEXT_348, 0, 0}, {TEXT_349, 0, 0},
    {TEXT_45, 0, 0},  {TEXT_46, 0, 0},  {TEXT_305, 0, 0}, {TEXT_69, 0, 0},
    {TEXT_70, 0, 0},  {TEXT_350, 0, 0}, {TEXT_83, 0, 0},  {TEXT_351, 0, 0},
    {TEXT_352, 0, 0}, {TEXT_112, 0, 0}, {TEXT_353, 0, 0}, {TEXT_171, 0, 0},
    {TEXT_177, 0, 0}, {TEXT_182, 0, 0}, {TEXT_192, 0, 0}, {TEXT_273, 0, 0},
    {TEXT_354, 0, 0}, {TEXT_197, 0, 0}, {TEXT_355, 0, 0}, {TEXT_199, 0, 0},
    {TEXT_356, 0, 0}, {TEXT_357, 0, 0}, {TEXT_358, 0, 0}, {TEXT_359, 0, 0},
    {TEXT_360, 0, 0}, {TEXT_361, 0, 0}, {TEXT_362, 0, 0}, {TEXT_363, 0, 0},
    {TEXT_364, 0, 0}, {TEXT_365, 0, 0}, {TEXT_207, 0, 0}, {TEXT_366, 0, 0},
};
NJS_TEXLIST ec_act_5_texlist = {
#include "adv0100/texlists/ec_act_5_texlist.inc"
};

static NJS_TEXNAME ec_sky_textures[] = {
    {TEXT_367, 0, 0}, {TEXT_368, 0, 0}, {TEXT_369, 0, 0},
    {TEXT_370, 0, 0}, {TEXT_371, 0, 0}, {TEXT_372, 0, 0},
    {TEXT_373, 0, 0}, {TEXT_374, 0, 0}, {TEXT_375, 0, 0},
};
NJS_TEXLIST ec_sky_texlist = {
#include "adv0100/texlists/ec_sky_texlist.inc"
};

static NJS_TEXNAME ec_tornado_textures[] = {
    {TEXT_376, 0, 0}, {TEXT_377, 0, 0}, {TEXT_378, 0, 0}, {TEXT_379, 0, 0},
    {TEXT_380, 0, 0}, {TEXT_381, 0, 0}, {TEXT_382, 0, 0}, {TEXT_383, 0, 0},
    {TEXT_384, 0, 0}, {TEXT_385, 0, 0}, {TEXT_386, 0, 0}, {TEXT_387, 0, 0},
    {TEXT_388, 0, 0}, {TEXT_389, 0, 0}, {TEXT_390, 0, 0}, {TEXT_391, 0, 0},
    {TEXT_392, 0, 0}, {TEXT_393, 0, 0}, {TEXT_394, 0, 0}, {TEXT_395, 0, 0},
    {TEXT_396, 0, 0}, {TEXT_397, 0, 0}, {TEXT_398, 0, 0}, {TEXT_399, 0, 0},
    {TEXT_400, 0, 0},
};
NJS_TEXLIST ec_tornado_texlist = {
#include "adv0100/texlists/ec_tornado_texlist.inc"
};

static NJS_TEXNAME ec_sea_textures[] = {
    {TEXT_401, 0, 0}, {TEXT_402, 0, 0}, {TEXT_403, 0, 0}, {TEXT_404, 0, 0},
    {TEXT_200, 0, 0}, {TEXT_405, 0, 0}, {TEXT_406, 0, 0}, {TEXT_407, 0, 0},
    {TEXT_408, 0, 0}, {TEXT_409, 0, 0}, {TEXT_410, 0, 0}, {TEXT_411, 0, 0},
    {TEXT_412, 0, 0}, {TEXT_413, 0, 0},
};
NJS_TEXLIST ec_sea_texlist = {
#include "adv0100/texlists/ec_sea_texlist.inc"
};

static NJS_TEXNAME boat_textures[] = {
    {TEXT_414, 0, 0}, {TEXT_415, 0, 0}, {TEXT_416, 0, 0}, {TEXT_417, 0, 0},
    {TEXT_418, 0, 0}, {TEXT_419, 0, 0}, {TEXT_420, 0, 0}, {TEXT_421, 0, 0},
    {TEXT_422, 0, 0}, {TEXT_423, 0, 0}, {TEXT_424, 0, 0}, {TEXT_425, 0, 0},
    {TEXT_426, 0, 0}, {TEXT_427, 0, 0}, {TEXT_428, 0, 0}, {TEXT_429, 0, 0},
    {TEXT_430, 0, 0}, {TEXT_431, 0, 0}, {TEXT_432, 0, 0}, {TEXT_433, 0, 0},
    {TEXT_434, 0, 0}, {TEXT_435, 0, 0},
};
NJS_TEXLIST boat_texlist = {
#include "adv0100/texlists/boat_texlist.inc"
};

static NJS_TEXNAME raft_textures[] = {
    {TEXT_436, 0, 0}, {TEXT_437, 0, 0}, {TEXT_438, 0, 0}, {TEXT_439, 0, 0},
    {TEXT_440, 0, 0}, {TEXT_441, 0, 0}, {TEXT_442, 0, 0}, {TEXT_443, 0, 0},
};
NJS_TEXLIST raft_texlist = {
#include "adv0100/texlists/raft_texlist.inc"
};

/* only the first of the four slot rows holds a list, ec_objects_texlist,
 * which texture_set_stage makes current for the objects that call it */
struct texture_pvm egg_carrier_pvms[8] = {
    {TEXT_444, &ec_objects_texlist},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_445, &ec_sky_texlist},
    {TEXT_446, &ec_tornado_texlist},
    {TEXT_447, &emblem_texlist},
    {NULL, NULL},
};

struct texture_pvm egg_carrier_sea_pvms[9] = {
    {TEXT_444, &ec_objects_texlist},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_448, &ec_sea_texlist},
    {TEXT_449, &boat_texlist},
    {TEXT_450, &raft_texlist},
    {TEXT_447, &emblem_texlist},
    {NULL, NULL},
};

/* the acts' rows, from act 0 to act 6. each act takes one, but act 5 takes
 * two */
struct texture_pvm egg_carrier_act_pvms[8] = {
    {TEXT_451, &ec_act_0_texlist}, {TEXT_452, &ec_act_1_texlist},
    {TEXT_453, &ec_act_2_texlist}, {TEXT_454, &ec_act_3_texlist},
    {TEXT_455, &ec_act_4_texlist}, {TEXT_456, &ec_act_5_texlist},
    {TEXT_457, &ec_water_texlist}, {TEXT_458, &ec_transform_texlist},
};
#pragma section
