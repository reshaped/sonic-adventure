/* the story so far, and SUMMARY's entry table. the furthest page the
 * player has reached scrolls up over the story's picture. the page shows
 * through a window, which fades it in at the bottom and out at the top */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "draw/screen.h"
#include "main.h"
#include "object/common.h"
#include "progress/progress.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "summary/picture.h"
#include "summary/staff_roll.h"
#include "summary/summary.h"
#include "summary/tutorial_screen.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/timer.h"

#define TEXT_1_LENGTH 39
#define TEXT_2_LENGTH 39
#define TEXT_3_LENGTH 38
#define TEXT_4_LENGTH 39
#define TEXT_5_LENGTH 37
#define TEXT_6_LENGTH 36
#define TEXT_7_LENGTH 32
#define TEXT_8_LENGTH 29
#define TEXT_9_LENGTH 31
#define TEXT_10_LENGTH 40
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 40
#define TEXT_13_LENGTH 40
#define TEXT_14_LENGTH 40
#define TEXT_15_LENGTH 20
#define TEXT_16_LENGTH 12
#define TEXT_17_LENGTH 40
#define TEXT_18_LENGTH 36
#define TEXT_19_LENGTH 40
#define TEXT_20_LENGTH 39
#define TEXT_21_LENGTH 36
#define TEXT_22_LENGTH 40
#define TEXT_23_LENGTH 40
#define TEXT_24_LENGTH 38
#define TEXT_25_LENGTH 39
#define TEXT_26_LENGTH 36
#define TEXT_27_LENGTH 39
#define TEXT_28_LENGTH 40
#define TEXT_29_LENGTH 40
#define TEXT_30_LENGTH 6
#define TEXT_31_LENGTH 38
#define TEXT_32_LENGTH 40
#define TEXT_33_LENGTH 30
#define TEXT_34_LENGTH 40
#define TEXT_35_LENGTH 39
#define TEXT_36_LENGTH 40
#define TEXT_37_LENGTH 39
#define TEXT_38_LENGTH 14
#define TEXT_39_LENGTH 39
#define TEXT_40_LENGTH 39
#define TEXT_41_LENGTH 40
#define TEXT_42_LENGTH 32
#define TEXT_43_LENGTH 40
#define TEXT_44_LENGTH 40
#define TEXT_45_LENGTH 40
#define TEXT_46_LENGTH 10
#define TEXT_47_LENGTH 40
#define TEXT_48_LENGTH 38
#define TEXT_49_LENGTH 31
#define TEXT_50_LENGTH 39
#define TEXT_51_LENGTH 40
#define TEXT_52_LENGTH 25
#define TEXT_53_LENGTH 40
#define TEXT_54_LENGTH 40
#define TEXT_55_LENGTH 40
#define TEXT_56_LENGTH 14
#define TEXT_57_LENGTH 39
#define TEXT_58_LENGTH 34
#define TEXT_59_LENGTH 40
#define TEXT_60_LENGTH 40
#define TEXT_61_LENGTH 40
#define TEXT_62_LENGTH 12
#define TEXT_63_LENGTH 40
#define TEXT_64_LENGTH 21
#define TEXT_65_LENGTH 40
#define TEXT_66_LENGTH 18
#define TEXT_67_LENGTH 40
#define TEXT_68_LENGTH 40
#define TEXT_69_LENGTH 40
#define TEXT_70_LENGTH 40
#define TEXT_71_LENGTH 40
#define TEXT_72_LENGTH 39
#define TEXT_73_LENGTH 22
#define TEXT_74_LENGTH 36
#define TEXT_75_LENGTH 38
#define TEXT_76_LENGTH 40
#define TEXT_77_LENGTH 40
#define TEXT_78_LENGTH 36
#define TEXT_79_LENGTH 40
#define TEXT_80_LENGTH 39
#define TEXT_81_LENGTH 16
#define TEXT_82_LENGTH 39
#define TEXT_83_LENGTH 38
#define TEXT_84_LENGTH 28
#define TEXT_85_LENGTH 38
#define TEXT_86_LENGTH 38
#define TEXT_87_LENGTH 35
#define TEXT_88_LENGTH 31
#define TEXT_89_LENGTH 40
#define TEXT_90_LENGTH 40
#define TEXT_91_LENGTH 32
#define TEXT_92_LENGTH 40
#define TEXT_93_LENGTH 8
#define TEXT_94_LENGTH 40
#define TEXT_95_LENGTH 38
#define TEXT_96_LENGTH 37
#define TEXT_97_LENGTH 37
#define TEXT_98_LENGTH 38
#define TEXT_99_LENGTH 29
#define TEXT_100_LENGTH 39
#define TEXT_101_LENGTH 40
#define TEXT_102_LENGTH 12
#define TEXT_103_LENGTH 40
#define TEXT_104_LENGTH 40
#define TEXT_105_LENGTH 26
#define TEXT_106_LENGTH 40
#define TEXT_107_LENGTH 39
#define TEXT_108_LENGTH 20
#define TEXT_109_LENGTH 37
#define TEXT_110_LENGTH 23
#define TEXT_111_LENGTH 40
#define TEXT_112_LENGTH 40
#define TEXT_113_LENGTH 18
#define TEXT_114_LENGTH 37
#define TEXT_115_LENGTH 39
#define TEXT_116_LENGTH 40
#define TEXT_117_LENGTH 39
#define TEXT_118_LENGTH 40
#define TEXT_119_LENGTH 40
#define TEXT_120_LENGTH 12
#define TEXT_121_LENGTH 34
#define TEXT_122_LENGTH 36
#define TEXT_123_LENGTH 39
#define TEXT_124_LENGTH 39
#define TEXT_125_LENGTH 34
#define TEXT_126_LENGTH 40
#define TEXT_127_LENGTH 33
#define TEXT_128_LENGTH 39
#define TEXT_129_LENGTH 39
#define TEXT_130_LENGTH 38
#define TEXT_131_LENGTH 40
#define TEXT_132_LENGTH 40
#define TEXT_133_LENGTH 40
#define TEXT_134_LENGTH 28
#define TEXT_135_LENGTH 36
#define TEXT_136_LENGTH 40
#define TEXT_137_LENGTH 40
#define TEXT_138_LENGTH 40
#define TEXT_139_LENGTH 16
#define TEXT_140_LENGTH 40
#define TEXT_141_LENGTH 38
#define TEXT_142_LENGTH 32
#define TEXT_143_LENGTH 38
#define TEXT_144_LENGTH 40
#define TEXT_145_LENGTH 10
#define TEXT_146_LENGTH 40
#define TEXT_147_LENGTH 40
#define TEXT_148_LENGTH 6
#define TEXT_149_LENGTH 40
#define TEXT_150_LENGTH 32
#define TEXT_151_LENGTH 22
#define TEXT_152_LENGTH 20
#define TEXT_153_LENGTH 14
#define TEXT_154_LENGTH 14
#define TEXT_155_LENGTH 19
#define TEXT_156_LENGTH 39
#define TEXT_157_LENGTH 14
#define TEXT_158_LENGTH 40
#define TEXT_159_LENGTH 38
#define TEXT_160_LENGTH 38
#define TEXT_161_LENGTH 39
#define TEXT_162_LENGTH 26
#define TEXT_163_LENGTH 40
#define TEXT_164_LENGTH 38
#define TEXT_165_LENGTH 30
#define TEXT_166_LENGTH 40
#define TEXT_167_LENGTH 22
#define TEXT_168_LENGTH 40
#define TEXT_169_LENGTH 39
#define TEXT_170_LENGTH 12
#define TEXT_171_LENGTH 40
#define TEXT_172_LENGTH 40
#define TEXT_173_LENGTH 39
#define TEXT_174_LENGTH 40
#define TEXT_175_LENGTH 40
#define TEXT_176_LENGTH 40
#define TEXT_177_LENGTH 39
#define TEXT_178_LENGTH 14
#define TEXT_179_LENGTH 40
#define TEXT_180_LENGTH 40
#define TEXT_181_LENGTH 8
#define TEXT_182_LENGTH 18
#define TEXT_183_LENGTH 40
#define TEXT_184_LENGTH 22
#define TEXT_185_LENGTH 13
#define TEXT_186_LENGTH 40
#define TEXT_187_LENGTH 8
#define TEXT_188_LENGTH 39
#define TEXT_189_LENGTH 40
#define TEXT_190_LENGTH 39
#define TEXT_191_LENGTH 20
#define TEXT_192_LENGTH 40
#define TEXT_193_LENGTH 39
#define TEXT_194_LENGTH 39
#define TEXT_195_LENGTH 14
#define TEXT_196_LENGTH 40
#define TEXT_197_LENGTH 39
#define TEXT_198_LENGTH 38
#define TEXT_199_LENGTH 38
#define TEXT_200_LENGTH 40
#define TEXT_201_LENGTH 40
#define TEXT_202_LENGTH 32
#define TEXT_203_LENGTH 31
#define TEXT_204_LENGTH 40
#define TEXT_205_LENGTH 38
#define TEXT_206_LENGTH 38
#define TEXT_207_LENGTH 11
#define TEXT_208_LENGTH 34
#define TEXT_209_LENGTH 26
#define TEXT_210_LENGTH 40
#define TEXT_211_LENGTH 39
#define TEXT_212_LENGTH 24
#define TEXT_213_LENGTH 39
#define TEXT_214_LENGTH 40
#define TEXT_215_LENGTH 30
#define TEXT_216_LENGTH 40
#define TEXT_217_LENGTH 20
#define TEXT_218_LENGTH 40
#define TEXT_219_LENGTH 38
#define TEXT_220_LENGTH 40
#define TEXT_221_LENGTH 14
#define TEXT_222_LENGTH 23
#define TEXT_223_LENGTH 40
#define TEXT_224_LENGTH 18
#define TEXT_225_LENGTH 40
#define TEXT_226_LENGTH 38
#define TEXT_227_LENGTH 38
#define TEXT_228_LENGTH 39
#define TEXT_229_LENGTH 22
#define TEXT_230_LENGTH 32
#define TEXT_231_LENGTH 39
#define TEXT_232_LENGTH 30
#define TEXT_233_LENGTH 37
#define TEXT_234_LENGTH 40
#define TEXT_235_LENGTH 12
#define TEXT_236_LENGTH 40
#define TEXT_237_LENGTH 32
#define TEXT_238_LENGTH 40
#define TEXT_239_LENGTH 38
#define TEXT_240_LENGTH 29
#define TEXT_241_LENGTH 27
#define TEXT_242_LENGTH 36
#define TEXT_243_LENGTH 28
#define TEXT_244_LENGTH 40
#define TEXT_245_LENGTH 14
#define TEXT_246_LENGTH 40
#define TEXT_247_LENGTH 39
#define TEXT_248_LENGTH 38
#define TEXT_249_LENGTH 40
#define TEXT_250_LENGTH 40
#define TEXT_251_LENGTH 8
#define TEXT_252_LENGTH 40
#define TEXT_253_LENGTH 39
#define TEXT_254_LENGTH 26
#define TEXT_255_LENGTH 37
#define TEXT_256_LENGTH 30
#define TEXT_257_LENGTH 39
#define TEXT_258_LENGTH 36
#define TEXT_259_LENGTH 39
#define TEXT_260_LENGTH 39
#define TEXT_261_LENGTH 40
#define TEXT_262_LENGTH 31
#define TEXT_263_LENGTH 39
#define TEXT_264_LENGTH 39
#define TEXT_265_LENGTH 36
#define TEXT_266_LENGTH 36
#define TEXT_267_LENGTH 36
#define TEXT_268_LENGTH 31
#define TEXT_269_LENGTH 37
#define TEXT_270_LENGTH 36
#define TEXT_271_LENGTH 16
#define TEXT_272_LENGTH 16
#define TEXT_273_LENGTH 37
#define TEXT_274_LENGTH 40
#define TEXT_275_LENGTH 40
#define TEXT_276_LENGTH 36
#define TEXT_277_LENGTH 28
#define TEXT_278_LENGTH 40
#define TEXT_279_LENGTH 40
#define TEXT_280_LENGTH 40
#define TEXT_281_LENGTH 12
#define TEXT_282_LENGTH 40
#define TEXT_283_LENGTH 40
#define TEXT_284_LENGTH 12
#define TEXT_285_LENGTH 40
#define TEXT_286_LENGTH 39
#define TEXT_287_LENGTH 40
#define TEXT_288_LENGTH 40
#define TEXT_289_LENGTH 20
#define TEXT_290_LENGTH 40
#define TEXT_291_LENGTH 40
#define TEXT_292_LENGTH 12
#define TEXT_293_LENGTH 40
#define TEXT_294_LENGTH 24
#define TEXT_295_LENGTH 35
#define TEXT_296_LENGTH 16
#define TEXT_297_LENGTH 38
#define TEXT_298_LENGTH 38
#define TEXT_299_LENGTH 32
#define TEXT_300_LENGTH 40
#define TEXT_301_LENGTH 20
#define TEXT_302_LENGTH 38
#define TEXT_303_LENGTH 40
#define TEXT_304_LENGTH 38
#define TEXT_305_LENGTH 32
#define TEXT_306_LENGTH 40
#define TEXT_307_LENGTH 40
#define TEXT_308_LENGTH 23
#define TEXT_309_LENGTH 38
#define TEXT_310_LENGTH 40
#define TEXT_311_LENGTH 39
#define TEXT_312_LENGTH 10
#define TEXT_313_LENGTH 30
#define TEXT_314_LENGTH 38
#define TEXT_315_LENGTH 40
#define TEXT_316_LENGTH 38
#define TEXT_317_LENGTH 39
#define TEXT_318_LENGTH 39
#define TEXT_319_LENGTH 38
#define TEXT_320_LENGTH 40
#define TEXT_321_LENGTH 40
#define TEXT_322_LENGTH 40
#define TEXT_323_LENGTH 26
#define TEXT_324_LENGTH 34
#define TEXT_325_LENGTH 38
#define TEXT_326_LENGTH 38
#define TEXT_327_LENGTH 22
#define TEXT_328_LENGTH 40
#define TEXT_329_LENGTH 14
#define TEXT_330_LENGTH 40
#define TEXT_331_LENGTH 40
#define TEXT_332_LENGTH 40
#define TEXT_333_LENGTH 40
#define TEXT_334_LENGTH 34
#define TEXT_335_LENGTH 40
#define TEXT_336_LENGTH 8
#define TEXT_337_LENGTH 39
#define TEXT_338_LENGTH 40
#define TEXT_339_LENGTH 40
#define TEXT_340_LENGTH 40
#define TEXT_341_LENGTH 10
#define TEXT_342_LENGTH 39
#define TEXT_343_LENGTH 26
#define TEXT_344_LENGTH 39
#define TEXT_345_LENGTH 36
#define TEXT_346_LENGTH 9
#define TEXT_347_LENGTH 39
#define TEXT_348_LENGTH 38
#define TEXT_349_LENGTH 39
#define TEXT_350_LENGTH 39
#define TEXT_351_LENGTH 40
#define TEXT_352_LENGTH 34
#define TEXT_353_LENGTH 22
#define TEXT_354_LENGTH 40
#define TEXT_355_LENGTH 40
#define TEXT_356_LENGTH 39
#define TEXT_357_LENGTH 36
#define TEXT_358_LENGTH 39
#define TEXT_359_LENGTH 35
#define TEXT_360_LENGTH 39
#define TEXT_361_LENGTH 40
#define TEXT_362_LENGTH 38
#define TEXT_363_LENGTH 12
#define TEXT_364_LENGTH 40
#define TEXT_365_LENGTH 40
#define TEXT_366_LENGTH 37
#define TEXT_367_LENGTH 23
#define TEXT_368_LENGTH 39
#define TEXT_369_LENGTH 14
#define TEXT_370_LENGTH 37
#define TEXT_371_LENGTH 40
#define TEXT_372_LENGTH 40
#define TEXT_373_LENGTH 28
#define TEXT_374_LENGTH 39
#define TEXT_375_LENGTH 40
#define TEXT_376_LENGTH 40
#define TEXT_377_LENGTH 40
#define TEXT_378_LENGTH 40
#define TEXT_379_LENGTH 14
#define TEXT_380_LENGTH 40
#define TEXT_381_LENGTH 38
#define TEXT_382_LENGTH 40
#define TEXT_383_LENGTH 34
#define TEXT_384_LENGTH 38
#define TEXT_385_LENGTH 28
#define TEXT_386_LENGTH 40
#define TEXT_387_LENGTH 27
#define TEXT_388_LENGTH 40
#define TEXT_389_LENGTH 18
#define TEXT_390_LENGTH 40
#define TEXT_391_LENGTH 39
#define TEXT_392_LENGTH 31
#define TEXT_393_LENGTH 21
#define TEXT_394_LENGTH 21
#define TEXT_395_LENGTH 29
#define TEXT_396_LENGTH 40
#define TEXT_397_LENGTH 31
#define TEXT_398_LENGTH 35
#define TEXT_399_LENGTH 37
#define TEXT_400_LENGTH 39
#define TEXT_401_LENGTH 12
#define TEXT_402_LENGTH 33
#define TEXT_403_LENGTH 37
#define TEXT_404_LENGTH 34
#define TEXT_405_LENGTH 40
#define TEXT_406_LENGTH 32
#define TEXT_407_LENGTH 38
#define TEXT_408_LENGTH 40
#define TEXT_409_LENGTH 25
#define TEXT_410_LENGTH 39
#define TEXT_411_LENGTH 30
#define TEXT_412_LENGTH 34
#define TEXT_413_LENGTH 20
#define TEXT_414_LENGTH 30
#define TEXT_415_LENGTH 39
#define TEXT_416_LENGTH 32
#define TEXT_417_LENGTH 37
#define TEXT_418_LENGTH 25
#define TEXT_419_LENGTH 26
#define TEXT_420_LENGTH 36
#define TEXT_421_LENGTH 22
#define TEXT_422_LENGTH 30
#define TEXT_423_LENGTH 39
#define TEXT_424_LENGTH 19
#define TEXT_425_LENGTH 28
#define TEXT_426_LENGTH 40
#define TEXT_427_LENGTH 36
#define TEXT_428_LENGTH 40
#define TEXT_429_LENGTH 30
#define TEXT_430_LENGTH 33
#define TEXT_431_LENGTH 36
#define TEXT_432_LENGTH 39
#define TEXT_433_LENGTH 38
#define TEXT_434_LENGTH 15
#define TEXT_435_LENGTH 39
#define TEXT_436_LENGTH 30
#define TEXT_437_LENGTH 40
#define TEXT_438_LENGTH 39
#define TEXT_439_LENGTH 8
#define TEXT_440_LENGTH 39
#define TEXT_441_LENGTH 39
#define TEXT_442_LENGTH 39
#define TEXT_443_LENGTH 10
#define TEXT_444_LENGTH 40
#define TEXT_445_LENGTH 23
#define TEXT_446_LENGTH 34
#define TEXT_447_LENGTH 30
#define TEXT_448_LENGTH 37
#define TEXT_449_LENGTH 40
#define TEXT_450_LENGTH 27
#define TEXT_451_LENGTH 40
#define TEXT_452_LENGTH 40
#define TEXT_453_LENGTH 40
#define TEXT_454_LENGTH 37
#define TEXT_455_LENGTH 38
#define TEXT_456_LENGTH 40
#define TEXT_457_LENGTH 22
#define TEXT_458_LENGTH 40
#define TEXT_459_LENGTH 40
#define TEXT_460_LENGTH 35
#define TEXT_461_LENGTH 16
#define TEXT_462_LENGTH 32
#define TEXT_463_LENGTH 34
#define TEXT_464_LENGTH 40
#define TEXT_465_LENGTH 36
#define TEXT_466_LENGTH 39
#define TEXT_467_LENGTH 38
#define TEXT_468_LENGTH 39
#define TEXT_469_LENGTH 37
#define TEXT_470_LENGTH 38
#define TEXT_471_LENGTH 40
#define TEXT_472_LENGTH 26
#define TEXT_473_LENGTH 40
#define TEXT_474_LENGTH 12
#define TEXT_475_LENGTH 37
#define TEXT_476_LENGTH 40
#define TEXT_477_LENGTH 40
#define TEXT_478_LENGTH 40
#define TEXT_479_LENGTH 18
#define TEXT_480_LENGTH 40
#define TEXT_481_LENGTH 33
#define TEXT_482_LENGTH 38
#define TEXT_483_LENGTH 40
#define TEXT_484_LENGTH 39
#define TEXT_485_LENGTH 18
#define TEXT_486_LENGTH 27
#define TEXT_487_LENGTH 38
#define TEXT_488_LENGTH 20
#define TEXT_489_LENGTH 40
#define TEXT_490_LENGTH 14
#define TEXT_491_LENGTH 24
#define TEXT_492_LENGTH 40
#define TEXT_493_LENGTH 29
#define TEXT_494_LENGTH 40
#define TEXT_495_LENGTH 6
#define TEXT_496_LENGTH 37
#define TEXT_497_LENGTH 38
#define TEXT_498_LENGTH 38
#define TEXT_499_LENGTH 38
#define TEXT_500_LENGTH 40
#define TEXT_501_LENGTH 6
#define TEXT_502_LENGTH 39
#define TEXT_503_LENGTH 37
#define TEXT_504_LENGTH 16
#define TEXT_505_LENGTH 40
#define TEXT_506_LENGTH 29
#define TEXT_507_LENGTH 40
#define TEXT_508_LENGTH 26
#define TEXT_509_LENGTH 40
#define TEXT_510_LENGTH 40
#define TEXT_511_LENGTH 36
#define TEXT_512_LENGTH 38
#define TEXT_513_LENGTH 38
#define TEXT_514_LENGTH 22
#define TEXT_515_LENGTH 39
#define TEXT_516_LENGTH 16
#define TEXT_517_LENGTH 36
#define TEXT_518_LENGTH 31
#include "summary/summary/text.inc"

enum summary_mode {
    SUMMARY_FADE_IN,
    SUMMARY_SCROLL,
    SUMMARY_FADE_OUT,
    SUMMARY_FADED,
    SUMMARY_DONE
};

/* summary_flags' bits */
/* the text task's destructor has run, as it does once the page has
 * scrolled off */
#define SUMMARY_TEXT_DESTROYED 1
/* a bit that would have summary_step return OVERLAY_BACK, not OVERLAY_DONE,
 * once the screen has faded out. nothing sets it */
#define SUMMARY_LEAVE 2

/* the text task's modes, in its work->mode */
enum summary_text_mode { TEXT_START = 0, TEXT_SCROLL = 2, TEXT_DONE = 4 };

/* the bitmap the page prints into: its top left on the screen, and its
 * size in dots */
#define VIEW_X 80
#define VIEW_Y 120
#define VIEW_WIDTH 480
#define VIEW_HEIGHT 240
/* how much the cover's alpha changes each step of a fade */
#define FADE_STEP 0.05F
/* a glyph's height in dots. a line takes it plus the bitmap's line_spacing,
 * the sum bitmap_newline moves the cursor down by */
#define GLYPH_SIZE 24

/* summary_text_create's settings for the text task */
struct summary_text_spec {
    int level; /* the task's level */
    void (*start)(void); /* called right after task_create, or null */
    int unk_08;
    void (*done)(void); /* called as the task is destroyed, or null */
    int story; /* the story whose page the task shows */
    int unk_14;
    int unk_18;
};

/* the text task's record, in task->extra */
struct summary_text {
    struct bitmap bitmap; /* the page's lines print into it */
    /* the corners the bitmap's texture draws through, as
     * summary_window_place sets them */
    NJS_TEXTURE_VTX window[8];
    int story;
    int unk_100;
    int page; /* the page shown, as summary_find_page gives it each step */
    int unk_108;
    int unk_10c;
    void (*done)(void);
};

/* in DHEAD, the section at SUMMARY's first address, where the main loop
 * reads the table */
#pragma section HEAD
struct summary summary_entries = {
    summary_start,         summary_step,         summary_end,
    tutorial_screen_start, tutorial_screen_step, tutorial_screen_end,
    staff_roll_start,      staff_roll_step,      staff_roll_end,
};
#pragma section

static Uint8 summary_tunes[] = {
    MUSIC_THESONIC, MUSIC_THEMILES, MUSIC_THEKNKLS, MUSIC_THEAMY,
    MUSIC_THEBIG,   MUSIC_THEE102,  MUSIC_CHAOS,
};

static char *sonic_summary_lines[218] = {
    TEXT_1,   TEXT_2,   TEXT_3,   TEXT_4,   TEXT_5,   TEXT_6,   TEXT_7,
    "",       TEXT_8,   TEXT_9,   TEXT_10,  TEXT_11,  TEXT_12,  TEXT_13,
    TEXT_14,  TEXT_15,  "",       TEXT_16,  TEXT_17,  "",       TEXT_18,
    TEXT_10,  TEXT_11,  TEXT_19,  TEXT_20,  TEXT_21,  TEXT_22,  TEXT_23,
    TEXT_24,  TEXT_25,  TEXT_26,  "",       TEXT_27,  TEXT_28,  TEXT_29,
    TEXT_30,  TEXT_10,  TEXT_11,  TEXT_31,  TEXT_32,  TEXT_33,  TEXT_34,
    TEXT_35,  TEXT_36,  TEXT_37,  TEXT_38,  TEXT_10,  TEXT_11,  TEXT_39,
    TEXT_40,  TEXT_41,  TEXT_42,  TEXT_43,  TEXT_44,  TEXT_45,  TEXT_46,
    TEXT_10,  TEXT_11,  TEXT_47,  TEXT_48,  "",       TEXT_49,  TEXT_10,
    TEXT_11,  TEXT_50,  TEXT_51,  TEXT_52,  "",       TEXT_53,  TEXT_54,
    TEXT_55,  TEXT_56,  TEXT_57,  TEXT_58,  TEXT_10,  TEXT_11,  TEXT_59,
    TEXT_60,  TEXT_61,  TEXT_62,  "",       TEXT_63,  TEXT_64,  TEXT_65,
    TEXT_66,  TEXT_10,  TEXT_11,  TEXT_67,  TEXT_68,  TEXT_69,  "",
    TEXT_70,  TEXT_71,  TEXT_72,  TEXT_73,  TEXT_10,  TEXT_11,  TEXT_74,
    TEXT_75,  TEXT_76,  TEXT_77,  "",       TEXT_78,  TEXT_79,  TEXT_80,
    TEXT_81,  TEXT_10,  TEXT_11,  TEXT_82,  TEXT_83,  TEXT_84,  TEXT_85,
    TEXT_86,  "",       TEXT_87,  TEXT_88,  TEXT_10,  TEXT_11,  TEXT_89,
    TEXT_90,  TEXT_91,  "",       TEXT_92,  TEXT_93,  TEXT_10,  TEXT_11,
    TEXT_94,  TEXT_95,  TEXT_96,  TEXT_97,  TEXT_98,  TEXT_99,  "",
    TEXT_100, TEXT_101, TEXT_102, TEXT_10,  TEXT_11,  TEXT_103, TEXT_104,
    TEXT_105, TEXT_106, TEXT_107, TEXT_108, "",       TEXT_109, TEXT_110,
    TEXT_10,  TEXT_11,  TEXT_111, TEXT_112, TEXT_113, "",       TEXT_114,
    TEXT_115, TEXT_116, TEXT_10,  TEXT_11,  TEXT_117, TEXT_118, TEXT_119,
    TEXT_120, "",       TEXT_121, TEXT_122, TEXT_10,  TEXT_11,  TEXT_123,
    TEXT_124, TEXT_125, TEXT_126, TEXT_127, "",       TEXT_128, TEXT_129,
    TEXT_130, TEXT_131, TEXT_10,  TEXT_11,  TEXT_132, TEXT_133, TEXT_134,
    TEXT_135, "",       TEXT_136, TEXT_10,  TEXT_11,  TEXT_137, TEXT_138,
    TEXT_139, TEXT_140, TEXT_141, TEXT_142, "",       TEXT_143, TEXT_144,
    TEXT_145, TEXT_10,  TEXT_11,  TEXT_146, TEXT_147, TEXT_148, TEXT_149,
    TEXT_150, TEXT_151, TEXT_10,  TEXT_11,  TEXT_152, TEXT_153, TEXT_154,
    TEXT_155, "",       TEXT_156, TEXT_157, "",       TEXT_158, TEXT_159,
    TEXT_160,
};
static struct summary_page sonic_summary_pages[21] = {
    {1.0F, 10, &sonic_summary_lines[0]},
    {1.0F, 11, &sonic_summary_lines[10]},
    {1.0F, 15, &sonic_summary_lines[21]},
    {1.0F, 10, &sonic_summary_lines[36]},
    {1.0F, 10, &sonic_summary_lines[46]},
    {1.0F, 6, &sonic_summary_lines[56]},
    {1.0F, 12, &sonic_summary_lines[62]},
    {1.0F, 11, &sonic_summary_lines[74]},
    {1.0F, 10, &sonic_summary_lines[85]},
    {1.0F, 11, &sonic_summary_lines[95]},
    {1.0F, 10, &sonic_summary_lines[106]},
    {1.0F, 8, &sonic_summary_lines[116]},
    {1.0F, 12, &sonic_summary_lines[124]},
    {1.0F, 11, &sonic_summary_lines[136]},
    {1.0F, 9, &sonic_summary_lines[147]},
    {1.0F, 9, &sonic_summary_lines[156]},
    {1.0F, 12, &sonic_summary_lines[165]},
    {1.0F, 8, &sonic_summary_lines[177]},
    {1.0F, 12, &sonic_summary_lines[185]},
    {1.0F, 8, &sonic_summary_lines[197]},
    {1.0F, 13, &sonic_summary_lines[205]},
};
static char *tails_summary_lines[132] = {
    TEXT_161, TEXT_162, TEXT_163, TEXT_164, TEXT_165, TEXT_166, TEXT_167,
    "",       TEXT_168, TEXT_169, TEXT_170, TEXT_161, TEXT_162, TEXT_171,
    TEXT_172, TEXT_173, TEXT_174, TEXT_175, TEXT_176, TEXT_177, TEXT_178,
    "",       TEXT_179, TEXT_180, TEXT_181, TEXT_182, "",       TEXT_183,
    TEXT_184, TEXT_185, "",       TEXT_186, TEXT_187, TEXT_182, "",
    TEXT_188, TEXT_189, TEXT_190, TEXT_191, "",       TEXT_192, TEXT_193,
    TEXT_187, TEXT_182, TEXT_194, TEXT_195, TEXT_196, TEXT_197, "",
    TEXT_198, TEXT_182, TEXT_199, TEXT_200, "",       TEXT_201, TEXT_202,
    TEXT_203, TEXT_182, TEXT_204, TEXT_205, TEXT_206, TEXT_207, TEXT_208,
    "",       TEXT_209, TEXT_182, TEXT_210, TEXT_211, TEXT_212, "",
    TEXT_213, TEXT_214, TEXT_215, TEXT_182, TEXT_216, TEXT_217, "",
    TEXT_218, TEXT_219, TEXT_220, TEXT_221, "",       TEXT_222, TEXT_182,
    TEXT_223, TEXT_224, "",       TEXT_225, TEXT_226, TEXT_227, TEXT_228,
    "",       TEXT_229, TEXT_230, TEXT_182, TEXT_231, TEXT_232, "",
    TEXT_233, TEXT_234, TEXT_235, TEXT_182, TEXT_236, TEXT_237, TEXT_238,
    TEXT_239, "",       TEXT_240, TEXT_241, TEXT_182, TEXT_242, TEXT_243,
    "",       TEXT_244, TEXT_245, TEXT_182, TEXT_246, TEXT_247, TEXT_248,
    TEXT_249, TEXT_250, TEXT_251, "",       TEXT_252, TEXT_253, TEXT_254,
    "",       "",       "",       "",       "",       "",
};
static struct summary_page tails_summary_pages[20] = {
    {1.0F, 11, &tails_summary_lines[0]},  {1.0F, 14, &tails_summary_lines[11]},
    {1.0F, 8, &tails_summary_lines[25]},  {1.0F, 10, &tails_summary_lines[33]},
    {1.0F, 7, &tails_summary_lines[43]},  {1.0F, 7, &tails_summary_lines[50]},
    {1.0F, 8, &tails_summary_lines[57]},  {1.0F, 8, &tails_summary_lines[65]},
    {1.0F, 10, &tails_summary_lines[73]}, {1.0F, 11, &tails_summary_lines[83]},
    {1.0F, 7, &tails_summary_lines[94]},  {1.0F, 8, &tails_summary_lines[101]},
    {1.0F, 6, &tails_summary_lines[109]}, {1.0F, 11, &tails_summary_lines[115]},
    {1.0F, 1, &tails_summary_lines[126]}, {1.0F, 1, &tails_summary_lines[127]},
    {1.0F, 1, &tails_summary_lines[128]}, {1.0F, 1, &tails_summary_lines[129]},
    {1.0F, 1, &tails_summary_lines[130]}, {1.0F, 1, &tails_summary_lines[131]},
};
static char *knuckles_summary_lines[131] = {
    TEXT_255, TEXT_256, "",       TEXT_257, TEXT_258, TEXT_259, TEXT_260,
    TEXT_261, TEXT_262, TEXT_263, TEXT_264, "",       TEXT_265, TEXT_255,
    TEXT_256, TEXT_266, TEXT_267, TEXT_268, "",       TEXT_269, TEXT_270,
    TEXT_271, TEXT_255, TEXT_256, TEXT_272, TEXT_273, TEXT_274, TEXT_275,
    "",       TEXT_276, TEXT_277, TEXT_255, TEXT_256, TEXT_278, TEXT_279,
    "",       TEXT_280, TEXT_281, TEXT_255, TEXT_256, TEXT_282, TEXT_283,
    TEXT_284, "",       TEXT_285, TEXT_286, TEXT_287, TEXT_288, TEXT_289,
    TEXT_255, TEXT_256, TEXT_290, TEXT_291, TEXT_292, TEXT_293, TEXT_294,
    "",       TEXT_295, TEXT_255, TEXT_256, "",       TEXT_296, TEXT_297,
    TEXT_298, TEXT_299, "",       TEXT_300, TEXT_301, TEXT_255, TEXT_256,
    "",       TEXT_302, TEXT_303, TEXT_304, TEXT_305, "",       TEXT_306,
    TEXT_307, TEXT_308, TEXT_255, TEXT_256, TEXT_309, TEXT_310, TEXT_311,
    TEXT_312, "",       TEXT_313, TEXT_255, TEXT_256, "",       TEXT_314,
    TEXT_315, TEXT_316, TEXT_317, TEXT_318, "",       TEXT_319, TEXT_320,
    TEXT_255, TEXT_256, "",       TEXT_321, TEXT_322, TEXT_323, "",
    TEXT_324, TEXT_255, TEXT_256, "",       TEXT_325, TEXT_326, TEXT_327,
    "",       TEXT_328, TEXT_329, TEXT_255, TEXT_256, "",       TEXT_330,
    TEXT_331, "",       TEXT_332, TEXT_333, TEXT_334, "",       "",
    "",       "",       "",       "",       "",
};
static struct summary_page knuckles_summary_pages[20] = {
    {1.0F, 13, &knuckles_summary_lines[0]},
    {1.0F, 9, &knuckles_summary_lines[13]},
    {1.0F, 9, &knuckles_summary_lines[22]},
    {1.0F, 7, &knuckles_summary_lines[31]},
    {1.0F, 11, &knuckles_summary_lines[38]},
    {1.0F, 9, &knuckles_summary_lines[49]},
    {1.0F, 10, &knuckles_summary_lines[58]},
    {1.0F, 11, &knuckles_summary_lines[68]},
    {1.0F, 8, &knuckles_summary_lines[79]},
    {1.0F, 11, &knuckles_summary_lines[87]},
    {1.0F, 8, &knuckles_summary_lines[98]},
    {1.0F, 9, &knuckles_summary_lines[106]},
    {1.0F, 9, &knuckles_summary_lines[115]},
    {1.0F, 1, &knuckles_summary_lines[124]},
    {1.0F, 1, &knuckles_summary_lines[125]},
    {1.0F, 1, &knuckles_summary_lines[126]},
    {1.0F, 1, &knuckles_summary_lines[127]},
    {1.0F, 1, &knuckles_summary_lines[128]},
    {1.0F, 1, &knuckles_summary_lines[129]},
    {1.0F, 1, &knuckles_summary_lines[130]},
};
static char *amy_summary_lines[85] = {
    TEXT_335, TEXT_336, TEXT_337, TEXT_338, TEXT_339, TEXT_340, TEXT_341,
    TEXT_342, TEXT_343, "",       TEXT_344, TEXT_345, TEXT_335, TEXT_346,
    TEXT_347, TEXT_348, TEXT_349, "",       TEXT_350, TEXT_351, TEXT_352,
    "",       TEXT_353, TEXT_354, TEXT_335, TEXT_346, TEXT_355, TEXT_356,
    TEXT_357, TEXT_358, TEXT_359, "",       TEXT_360, TEXT_361, TEXT_362,
    TEXT_363, TEXT_364, TEXT_335, TEXT_346, TEXT_365, TEXT_366, TEXT_367,
    "",       TEXT_368, TEXT_369, TEXT_370, TEXT_335, TEXT_346, TEXT_371,
    TEXT_372, TEXT_373, TEXT_374, TEXT_375, TEXT_376, TEXT_377, TEXT_378,
    TEXT_379, "",       TEXT_380, TEXT_381, "",       TEXT_335, TEXT_346,
    TEXT_382, TEXT_383, TEXT_384, TEXT_385, "",       TEXT_386, TEXT_387,
    TEXT_388, TEXT_389, "",       "",       "",       "",       "",
    "",       "",       "",       "",       "",       "",       "",
    "",
};
static struct summary_page amy_summary_pages[20] = {
    {1.0F, 12, &amy_summary_lines[0]},  {1.0F, 12, &amy_summary_lines[12]},
    {1.0F, 13, &amy_summary_lines[24]}, {1.0F, 9, &amy_summary_lines[37]},
    {1.0F, 14, &amy_summary_lines[46]}, {1.0F, 1, &amy_summary_lines[60]},
    {1.0F, 11, &amy_summary_lines[61]}, {1.0F, 1, &amy_summary_lines[72]},
    {1.0F, 1, &amy_summary_lines[73]},  {1.0F, 1, &amy_summary_lines[74]},
    {1.0F, 1, &amy_summary_lines[75]},  {1.0F, 1, &amy_summary_lines[76]},
    {1.0F, 1, &amy_summary_lines[77]},  {1.0F, 1, &amy_summary_lines[78]},
    {1.0F, 1, &amy_summary_lines[79]},  {1.0F, 1, &amy_summary_lines[80]},
    {1.0F, 1, &amy_summary_lines[81]},  {1.0F, 1, &amy_summary_lines[82]},
    {1.0F, 1, &amy_summary_lines[83]},  {1.0F, 1, &amy_summary_lines[84]},
};
static char *e102_summary_lines[99] = {
    TEXT_390, TEXT_391, TEXT_392, TEXT_393, TEXT_394, TEXT_395, TEXT_390,
    TEXT_391, TEXT_392, TEXT_396, TEXT_397, TEXT_398, TEXT_399, TEXT_400,
    TEXT_401, TEXT_402, TEXT_403, TEXT_404, TEXT_405, TEXT_406, TEXT_399,
    TEXT_400, TEXT_401, TEXT_407, TEXT_408, TEXT_409, TEXT_399, TEXT_400,
    TEXT_410, TEXT_411, TEXT_412, TEXT_413, TEXT_414, TEXT_399, TEXT_400,
    TEXT_401, TEXT_412, TEXT_413, TEXT_415, TEXT_416, TEXT_417, TEXT_418,
    TEXT_399, TEXT_400, TEXT_401, TEXT_419, TEXT_420, TEXT_421, TEXT_422,
    TEXT_423, TEXT_399, TEXT_400, TEXT_401, TEXT_424, TEXT_425, TEXT_426,
    TEXT_399, TEXT_400, TEXT_401, TEXT_427, TEXT_428, TEXT_429, TEXT_414,
    TEXT_430, TEXT_431, TEXT_432, TEXT_433, TEXT_434, TEXT_435, TEXT_436,
    TEXT_430, TEXT_437, TEXT_438, TEXT_439, TEXT_440, TEXT_441, TEXT_430,
    TEXT_437, TEXT_438, TEXT_439, TEXT_442, TEXT_443, TEXT_444, TEXT_445,
    TEXT_446, TEXT_430, TEXT_437, TEXT_438, TEXT_439, TEXT_447, TEXT_448,
    TEXT_449, "",       "",       "",       "",       "",       "",
    "",
};
static struct summary_page e102_summary_pages[20] = {
    {1.0F, 6, &e102_summary_lines[0]},  {1.0F, 6, &e102_summary_lines[6]},
    {1.0F, 8, &e102_summary_lines[12]}, {1.0F, 6, &e102_summary_lines[20]},
    {1.0F, 7, &e102_summary_lines[26]}, {1.0F, 9, &e102_summary_lines[33]},
    {1.0F, 8, &e102_summary_lines[42]}, {1.0F, 6, &e102_summary_lines[50]},
    {1.0F, 7, &e102_summary_lines[56]}, {1.0F, 7, &e102_summary_lines[63]},
    {1.0F, 6, &e102_summary_lines[70]}, {1.0F, 9, &e102_summary_lines[76]},
    {1.0F, 7, &e102_summary_lines[85]}, {1.0F, 1, &e102_summary_lines[92]},
    {1.0F, 1, &e102_summary_lines[93]}, {1.0F, 1, &e102_summary_lines[94]},
    {1.0F, 1, &e102_summary_lines[95]}, {1.0F, 1, &e102_summary_lines[96]},
    {1.0F, 1, &e102_summary_lines[97]}, {1.0F, 1, &e102_summary_lines[98]},
};
static char *big_summary_lines[77] = {
    TEXT_450, "",       TEXT_451, TEXT_452, "",       TEXT_453, TEXT_454,
    TEXT_455, "",       TEXT_456, TEXT_457, TEXT_458, TEXT_450, "",
    TEXT_459, TEXT_460, TEXT_461, TEXT_462, "",       TEXT_463, TEXT_450,
    "",       TEXT_464, TEXT_465, "",       TEXT_466, TEXT_467, TEXT_450,
    "",       TEXT_468, TEXT_469, "",       TEXT_470, TEXT_450, "",
    TEXT_471, TEXT_472, "",       TEXT_473, TEXT_474, TEXT_475, TEXT_450,
    "",       TEXT_476, TEXT_477, TEXT_478, TEXT_479, "",       TEXT_480,
    TEXT_481, TEXT_482, TEXT_450, "",       TEXT_483, TEXT_484, TEXT_485,
    "",       TEXT_486, TEXT_450, "",       TEXT_487, TEXT_488, "",
    TEXT_489, TEXT_490, "",       "",       "",       "",       "",
    "",       "",       "",       "",       "",       "",       "",
};
static struct summary_page big_summary_pages[20] = {
    {1.0F, 12, &big_summary_lines[0]}, {1.0F, 8, &big_summary_lines[12]},
    {1.0F, 7, &big_summary_lines[20]}, {1.0F, 6, &big_summary_lines[27]},
    {1.0F, 8, &big_summary_lines[33]}, {1.0F, 10, &big_summary_lines[41]},
    {1.0F, 7, &big_summary_lines[51]}, {1.0F, 7, &big_summary_lines[58]},
    {1.0F, 1, &big_summary_lines[65]}, {1.0F, 1, &big_summary_lines[66]},
    {1.0F, 1, &big_summary_lines[67]}, {1.0F, 1, &big_summary_lines[68]},
    {1.0F, 1, &big_summary_lines[69]}, {1.0F, 1, &big_summary_lines[70]},
    {1.0F, 1, &big_summary_lines[71]}, {1.0F, 1, &big_summary_lines[72]},
    {1.0F, 1, &big_summary_lines[73]}, {1.0F, 1, &big_summary_lines[74]},
    {1.0F, 1, &big_summary_lines[75]}, {1.0F, 1, &big_summary_lines[76]},
};
static char *supersonic_summary_lines[58] = {
    TEXT_10,  TEXT_491, TEXT_492, TEXT_493, TEXT_494, TEXT_495, "",
    TEXT_496, TEXT_497, "",       TEXT_498, TEXT_10,  TEXT_11,  TEXT_499,
    TEXT_500, TEXT_501, TEXT_502, "",       TEXT_503, TEXT_504, TEXT_505,
    TEXT_10,  TEXT_11,  TEXT_506, TEXT_507, TEXT_508, "",       TEXT_509,
    "",       TEXT_510, TEXT_10,  TEXT_11,  TEXT_511, TEXT_512, TEXT_513,
    TEXT_514, "",       TEXT_515, TEXT_516, "",       TEXT_517, TEXT_518,
    "",       "",       "",       "",       "",       "",       "",
    "",       "",       "",       "",       "",       "",       "",
    "",       "",
};
static struct summary_page supersonic_summary_pages[20] = {
    {1.0F, 11, &supersonic_summary_lines[0]},
    {1.0F, 10, &supersonic_summary_lines[11]},
    {1.0F, 9, &supersonic_summary_lines[21]},
    {1.0F, 12, &supersonic_summary_lines[30]},
    {1.0F, 1, &supersonic_summary_lines[42]},
    {1.0F, 1, &supersonic_summary_lines[43]},
    {1.0F, 1, &supersonic_summary_lines[44]},
    {1.0F, 1, &supersonic_summary_lines[45]},
    {1.0F, 1, &supersonic_summary_lines[46]},
    {1.0F, 1, &supersonic_summary_lines[47]},
    {1.0F, 1, &supersonic_summary_lines[48]},
    {1.0F, 1, &supersonic_summary_lines[49]},
    {1.0F, 1, &supersonic_summary_lines[50]},
    {1.0F, 1, &supersonic_summary_lines[51]},
    {1.0F, 1, &supersonic_summary_lines[52]},
    {1.0F, 1, &supersonic_summary_lines[53]},
    {1.0F, 1, &supersonic_summary_lines[54]},
    {1.0F, 1, &supersonic_summary_lines[55]},
    {1.0F, 1, &supersonic_summary_lines[56]},
    {1.0F, 1, &supersonic_summary_lines[57]},
};
/* each story's script, as an index into summary_scripts: its own number */
static Uint8 summary_story_scripts[7] = {0, 1, 2, 3, 4, 5, 6};
/* each story's pages, as its script gives them. nothing reads them */
static struct summary_page *summary_story_pages[7] = {
    sonic_summary_pages,      tails_summary_pages, knuckles_summary_pages,
    amy_summary_pages,        big_summary_pages,   e102_summary_pages,
    supersonic_summary_pages,
};
static Uint16 sonic_summary_flags[21] = {
    176,
    FLAG_SONIC_CLEAR_BEACH,
    179,
    FLAG_SONIC_CLEAR_WINDY,
    FLAG_SONIC_CLEAR_CASINO,
    FLAG_SONIC_CLEAR_ICECAP,
    FLAG_SONIC_MBOSS_KNUCKLES,
    177,
    181,
    SUMMARY_PLAYED | 18,
    FLAG_SONIC_CLEAR_TWINKLE_PARK,
    170,
    FLAG_SONIC_MR_ISLAND_DOOR,
    171,
    182,
    SUMMARY_PLAYED | 258,
    164,
    SUMMARY_PLAYED | 263,
    178,
    174,
    SUMMARY_PLAYED | 35,
};
static Uint16 tails_summary_flags[14] = {
    SUMMARY_PLAYED | 50,
    233,
    FLAG_TAILS_CLEAR_WINDY,
    FLAG_TAILS_CLEAR_CASINO,
    FLAG_TAILS_CLEAR_ICECAP,
    FLAG_TAILS_MBOSS_KNUCKLES,
    231,
    234,
    SUMMARY_PLAYED | 66,
    SUMMARY_PLAYED | 69,
    235,
    SUMMARY_PLAYED | 274,
    229,
    223,
};
static Uint16 knuckles_summary_flags[13] = {
    SUMMARY_PLAYED | 130,
    288,
    287,
    SUMMARY_PLAYED | 133,
    FLAG_KNUCKLES_CLEAR_CHAOS2,
    289,
    FLAG_KNUCKLES_MBOSS_SONIC,
    293,
    290,
    SUMMARY_PLAYED | 143,
    SUMMARY_PLAYED | 148,
    294,
    SUMMARY_PLAYED | 152,
};
static Uint16 amy_summary_flags[8] = {
    SUMMARY_PLAYED | 96,
    FLAG_AMY_SS_TPARK_ELEVATOR,
    FLAG_AMY_HAMMER_WON,
    343,
    SUMMARY_PLAYED | 109,
    SUMMARY_NEVER,
    344,
    SUMMARY_NEVER,
};
static Uint16 big_summary_flags[8] = {
    SUMMARY_PLAYED | 208,
    SUMMARY_NEVER,
    FLAG_BIG_CLEAR_TWINKLE_PARK,
    472,
    FLAG_BIG_SS_HOTEL_POOL,
    469,
    470,
    SUMMARY_PLAYED | 220,
};
static Uint16 e102_summary_flags[13] = {
    SUMMARY_PLAYED | 176,
    415,
    416,
    FLAG_E102_CLEAR_BEACH,
    SUMMARY_PLAYED | 187,
    SUMMARY_PLAYED | 189,
    SUMMARY_PLAYED | 191,
    SUMMARY_PLAYED | 320,
    SUMMARY_PLAYED | 192,
    SUMMARY_PLAYED | 194,
    412,
    413,
    FLAG_E102_EC_HOT_SHELTER,
};
static Uint16 supersonic_summary_flags[4] = {
    SUMMARY_PLAYED | 244,
    SUMMARY_PLAYED | 245,
    SUMMARY_PLAYED | 247,
    SUMMARY_PLAYED | 248,
};

static enum summary_mode summary_mode;
static int summary_flags;
static int dat_8c907cd8;
static Float summary_fade;

static struct task *summary_text_create(struct summary_text_spec *spec);
static void summary_window_place(NJS_TEXTURE_VTX *window, Uint32 color,
                                 Float fade, Float x, Float y, Float z,
                                 Float width, Float height);
static void summary_music_step(struct task *task);

int summary_start(void *unused)
{
    int page;
    int result;

    page = summary_find_page(menu_get_story(get_character()));
    if (page == -1) {
        return OVERLAY_EMPTY;
    }
    stream_destroy();
    texture_release_characters();
    njReleaseTextureAll();
    heap_init();
    task_init();
    frame_pacing_init(2, 1);
    bitmap_clear_queue();
    njSetBorderColor(0xff000000);
    njSetBackColor(0xff000000, 0xff000000, 0xff000000);
    njFogDisable();
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    screen_quad_init();
    screen_cover_set_color(0);
    summary_fade = 1.0F;
    picture_create(1, menu_get_story(get_character()));
    stream_reset();
    /* the music task, as an int. the main loop tests the start's result
     * only against OVERLAY_EMPTY, which this never is */
    result = (int)task_create(0, 4, summary_music_step);
    summary_mode = SUMMARY_FADE_IN;
    summary_flags = 0;
    dat_8c907cd8 = 0;
    return result;
}

void summary_end(void *unused)
{
    draw_off = TRUE;
    bitmap_clear_queue();
    stream_destroy();
    task_destroy_all();
    task_step_all();
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    draw_off = FALSE;
}

int summary_step(void *unused)
{
    struct summary_text_spec spec;

    if (summary_mode == SUMMARY_DONE) {
        return OVERLAY_DONE;
    }
    pad_scan();
    pad_enable();
    task_step_all();
    task_draw_all();
    switch (summary_mode) {
    case SUMMARY_FADE_IN:
        screen_cover_show(TRUE);
        summary_fade -= FADE_STEP;
        if (summary_fade <= 0.0F) {
            summary_fade = 0.0F;
            summary_mode = SUMMARY_SCROLL;
            spec.level = 1;
            spec.start = NULL;
            spec.unk_08 = 0;
            spec.done = NULL;
            spec.story = menu_get_story(get_character());
            spec.unk_14 = 0;
            summary_text_create(&spec);
        }
        screen_cover_draw(summary_fade);
        break;
    case SUMMARY_SCROLL:
        screen_cover_show(FALSE);
        if ((summary_flags & SUMMARY_TEXT_DESTROYED) ||
            (per_info[0]->press & (PDD_DGT_ST | PDD_DGT_TA))) {
            summary_mode = SUMMARY_FADE_OUT;
            summary_flags &= ~SUMMARY_TEXT_DESTROYED;
            summary_fade = 0.0F;
        } else if (per_info[0]->press & PDD_DGT_TB) {
            /* a press of B does nothing, though the step tests for it */
        }
        break;
    case SUMMARY_FADE_OUT:
        screen_cover_show(TRUE);
        summary_fade += FADE_STEP;
        if (summary_fade >= 1.0F) {
            summary_fade = 1.0F;
            summary_mode = SUMMARY_FADED;
        }
        screen_cover_draw(summary_fade);
        break;
    case SUMMARY_FADED:
        screen_cover_show(TRUE);
        summary_fade = 1.0F;
        screen_cover_draw(summary_fade);
        summary_end(NULL);
        if (summary_flags & SUMMARY_LEAVE) {
            return OVERLAY_BACK;
        } else {
            summary_mode = SUMMARY_DONE;
            return OVERLAY_DONE;
        }
    case SUMMARY_DONE:
        break;
    }
    if (pad_quit_pressed(0)) {
        summary_end(NULL);
        return OVERLAY_RESET;
    }
    return OVERLAY_RUNNING;
}

/* scroll the page up by its speed each step, and kill the task once the
 * whole page is above the bitmap's top. the task has no draw: its step
 * prints the page into the bitmap and draws the bitmap's texture through
 * the window */
static void summary_text_step(struct task *task)
{
    struct summary_text *text;
    struct task_work *work;
    struct summary_page *page;
    struct bitmap *bitmap;
    char **lines;
    int script;
    int i;

    text = task->extra;
    work = task->work;
    script = summary_story_scripts[text->story];
    page = summary_scripts[script].pages;
    text->page = summary_find_page(text->story);
    page += text->page;
    bitmap = &text->bitmap;
    /* @hack redundant: a store through text and work that never runs. it
     * keeps text in R12, work in R13 and page in R11 (0x8c900354, 0x8c900356,
     * 0x8c900370); without it, they take R11, R12 and R13, and 30 of
     * summary_text_step's 158 words differ */
    if (0) {
        work->mode = text->page = text->story;
    }
    switch (work->mode) {
    case TEXT_START:
        /* the page starts below the bitmap's bottom edge */
        work->pos.y = VIEW_HEIGHT;
        bitmap->cursor_y = work->pos.y;
        work->mode = TEXT_SCROLL;
        break;
    case TEXT_SCROLL:
        bitmap_clear(bitmap);
        bitmap->text_color.color = 0xffffffff;
        work->pos.y -= page->speed;
        if (work->pos.y <
            -(page->count * (Float)(bitmap->line_spacing + GLYPH_SIZE))) {
            work->mode = TEXT_DONE;
            break;
        }
        bitmap->cursor_y = work->pos.y;
        lines = page->lines;
        for (i = 0; i < page->count; i++, lines++) {
            bitmap_print(bitmap, *lines);
            bitmap_newline(bitmap);
        }
        bitmap_queue_add(bitmap);
        njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
        njTextureClampMode(NJD_TEXTURECLAMP_CLAMP_UV);
        if (bitmap->texlist.textures && bitmap->texlist.nbTexture > 0) {
            njSetTexture(&bitmap->texlist);
            njDrawTexture(text->window, 8, bitmap->index, TRUE);
        }
        break;
    case TEXT_DONE:
        task_kill(task);
        break;
    }
}

static void summary_text_destroy(struct task *task)
{
    struct summary_text *text;

    text = task->extra;
    if (text) {
        if (text->bitmap.pixels) {
            bitmap_free(&text->bitmap);
        }
        if (text->done) {
            text->done();
        }
        heap_free(text);
        task->extra = NULL;
    }
    summary_flags |= SUMMARY_TEXT_DESTROYED;
}

/* start the text task for spec's story in spec's level, or with spec null
 * for story 0 in level 2, and return it. when task_create fails, it writes
 * through null (the @bug below) and returns null. the record's allocation
 * goes unchecked */
static struct task *summary_text_create(struct summary_text_spec *spec)
{
    struct task *task;
    struct summary_text *text;
    struct bitmap *bitmap;

    if (spec) {
        task = task_create(TASK_WORK, spec->level, summary_text_step);
        if (spec->start) {
            spec->start();
        }
    } else {
        task = task_create(TASK_WORK, 2, summary_text_step);
    }
    if (task) {
        text = task->extra = heap_calloc(1, sizeof(struct summary_text));
        if (spec) {
            text->story = spec->story;
            text->unk_100 = spec->unk_14;
            text->unk_10c = spec->unk_08;
            text->done = spec->done;
        }
        bitmap = &text->bitmap;
        bitmap_alloc(bitmap, VIEW_X, VIEW_Y, VIEW_WIDTH, VIEW_HEIGHT,
                     0xffffff00);
        bitmap->letter_spacing = 0;
        bitmap->line_spacing = 16;
        summary_window_place(text->window, 0xffffffff, 64.0F, bitmap->x,
                             bitmap->y, 20.0F, bitmap->texture_width,
                             bitmap->texture_height);
    }
    /* @bug memory: the store is outside the test of task, so a failed
     * task_create has it write the destructor through null. the BT at
     * 0x8c90058e skips the test's body to 0x8c900608, which loads
     * summary_text_destroy's address. the MOV.L R2,@(24,R13) at 0x8c90060c
     * then stores it through R13, the null task */
    task->destructor = summary_text_destroy;
    return task;
}

/* set window's eight corners for a width by height texture at x and y, at
 * depth z. the corners run in four rows down the texture, each row a right
 * corner then a left. the top and bottom rows take color with alpha 0. the
 * second row lies fade dots below the top, and the third fade dots above the
 * bottom. both take color as it is, so the texture fades to clear across its
 * first and last fade dots */
static void summary_window_place(NJS_TEXTURE_VTX *window, Uint32 color,
                                 Float fade, Float x, Float y, Float z,
                                 Float width, Float height)
{
    Float rows[2];
    Float vs[2];
    Float oow;

    oow = 1.0F / z;
    rows[0] = fade + y;
    rows[1] = y - fade + height;
    vs[0] = fade / height;
    vs[1] = 1.0F - vs[0];
    window[0].x = x + width;
    window[0].y = y;
    window[0].z = oow;
    window[0].u = 1.0F;
    window[0].v = 0.0F;
    window[0].col = color & 0xffffff;
    window[1].x = x;
    window[1].y = y;
    window[1].z = oow;
    window[1].u = 0.0F;
    window[1].v = 0.0F;
    window[1].col = color & 0xffffff;
    window[2].x = x + width;
    window[2].y = rows[0];
    window[2].z = oow;
    window[2].u = 1.0F;
    window[2].v = vs[0];
    window[2].col = color;
    window[3].x = x;
    window[3].y = rows[0];
    window[3].z = oow;
    window[3].u = 0.0F;
    window[3].v = vs[0];
    window[3].col = color;
    window[4].x = x + width;
    window[4].y = rows[1];
    window[4].z = oow;
    window[4].u = 1.0F;
    window[4].v = vs[1];
    window[4].col = color;
    window[5].x = x;
    window[5].y = rows[1];
    window[5].z = oow;
    window[5].u = 0.0F;
    window[5].v = vs[1];
    window[5].col = color;
    window[6].x = x + width;
    window[6].y = y + height;
    window[6].z = oow;
    window[6].u = 1.0F;
    window[6].v = 1.0F;
    window[6].col = color & 0xffffff;
    window[7].x = x;
    window[7].y = y + height;
    window[7].z = oow;
    window[7].u = 0.0F;
    window[7].v = 1.0F;
    window[7].col = color & 0xffffff;
}

/* request the music of the story picked on the menus, then kill the task,
 * which runs one step */
static void summary_music_step(struct task *task)
{
    int story;

    story = menu_get_story(get_character());
    /* @bug logic: menu_get_story returns menu_state.story, a Uint8, so 0
     * to 255 (EXTU.B at 0x8c05b60c). the CMP/EQ #-1 at 0x8c9007b0 never
     * holds, and the test always passes */
    if (story != -1) {
        stream_request_music(summary_tunes[story]);
    }
    task_kill(task);
}

/* @hack type: summary_text_step indexes summary_scripts through
 * summary.h's declaration, which has no bound, so the definition with its
 * bound comes after the step. with the bound in scope, SHC sign-extends the
 * script's offset as a byte, an EXTS.B after the SHLL2 at 0x8c90036c, and
 * 60 of summary_text_step's 158 words differ */
struct summary_script summary_scripts[7] = {
    {sonic_summary_pages, sonic_summary_flags, 21},
    {tails_summary_pages, tails_summary_flags, 14},
    {knuckles_summary_pages, knuckles_summary_flags, 13},
    {amy_summary_pages, amy_summary_flags, 8},
    {big_summary_pages, big_summary_flags, 8},
    {e102_summary_pages, e102_summary_flags, 13},
    {supersonic_summary_pages, supersonic_summary_flags, 4},
};
