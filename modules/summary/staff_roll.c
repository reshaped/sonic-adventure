/* the staff roll. the credits scroll up the screen while the story's
 * stills show behind them, one at a time. then the story's last picture
 * fades in and out */
#include <ninja.h>
#include <sg_xpt.h>

#include "advertise/font.h"
#include "draw/screen.h"
#include "main.h"
#include "object/common.h"
#include "player/player.h"
#include "progress/emblem.h"
#include "save/card.h"
#include "save/save.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "summary/picture.h"
#include "summary/staff_roll.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/texture.h"
#include "system/timer.h"

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
#define TEXT_49_LENGTH 7
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
#define TEXT_96_LENGTH 7
#define TEXT_97_LENGTH 7
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
#define TEXT_121_LENGTH 7
#define TEXT_122_LENGTH 7
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
#define TEXT_151_LENGTH 8
#define TEXT_152_LENGTH 8
#define TEXT_153_LENGTH 8
#define TEXT_154_LENGTH 8
#define TEXT_155_LENGTH 8
#define TEXT_156_LENGTH 8
#define TEXT_157_LENGTH 8
#define TEXT_158_LENGTH 8
#define TEXT_159_LENGTH 8
#define TEXT_160_LENGTH 8
#define TEXT_161_LENGTH 8
#define TEXT_162_LENGTH 8
#define TEXT_163_LENGTH 8
#define TEXT_164_LENGTH 8
#define TEXT_165_LENGTH 8
#define TEXT_166_LENGTH 8
#define TEXT_167_LENGTH 8
#define TEXT_168_LENGTH 8
#define TEXT_169_LENGTH 8
#define TEXT_170_LENGTH 8
#define TEXT_171_LENGTH 8
#define TEXT_172_LENGTH 8
#define TEXT_173_LENGTH 8
#define TEXT_174_LENGTH 8
#define TEXT_175_LENGTH 8
#define TEXT_176_LENGTH 8
#define TEXT_177_LENGTH 8
#define TEXT_178_LENGTH 8
#define TEXT_179_LENGTH 8
#define TEXT_180_LENGTH 8
#define TEXT_181_LENGTH 8
#define TEXT_182_LENGTH 8
#define TEXT_183_LENGTH 8
#define TEXT_184_LENGTH 8
#define TEXT_185_LENGTH 8
#define TEXT_186_LENGTH 8
#define TEXT_187_LENGTH 8
#define TEXT_188_LENGTH 8
#define TEXT_189_LENGTH 8
#define TEXT_190_LENGTH 8
#define TEXT_191_LENGTH 8
#define TEXT_192_LENGTH 8
#define TEXT_193_LENGTH 8
#define TEXT_194_LENGTH 8
#define TEXT_195_LENGTH 8
#define TEXT_196_LENGTH 8
#define TEXT_197_LENGTH 8
#define TEXT_198_LENGTH 8
#define TEXT_199_LENGTH 8
#define TEXT_200_LENGTH 8
#define TEXT_201_LENGTH 8
#define TEXT_202_LENGTH 8
#define TEXT_203_LENGTH 8
#define TEXT_204_LENGTH 8
#define TEXT_205_LENGTH 8
#define TEXT_206_LENGTH 8
#define TEXT_207_LENGTH 8
#define TEXT_208_LENGTH 8
#define TEXT_209_LENGTH 8
#define TEXT_210_LENGTH 8
#define TEXT_211_LENGTH 8
#define TEXT_212_LENGTH 8
#define TEXT_213_LENGTH 8
#define TEXT_214_LENGTH 8
#define TEXT_215_LENGTH 8
#define TEXT_216_LENGTH 8
#define TEXT_217_LENGTH 8
#define TEXT_218_LENGTH 8
#define TEXT_219_LENGTH 8
#define TEXT_220_LENGTH 8
#define TEXT_221_LENGTH 8
#define TEXT_222_LENGTH 8
#define TEXT_223_LENGTH 8
#define TEXT_224_LENGTH 8
#define TEXT_225_LENGTH 8
#define TEXT_226_LENGTH 9
#define TEXT_227_LENGTH 9
#define TEXT_228_LENGTH 9
#define TEXT_229_LENGTH 9
#define TEXT_230_LENGTH 9
#define TEXT_231_LENGTH 9
#define TEXT_232_LENGTH 9
#define TEXT_233_LENGTH 9
#define TEXT_234_LENGTH 8
#define TEXT_235_LENGTH 12
#define TEXT_236_LENGTH 9
#define TEXT_237_LENGTH 11
#define TEXT_238_LENGTH 21
#define TEXT_239_LENGTH 19
#define TEXT_240_LENGTH 14
#define TEXT_241_LENGTH 19
#define TEXT_242_LENGTH 18
#define TEXT_243_LENGTH 12
#define TEXT_244_LENGTH 11
#define TEXT_245_LENGTH 4
#define TEXT_246_LENGTH 8
#define TEXT_247_LENGTH 9
#define TEXT_248_LENGTH 8
#define TEXT_249_LENGTH 14
#define TEXT_250_LENGTH 12
#define TEXT_251_LENGTH 16
#define TEXT_252_LENGTH 15
#define TEXT_253_LENGTH 12
#define TEXT_254_LENGTH 14
#define TEXT_255_LENGTH 10
#define TEXT_256_LENGTH 18
#define TEXT_257_LENGTH 11
#define TEXT_258_LENGTH 24
#define TEXT_259_LENGTH 13
#define TEXT_260_LENGTH 12
#define TEXT_261_LENGTH 28
#define TEXT_262_LENGTH 17
#define TEXT_263_LENGTH 12
#define TEXT_264_LENGTH 18
#define TEXT_265_LENGTH 16
#define TEXT_266_LENGTH 26
#define TEXT_267_LENGTH 14
#define TEXT_268_LENGTH 11
#define TEXT_269_LENGTH 15
#define TEXT_270_LENGTH 13
#define TEXT_271_LENGTH 15
#define TEXT_272_LENGTH 13
#define TEXT_273_LENGTH 22
#define TEXT_274_LENGTH 17
#define TEXT_275_LENGTH 16
#define TEXT_276_LENGTH 14
#define TEXT_277_LENGTH 17
#define TEXT_278_LENGTH 18
#define TEXT_279_LENGTH 18
#define TEXT_280_LENGTH 14
#define TEXT_281_LENGTH 16
#define TEXT_282_LENGTH 11
#define TEXT_283_LENGTH 14
#define TEXT_284_LENGTH 18
#define TEXT_285_LENGTH 14
#define TEXT_286_LENGTH 14
#define TEXT_287_LENGTH 18
#define TEXT_288_LENGTH 16
#define TEXT_289_LENGTH 13
#define TEXT_290_LENGTH 17
#define TEXT_291_LENGTH 13
#define TEXT_292_LENGTH 12
#define TEXT_293_LENGTH 12
#define TEXT_294_LENGTH 13
#define TEXT_295_LENGTH 12
#define TEXT_296_LENGTH 16
#define TEXT_297_LENGTH 8
#define TEXT_298_LENGTH 14
#define TEXT_299_LENGTH 17
#define TEXT_300_LENGTH 15
#define TEXT_301_LENGTH 10
#define TEXT_302_LENGTH 14
#define TEXT_303_LENGTH 17
#define TEXT_304_LENGTH 17
#define TEXT_305_LENGTH 23
#define TEXT_306_LENGTH 16
#define TEXT_307_LENGTH 21
#define TEXT_308_LENGTH 16
#define TEXT_309_LENGTH 16
#define TEXT_310_LENGTH 17
#define TEXT_311_LENGTH 15
#define TEXT_312_LENGTH 19
#define TEXT_313_LENGTH 15
#define TEXT_314_LENGTH 15
#define TEXT_315_LENGTH 14
#define TEXT_316_LENGTH 15
#define TEXT_317_LENGTH 10
#define TEXT_318_LENGTH 11
#define TEXT_319_LENGTH 22
#define TEXT_320_LENGTH 12
#define TEXT_321_LENGTH 11
#define TEXT_322_LENGTH 22
#define TEXT_323_LENGTH 13
#define TEXT_324_LENGTH 18
#define TEXT_325_LENGTH 26
#define TEXT_326_LENGTH 20
#define TEXT_327_LENGTH 12
#define TEXT_328_LENGTH 16
#define TEXT_329_LENGTH 16
#define TEXT_330_LENGTH 19
#define TEXT_331_LENGTH 25
#define TEXT_332_LENGTH 14
#define TEXT_333_LENGTH 21
#define TEXT_334_LENGTH 16
#define TEXT_335_LENGTH 15
#define TEXT_336_LENGTH 10
#define TEXT_337_LENGTH 17
#define TEXT_338_LENGTH 17
#define TEXT_339_LENGTH 16
#define TEXT_340_LENGTH 14
#define TEXT_341_LENGTH 12
#define TEXT_342_LENGTH 16
#define TEXT_343_LENGTH 12
#define TEXT_344_LENGTH 10
#define TEXT_345_LENGTH 19
#define TEXT_346_LENGTH 23
#define TEXT_347_LENGTH 24
#define TEXT_348_LENGTH 15
#define TEXT_349_LENGTH 18
#define TEXT_350_LENGTH 7
#define TEXT_351_LENGTH 17
#define TEXT_352_LENGTH 22
#define TEXT_353_LENGTH 12
#define TEXT_354_LENGTH 20
#define TEXT_355_LENGTH 15
#define TEXT_356_LENGTH 14
#define TEXT_357_LENGTH 27
#define TEXT_358_LENGTH 24
#define TEXT_359_LENGTH 13
#define TEXT_360_LENGTH 17
#define TEXT_361_LENGTH 20
#define TEXT_362_LENGTH 17
#define TEXT_363_LENGTH 23
#define TEXT_364_LENGTH 13
#define TEXT_365_LENGTH 15
#define TEXT_366_LENGTH 19
#define TEXT_367_LENGTH 13
#define TEXT_368_LENGTH 16
#define TEXT_369_LENGTH 13
#define TEXT_370_LENGTH 25
#define TEXT_371_LENGTH 23
#define TEXT_372_LENGTH 13
#define TEXT_373_LENGTH 15
#define TEXT_374_LENGTH 22
#define TEXT_375_LENGTH 16
#define TEXT_376_LENGTH 24
#define TEXT_377_LENGTH 12
#define TEXT_378_LENGTH 21
#define TEXT_379_LENGTH 10
#define TEXT_380_LENGTH 13
#define TEXT_381_LENGTH 19
#define TEXT_382_LENGTH 16
#define TEXT_383_LENGTH 12
#define TEXT_384_LENGTH 13
#define TEXT_385_LENGTH 19
#define TEXT_386_LENGTH 14
#define TEXT_387_LENGTH 14
#define TEXT_388_LENGTH 17
#define TEXT_389_LENGTH 17
#define TEXT_390_LENGTH 15
#define TEXT_391_LENGTH 20
#define TEXT_392_LENGTH 18
#define TEXT_393_LENGTH 16
#define TEXT_394_LENGTH 15
#define TEXT_395_LENGTH 14
#define TEXT_396_LENGTH 17
#define TEXT_397_LENGTH 15
#define TEXT_398_LENGTH 10
#define TEXT_399_LENGTH 12
#define TEXT_400_LENGTH 11
#define TEXT_401_LENGTH 17
#define TEXT_402_LENGTH 14
#define TEXT_403_LENGTH 20
#define TEXT_404_LENGTH 9
#define TEXT_405_LENGTH 27
#define TEXT_406_LENGTH 11
#define TEXT_407_LENGTH 24
#define TEXT_408_LENGTH 12
#define TEXT_409_LENGTH 19
#define TEXT_410_LENGTH 15
#define TEXT_411_LENGTH 24
#define TEXT_412_LENGTH 19
#define TEXT_413_LENGTH 14
#define TEXT_414_LENGTH 20
#define TEXT_415_LENGTH 28
#define TEXT_416_LENGTH 10
#define TEXT_417_LENGTH 22
#define TEXT_418_LENGTH 15
#define TEXT_419_LENGTH 15
#define TEXT_420_LENGTH 12
#define TEXT_421_LENGTH 11
#define TEXT_422_LENGTH 27
#define TEXT_423_LENGTH 15
#define TEXT_424_LENGTH 19
#define TEXT_425_LENGTH 15
#define TEXT_426_LENGTH 13
#define TEXT_427_LENGTH 14
#define TEXT_428_LENGTH 13
#define TEXT_429_LENGTH 16
#define TEXT_430_LENGTH 15
#define TEXT_431_LENGTH 13
#define TEXT_432_LENGTH 24
#define TEXT_433_LENGTH 17
#define TEXT_434_LENGTH 42
#define TEXT_435_LENGTH 17
#define TEXT_436_LENGTH 30
#define TEXT_437_LENGTH 34
#define TEXT_438_LENGTH 28
#define TEXT_439_LENGTH 34
#define TEXT_440_LENGTH 35
#define TEXT_441_LENGTH 24
#define TEXT_442_LENGTH 27
#define TEXT_443_LENGTH 19
#define TEXT_444_LENGTH 19
#define TEXT_445_LENGTH 13
#define TEXT_446_LENGTH 15
#define TEXT_447_LENGTH 14
#define TEXT_448_LENGTH 31
#define TEXT_449_LENGTH 38
#define TEXT_450_LENGTH 37
#define TEXT_451_LENGTH 22
#define TEXT_452_LENGTH 43
#define TEXT_453_LENGTH 39
#define TEXT_454_LENGTH 25
#define TEXT_455_LENGTH 16
#define TEXT_456_LENGTH 33
#define TEXT_457_LENGTH 32
#define TEXT_458_LENGTH 19
#define TEXT_459_LENGTH 31
#define TEXT_460_LENGTH 33
#define TEXT_461_LENGTH 9
#define TEXT_462_LENGTH 6
#define TEXT_463_LENGTH 13
#define TEXT_464_LENGTH 12
#define TEXT_465_LENGTH 9
#define TEXT_466_LENGTH 15
#define TEXT_467_LENGTH 9
#define TEXT_468_LENGTH 15
#define TEXT_469_LENGTH 11
#define TEXT_470_LENGTH 11
#define TEXT_471_LENGTH 13
#define TEXT_472_LENGTH 7
#define TEXT_473_LENGTH 13
#define TEXT_474_LENGTH 9
#define TEXT_475_LENGTH 13
#define TEXT_476_LENGTH 4
#define TEXT_477_LENGTH 13
#define TEXT_478_LENGTH 8
#define TEXT_479_LENGTH 14
#define TEXT_480_LENGTH 8
#define TEXT_481_LENGTH 5
#define TEXT_482_LENGTH 10
#define TEXT_483_LENGTH 13
#define TEXT_484_LENGTH 9
#define TEXT_485_LENGTH 15
#define TEXT_486_LENGTH 10
#define TEXT_487_LENGTH 15
#define TEXT_488_LENGTH 12
#define TEXT_489_LENGTH 14
#define TEXT_490_LENGTH 21
#define TEXT_491_LENGTH 10
#define TEXT_492_LENGTH 23
#define TEXT_493_LENGTH 15
#define TEXT_494_LENGTH 18
#define TEXT_495_LENGTH 15
#define TEXT_496_LENGTH 5
#define TEXT_497_LENGTH 15
#define TEXT_498_LENGTH 16
#define TEXT_499_LENGTH 14
#define TEXT_500_LENGTH 17
#define TEXT_501_LENGTH 12
#define TEXT_502_LENGTH 12
#define TEXT_503_LENGTH 12
#define TEXT_504_LENGTH 9
#define TEXT_505_LENGTH 10
#define TEXT_506_LENGTH 10
#define TEXT_507_LENGTH 13
#define TEXT_508_LENGTH 22
#define TEXT_509_LENGTH 14
#define TEXT_510_LENGTH 15
#define TEXT_511_LENGTH 22
#define TEXT_512_LENGTH 12
#define TEXT_513_LENGTH 9
#define TEXT_514_LENGTH 13
#define TEXT_515_LENGTH 20
#define TEXT_516_LENGTH 15
#define TEXT_517_LENGTH 15
#define TEXT_518_LENGTH 11
#define TEXT_519_LENGTH 14
#define TEXT_520_LENGTH 12
#define TEXT_521_LENGTH 22
#define TEXT_522_LENGTH 14
#define TEXT_523_LENGTH 14
#define TEXT_524_LENGTH 13
#define TEXT_525_LENGTH 15
#define TEXT_526_LENGTH 14
#define TEXT_527_LENGTH 13
#define TEXT_528_LENGTH 18
#define TEXT_529_LENGTH 13
#define TEXT_530_LENGTH 20
#define TEXT_531_LENGTH 13
#define TEXT_532_LENGTH 12
#define TEXT_533_LENGTH 15
#define TEXT_534_LENGTH 14
#define TEXT_535_LENGTH 19
#define TEXT_536_LENGTH 10
#define TEXT_537_LENGTH 14
#define TEXT_538_LENGTH 16
#define TEXT_539_LENGTH 16
#define TEXT_540_LENGTH 14
#define TEXT_541_LENGTH 15
#define TEXT_542_LENGTH 16
#define TEXT_543_LENGTH 15
#define TEXT_544_LENGTH 12
#define TEXT_545_LENGTH 13
#define TEXT_546_LENGTH 15
#define TEXT_547_LENGTH 13
#define TEXT_548_LENGTH 12
#define TEXT_549_LENGTH 14
#define TEXT_550_LENGTH 11
#define TEXT_551_LENGTH 9
#define TEXT_552_LENGTH 7
#define TEXT_553_LENGTH 23
#define TEXT_554_LENGTH 18
#define TEXT_555_LENGTH 22
#define TEXT_556_LENGTH 13
#define TEXT_557_LENGTH 16
#define TEXT_558_LENGTH 15
#define TEXT_559_LENGTH 10
#define TEXT_560_LENGTH 14
#define TEXT_561_LENGTH 14
#define TEXT_562_LENGTH 15
#define TEXT_563_LENGTH 16
#define TEXT_564_LENGTH 10
#define TEXT_565_LENGTH 14
#define TEXT_566_LENGTH 13
#define TEXT_567_LENGTH 12
#define TEXT_568_LENGTH 15
#define TEXT_569_LENGTH 24
#define TEXT_570_LENGTH 23
#define TEXT_571_LENGTH 12
#define TEXT_572_LENGTH 13
#define TEXT_573_LENGTH 13
#define TEXT_574_LENGTH 17
#define TEXT_575_LENGTH 16
#define TEXT_576_LENGTH 17
#define TEXT_577_LENGTH 15
#define TEXT_578_LENGTH 18
#define TEXT_579_LENGTH 12
#define TEXT_580_LENGTH 11
#define TEXT_581_LENGTH 11
#define TEXT_582_LENGTH 15
#define TEXT_583_LENGTH 16
#define TEXT_584_LENGTH 16
#define TEXT_585_LENGTH 15
#define TEXT_586_LENGTH 15
#define TEXT_587_LENGTH 15
#define TEXT_588_LENGTH 14
#define TEXT_589_LENGTH 13
#define TEXT_590_LENGTH 14
#define TEXT_591_LENGTH 13
#define TEXT_592_LENGTH 16
#define TEXT_593_LENGTH 27
#define TEXT_594_LENGTH 28
#define TEXT_595_LENGTH 14
#define TEXT_596_LENGTH 12
#define TEXT_597_LENGTH 20
#define TEXT_598_LENGTH 10
#define TEXT_599_LENGTH 9
#define TEXT_600_LENGTH 8
#define TEXT_601_LENGTH 24
#define TEXT_602_LENGTH 15
#define TEXT_603_LENGTH 12
#define TEXT_604_LENGTH 21
#include "summary/staff_roll/text.inc"

/* the stories, as menu_get_story numbers them: the six characters', then
 * Super Sonic's */
#define STORIES 7

/* the steps of the waits in ROLL_EMBLEM, ROLL_EMBLEM_COUNT and ROLL_SAVE */
#define EMBLEM_DELAY 10
#define EMBLEM_COUNT_STEPS 300
#define SAVE_STEPS 120

/* the last picture's steps: each fade's, and the hold's between them */
#define LAST_FADE_STEPS 120.0F
#define LAST_HOLD_STEPS 300.0F

/* the logos draw at 1.5 times their textures' size */
#define LOGO_SCALE 1.5F

/* staff_roll_step's modes, each with the mode it moves on to */
enum staff_roll_mode {
    ROLL_START, /* the first step, then ROLL_CREDITS */
    ROLL_CREDITS, /* until the credits end, then ROLL_LAST */
    ROLL_LAST, /* until the last picture fades out, then ROLL_EMBLEM */
    ROLL_END, /* ends the roll, then ROLL_DONE */
    ROLL_DONE, /* once the roll has ended */
    /* waits EMBLEM_DELAY steps. if the save holds the story's emblem
     * already, the roll then moves to ROLL_END. otherwise it sets the emblem
     * in the save, starts the emblem count and moves to ROLL_EMBLEM_COUNT */
    ROLL_EMBLEM,
    /* EMBLEM_COUNT_STEPS of the emblem count, then ROLL_SAVE */
    ROLL_EMBLEM_COUNT,
    ROLL_SAVE /* SAVE_STEPS while the saves write, then ROLL_END */
};

/* a credits line's kind, which says how the line shows. credits_line_height
 * gives the pixels each kind takes down the roll to the next line */
enum {
    LINE_BREAK, /* a section's start, which shows nothing */
    LINE_TITLE, /* centered and underlined */
    LINE_ROLE, /* at the margin, underlined */
    LINE_NAME, /* indented */
    LINE_PLAIN, /* at the margin */
    LINE_LOGO, /* staff_roll_logo_texlist's texture parameter, centered */
    LINE_CENTERED,
    LINE_CLOSING, /* centered */
    LINE_BLANK, /* shows nothing */
    LINE_GAP, /* shows nothing */
    /* shows nothing. the stills' share of the roll ends past it */
    LINE_STILLS_END
};

/* the credits task's modes */
enum { CREDITS_START, CREDITS_SCROLL, CREDITS_CLEAR, CREDITS_DONE };

/* the stills task's modes. after the last still, the task waits in
 * STILLS_WAIT until stills_show_last starts the last picture */
enum {
    STILLS_START,
    STILLS_FADE_IN,
    STILLS_HOLD,
    STILLS_FADE_OUT,
    STILLS_NEXT,
    STILLS_WAIT,
    LAST_FADE_IN,
    LAST_HOLD,
    LAST_FADE_OUT,
    STILLS_DONE
};

struct staff_line {
    Uint8 kind;
    /* a logo's texture, or a gap's pixels. a line of another kind leaves it
     * unread */
    Uint8 parameter;
    char *text;
};

struct staff_script {
    struct staff_line *lines;
    int count;
};

/* a story's stills and its last picture. the stills are the textures of
 * count pvms, shown one at a time. the last picture's eight textures cover
 * the screen as last_parts lays them out */
struct staff_stills {
    char **files;
    NJS_TEXLIST **texlists;
    int count;
    char *last_file;
    NJS_TEXLIST *last_texlist;
    struct picture_parts *last_parts;
};

/* the credits task's extra. a line's place is how far down the roll it
 * lies, in pixels */
struct credits {
    Uint8 mode;
    Uint8 done; /* 1 once every line has started and scrolled off */
    Uint8 story;
    int next_line; /* the index in script of the next line to start */
    int step; /* the steps the task has run. staff_roll.c never reads it */
    int steps; /* the steps the roll's height takes to scroll */
    int next_y; /* the next line's place */
    Float margin; /* the x of the lines at the margin */
    Float scroll; /* the pixels the roll has scrolled up */
    Uint8 unk_1c[8];
    Float speed; /* the pixels scroll gains a step */
    Uint8 unk_28[4];
    struct staff_script *script;
    Uint8 unk_30[4];
    NJS_TEXLIST *logo_texlist;
};

/* a credits line task's extra */
struct credit_line {
    Uint8 unk_00[4];
    struct task *credits_task;
    struct staff_line *line;
    Float x; /* where the line starts, or a logo's middle */
    Float y; /* minus the line's place */
    Float z; /* the line's depth */
    NJS_TEXLIST *logo_texlist;
};

/* the stills task's extra. step counts each still's steps from 0, and start,
 * full, fade_out and duration are marks on that count. the last picture
 * fades in and out over fade_steps each */
struct stills {
    Uint8 mode;
    Uint8 done; /* 1 once the last picture has faded out */
    Uint8 unk_02[2];
    /* the stills ended so far. staff_roll.c never reads it */
    Uint16 shown;
    /* stills_fade_out's value. staff_roll.c never reads it */
    Uint16 unk_06;
    Uint16 texture; /* the still, an index in texlist */
    Uint8 unk_0a[2];
    NJS_POINT3 pos; /* of which stills_draw reads only z, the depth */
    NJS_TEXLIST *texlist; /* the texlist of the still's pvm */
    int pvm; /* texlist's index in stills->texlists */
    Float step;
    Float hold_steps;
    Float fade_steps;
    Float start; /* the step a still's fade in starts at */
    Float full; /* the step at which its fade in ends and its hold starts */
    Float fade_out; /* the step its fade out starts at */
    Float duration; /* the step its fade out ends at */
    /* colors[0] is the white the still and the last picture draw in, at
     * the fade's alpha. stills_create sets colors[1] white, and staff_roll.c
     * never reads it */
    NJS_ARGB colors[2];
    struct staff_stills *stills;
};

static NJS_TEXNAME sonic_stills_0_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
static NJS_TEXLIST sonic_stills_0_texlist = {
#include "summary/staff_roll/sonic_stills_0_texlist.inc"
};
static NJS_TEXNAME sonic_stills_1_textures[10] = {
    {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
    {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0},
    {TEXT_18, 0, 0}, {TEXT_19, 0, 0},
};
static NJS_TEXLIST sonic_stills_1_texlist = {
#include "summary/staff_roll/sonic_stills_1_texlist.inc"
};
static NJS_TEXNAME sonic_stills_2_textures[8] = {
    {TEXT_20, 0, 0}, {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
    {TEXT_24, 0, 0}, {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0},
};
static NJS_TEXLIST sonic_stills_2_texlist = {
#include "summary/staff_roll/sonic_stills_2_texlist.inc"
};
static NJS_TEXNAME tails_stills_0_textures[9] = {
    {TEXT_28, 0, 0}, {TEXT_29, 0, 0}, {TEXT_30, 0, 0},
    {TEXT_31, 0, 0}, {TEXT_32, 0, 0}, {TEXT_33, 0, 0},
    {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
};
static NJS_TEXLIST tails_stills_0_texlist = {
#include "summary/staff_roll/tails_stills_0_texlist.inc"
};
static NJS_TEXNAME tails_stills_1_textures[10] = {
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0},
};
static NJS_TEXLIST tails_stills_1_texlist = {
#include "summary/staff_roll/tails_stills_1_texlist.inc"
};
static NJS_TEXNAME tails_stills_2_textures[4] = {
    {TEXT_47, 0, 0},
    {TEXT_48, 0, 0},
    {TEXT_49, 0, 0},
    {TEXT_50, 0, 0},
};
static NJS_TEXLIST tails_stills_2_texlist = {
#include "summary/staff_roll/tails_stills_2_texlist.inc"
};
static NJS_TEXNAME knuckles_stills_0_textures[9] = {
    {TEXT_51, 0, 0}, {TEXT_52, 0, 0}, {TEXT_53, 0, 0},
    {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
    {TEXT_57, 0, 0}, {TEXT_58, 0, 0}, {TEXT_59, 0, 0},
};
static NJS_TEXLIST knuckles_stills_0_texlist = {
#include "summary/staff_roll/knuckles_stills_0_texlist.inc"
};
static NJS_TEXNAME knuckles_stills_1_textures[10] = {
    {TEXT_60, 0, 0}, {TEXT_61, 0, 0}, {TEXT_62, 0, 0}, {TEXT_63, 0, 0},
    {TEXT_64, 0, 0}, {TEXT_65, 0, 0}, {TEXT_66, 0, 0}, {TEXT_67, 0, 0},
    {TEXT_68, 0, 0}, {TEXT_69, 0, 0},
};
static NJS_TEXLIST knuckles_stills_1_texlist = {
#include "summary/staff_roll/knuckles_stills_1_texlist.inc"
};
static NJS_TEXNAME knuckles_stills_2_textures[8] = {
    {TEXT_70, 0, 0}, {TEXT_71, 0, 0}, {TEXT_72, 0, 0}, {TEXT_73, 0, 0},
    {TEXT_74, 0, 0}, {TEXT_75, 0, 0}, {TEXT_76, 0, 0}, {TEXT_77, 0, 0},
};
static NJS_TEXLIST knuckles_stills_2_texlist = {
#include "summary/staff_roll/knuckles_stills_2_texlist.inc"
};
static NJS_TEXNAME amy_stills_0_textures[9] = {
    {TEXT_78, 0, 0}, {TEXT_79, 0, 0}, {TEXT_80, 0, 0},
    {TEXT_81, 0, 0}, {TEXT_82, 0, 0}, {TEXT_83, 0, 0},
    {TEXT_84, 0, 0}, {TEXT_85, 0, 0}, {TEXT_86, 0, 0},
};
static NJS_TEXLIST amy_stills_0_texlist = {
#include "summary/staff_roll/amy_stills_0_texlist.inc"
};
static NJS_TEXNAME amy_stills_1_textures[9] = {
    {TEXT_87, 0, 0}, {TEXT_88, 0, 0}, {TEXT_89, 0, 0},
    {TEXT_90, 0, 0}, {TEXT_91, 0, 0}, {TEXT_92, 0, 0},
    {TEXT_93, 0, 0}, {TEXT_94, 0, 0}, {TEXT_95, 0, 0},
};
static NJS_TEXLIST amy_stills_1_texlist = {
#include "summary/staff_roll/amy_stills_1_texlist.inc"
};
static NJS_TEXNAME amy_stills_2_textures[5] = {
    {TEXT_96, 0, 0}, {TEXT_97, 0, 0},  {TEXT_98, 0, 0},
    {TEXT_99, 0, 0}, {TEXT_100, 0, 0},
};
static NJS_TEXLIST amy_stills_2_texlist = {
#include "summary/staff_roll/amy_stills_2_texlist.inc"
};
static NJS_TEXNAME big_stills_0_textures[9] = {
    {TEXT_101, 0, 0}, {TEXT_102, 0, 0}, {TEXT_103, 0, 0},
    {TEXT_104, 0, 0}, {TEXT_105, 0, 0}, {TEXT_106, 0, 0},
    {TEXT_107, 0, 0}, {TEXT_108, 0, 0}, {TEXT_109, 0, 0},
};
static NJS_TEXLIST big_stills_0_texlist = {
#include "summary/staff_roll/big_stills_0_texlist.inc"
};
static NJS_TEXNAME big_stills_1_textures[10] = {
    {TEXT_110, 0, 0}, {TEXT_111, 0, 0}, {TEXT_112, 0, 0}, {TEXT_113, 0, 0},
    {TEXT_114, 0, 0}, {TEXT_115, 0, 0}, {TEXT_116, 0, 0}, {TEXT_117, 0, 0},
    {TEXT_118, 0, 0}, {TEXT_119, 0, 0},
};
static NJS_TEXLIST big_stills_1_texlist = {
#include "summary/staff_roll/big_stills_1_texlist.inc"
};
static NJS_TEXNAME big_stills_2_textures[5] = {
    {TEXT_120, 0, 0}, {TEXT_121, 0, 0}, {TEXT_122, 0, 0},
    {TEXT_123, 0, 0}, {TEXT_124, 0, 0},
};
static NJS_TEXLIST big_stills_2_texlist = {
#include "summary/staff_roll/big_stills_2_texlist.inc"
};
static NJS_TEXNAME e102_stills_0_textures[9] = {
    {TEXT_125, 0, 0}, {TEXT_126, 0, 0}, {TEXT_127, 0, 0},
    {TEXT_128, 0, 0}, {TEXT_129, 0, 0}, {TEXT_130, 0, 0},
    {TEXT_131, 0, 0}, {TEXT_132, 0, 0}, {TEXT_133, 0, 0},
};
static NJS_TEXLIST e102_stills_0_texlist = {
#include "summary/staff_roll/e102_stills_0_texlist.inc"
};
static NJS_TEXNAME e102_stills_1_textures[10] = {
    {TEXT_134, 0, 0}, {TEXT_135, 0, 0}, {TEXT_136, 0, 0}, {TEXT_137, 0, 0},
    {TEXT_138, 0, 0}, {TEXT_139, 0, 0}, {TEXT_140, 0, 0}, {TEXT_141, 0, 0},
    {TEXT_142, 0, 0}, {TEXT_143, 0, 0},
};
static NJS_TEXLIST e102_stills_1_texlist = {
#include "summary/staff_roll/e102_stills_1_texlist.inc"
};
static NJS_TEXNAME e102_stills_2_textures[7] = {
    {TEXT_144, 0, 0}, {TEXT_145, 0, 0}, {TEXT_146, 0, 0}, {TEXT_147, 0, 0},
    {TEXT_148, 0, 0}, {TEXT_149, 0, 0}, {TEXT_150, 0, 0},
};
static NJS_TEXLIST e102_stills_2_texlist = {
#include "summary/staff_roll/e102_stills_2_texlist.inc"
};
static NJS_TEXNAME supersonic_stills_0_textures[8] = {
    {TEXT_151, 0, 0}, {TEXT_152, 0, 0}, {TEXT_153, 0, 0}, {TEXT_154, 0, 0},
    {TEXT_155, 0, 0}, {TEXT_156, 0, 0}, {TEXT_157, 0, 0}, {TEXT_158, 0, 0},
};
static NJS_TEXLIST supersonic_stills_0_texlist = {
#include "summary/staff_roll/supersonic_stills_0_texlist.inc"
};
static NJS_TEXNAME supersonic_stills_1_textures[6] = {
    {TEXT_159, 0, 0}, {TEXT_160, 0, 0}, {TEXT_161, 0, 0},
    {TEXT_162, 0, 0}, {TEXT_163, 0, 0}, {TEXT_164, 0, 0},
};
static NJS_TEXLIST supersonic_stills_1_texlist = {
#include "summary/staff_roll/supersonic_stills_1_texlist.inc"
};
static NJS_TEXNAME supersonic_stills_2_textures[10] = {
    {TEXT_165, 0, 0}, {TEXT_166, 0, 0}, {TEXT_167, 0, 0}, {TEXT_168, 0, 0},
    {TEXT_169, 0, 0}, {TEXT_170, 0, 0}, {TEXT_171, 0, 0}, {TEXT_172, 0, 0},
    {TEXT_173, 0, 0}, {TEXT_174, 0, 0},
};
static NJS_TEXLIST supersonic_stills_2_texlist = {
#include "summary/staff_roll/supersonic_stills_2_texlist.inc"
};
static NJS_TEXNAME supersonic_stills_3_textures[3] = {
    {TEXT_175, 0, 0},
    {TEXT_176, 0, 0},
    {TEXT_177, 0, 0},
};
static NJS_TEXLIST supersonic_stills_3_texlist = {
#include "summary/staff_roll/supersonic_stills_3_texlist.inc"
};
static NJS_TEXNAME sonic_last_textures[8] = {
    {TEXT_178, 0, 0}, {TEXT_179, 0, 0}, {TEXT_180, 0, 0}, {TEXT_181, 0, 0},
    {TEXT_182, 0, 0}, {TEXT_183, 0, 0}, {TEXT_184, 0, 0}, {TEXT_185, 0, 0},
};
static NJS_TEXLIST sonic_last_texlist = {
#include "summary/staff_roll/sonic_last_texlist.inc"
};
static NJS_TEXNAME tails_last_textures[8] = {
    {TEXT_186, 0, 0}, {TEXT_187, 0, 0}, {TEXT_188, 0, 0}, {TEXT_189, 0, 0},
    {TEXT_190, 0, 0}, {TEXT_191, 0, 0}, {TEXT_192, 0, 0}, {TEXT_193, 0, 0},
};
static NJS_TEXLIST tails_last_texlist = {
#include "summary/staff_roll/tails_last_texlist.inc"
};
static NJS_TEXNAME knuckles_last_textures[8] = {
    {TEXT_194, 0, 0}, {TEXT_195, 0, 0}, {TEXT_196, 0, 0}, {TEXT_197, 0, 0},
    {TEXT_198, 0, 0}, {TEXT_199, 0, 0}, {TEXT_200, 0, 0}, {TEXT_201, 0, 0},
};
static NJS_TEXLIST knuckles_last_texlist = {
#include "summary/staff_roll/knuckles_last_texlist.inc"
};
static NJS_TEXNAME amy_last_textures[8] = {
    {TEXT_202, 0, 0}, {TEXT_203, 0, 0}, {TEXT_204, 0, 0}, {TEXT_205, 0, 0},
    {TEXT_206, 0, 0}, {TEXT_207, 0, 0}, {TEXT_208, 0, 0}, {TEXT_209, 0, 0},
};
static NJS_TEXLIST amy_last_texlist = {
#include "summary/staff_roll/amy_last_texlist.inc"
};
static NJS_TEXNAME big_last_textures[8] = {
    {TEXT_210, 0, 0}, {TEXT_211, 0, 0}, {TEXT_212, 0, 0}, {TEXT_213, 0, 0},
    {TEXT_214, 0, 0}, {TEXT_215, 0, 0}, {TEXT_216, 0, 0}, {TEXT_217, 0, 0},
};
static NJS_TEXLIST big_last_texlist = {
#include "summary/staff_roll/big_last_texlist.inc"
};
static NJS_TEXNAME e102_last_textures[8] = {
    {TEXT_218, 0, 0}, {TEXT_219, 0, 0}, {TEXT_220, 0, 0}, {TEXT_221, 0, 0},
    {TEXT_222, 0, 0}, {TEXT_223, 0, 0}, {TEXT_224, 0, 0}, {TEXT_225, 0, 0},
};
static NJS_TEXLIST e102_last_texlist = {
#include "summary/staff_roll/e102_last_texlist.inc"
};
static NJS_TEXNAME supersonic_last_textures[8] = {
    {TEXT_226, 0, 0}, {TEXT_227, 0, 0}, {TEXT_228, 0, 0}, {TEXT_229, 0, 0},
    {TEXT_230, 0, 0}, {TEXT_231, 0, 0}, {TEXT_232, 0, 0}, {TEXT_233, 0, 0},
};
static NJS_TEXLIST supersonic_last_texlist = {
#include "summary/staff_roll/supersonic_last_texlist.inc"
};
/* endbg_logo's textures */
static NJS_TEXNAME staff_roll_logo_textures[4] = {
    {TEXT_234, 0, 0},
    {TEXT_235, 0, 0},
    {TEXT_236, 0, 0},
    {TEXT_237, 0, 0},
};
static NJS_TEXLIST staff_roll_logo_texlist = {
#include "summary/staff_roll/staff_roll_logo_texlist.inc"
};
/* clang-format off */
static struct staff_line staff_roll_lines[414] = {
    {LINE_TITLE, 0, TEXT_238},
    {LINE_BLANK, 255, ""},
    {LINE_ROLE, 255, TEXT_239},
    {LINE_NAME, 255, TEXT_240},
    {LINE_NAME, 255, TEXT_241},
    {LINE_ROLE, 255, TEXT_242},
    {LINE_NAME, 255, TEXT_243},
    {LINE_NAME, 255, TEXT_244},
    {LINE_BREAK, 0, TEXT_245},
    {LINE_ROLE, 255, TEXT_246},
    {LINE_NAME, 255, TEXT_247},
    {LINE_ROLE, 255, TEXT_248},
    {LINE_NAME, 255, TEXT_249},
    {LINE_ROLE, 255, TEXT_250},
    {LINE_NAME, 255, TEXT_251},
    {LINE_ROLE, 255, TEXT_252},
    {LINE_NAME, 255, TEXT_253},
    {LINE_ROLE, 255, TEXT_254},
    {LINE_NAME, 255, TEXT_255},
    {LINE_BLANK, 255, ""},
    {LINE_ROLE, 255, TEXT_256},
    {LINE_NAME, 255, TEXT_257},
    {LINE_ROLE, 255, TEXT_258},
    {LINE_NAME, 255, TEXT_249},
    {LINE_NAME, 255, TEXT_259},
    {LINE_NAME, 255, TEXT_260},
    {LINE_ROLE, 255, TEXT_261},
    {LINE_NAME, 255, TEXT_253},
    {LINE_NAME, 255, TEXT_262},
    {LINE_NAME, 255, TEXT_263},
    {LINE_NAME, 255, TEXT_264},
    {LINE_NAME, 255, TEXT_265},
    {LINE_ROLE, 255, TEXT_266},
    {LINE_NAME, 255, TEXT_257},
    {LINE_NAME, 255, TEXT_267},
    {LINE_BREAK, 1, TEXT_268},
    {LINE_ROLE, 255, TEXT_269},
    {LINE_NAME, 255, TEXT_259},
    {LINE_NAME, 255, TEXT_270},
    {LINE_NAME, 255, TEXT_260},
    {LINE_NAME, 255, TEXT_271},
    {LINE_NAME, 255, TEXT_272},
    {LINE_ROLE, 255, TEXT_273},
    {LINE_NAME, 255, TEXT_274},
    {LINE_NAME, 255, TEXT_275},
    {LINE_NAME, 255, TEXT_253},
    {LINE_NAME, 255, TEXT_276},
    {LINE_ROLE, 255, TEXT_277},
    {LINE_NAME, 255, TEXT_278},
    {LINE_NAME, 255, TEXT_279},
    {LINE_NAME, 255, TEXT_280},
    {LINE_NAME, 255, TEXT_281},
    {LINE_NAME, 255, TEXT_282},
    {LINE_NAME, 255, TEXT_283},
    {LINE_ROLE, 255, TEXT_284},
    {LINE_NAME, 255, TEXT_251},
    {LINE_NAME, 255, TEXT_285},
    {LINE_NAME, 255, TEXT_286},
    {LINE_NAME, 255, TEXT_287},
    {LINE_NAME, 255, TEXT_288},
    {LINE_NAME, 255, TEXT_289},
    {LINE_NAME, 255, TEXT_290},
    {LINE_ROLE, 255, TEXT_291},
    {LINE_NAME, 255, TEXT_292},
    {LINE_NAME, 255, TEXT_293},
    {LINE_NAME, 255, TEXT_294},
    {LINE_NAME, 255, TEXT_295},
    {LINE_NAME, 255, TEXT_296},
    {LINE_NAME, 255, TEXT_297},
    {LINE_NAME, 255, TEXT_298},
    {LINE_NAME, 255, TEXT_299},
    {LINE_NAME, 255, TEXT_300},
    {LINE_NAME, 255, TEXT_301},
    {LINE_NAME, 255, TEXT_302},
    {LINE_NAME, 255, TEXT_303},
    {LINE_NAME, 255, TEXT_304},
    {LINE_ROLE, 255, TEXT_305},
    {LINE_NAME, 255, TEXT_306},
    {LINE_ROLE, 255, TEXT_307},
    {LINE_NAME, 255, TEXT_308},
    {LINE_NAME, 255, TEXT_309},
    {LINE_ROLE, 255, TEXT_310},
    {LINE_NAME, 255, TEXT_311},
    {LINE_NAME, 255, TEXT_312},
    {LINE_NAME, 255, TEXT_313},
    {LINE_NAME, 255, TEXT_314},
    {LINE_NAME, 255, TEXT_315},
    {LINE_NAME, 255, TEXT_316},
    {LINE_NAME, 255, TEXT_317},
    {LINE_NAME, 255, TEXT_318},
    {LINE_ROLE, 255, TEXT_319},
    {LINE_NAME, 255, TEXT_320},
    {LINE_BREAK, 2, TEXT_321},
    {LINE_ROLE, 255, TEXT_322},
    {LINE_NAME, 255, TEXT_249},
    {LINE_NAME, 255, TEXT_323},
    {LINE_ROLE, 255, TEXT_324},
    {LINE_NAME, 255, TEXT_263},
    {LINE_ROLE, 255, TEXT_325},
    {LINE_NAME, 255, TEXT_257},
    {LINE_ROLE, 255, TEXT_326},
    {LINE_NAME, 255, TEXT_259},
    {LINE_NAME, 255, TEXT_327},
    {LINE_NAME, 255, TEXT_328},
    {LINE_ROLE, 255, TEXT_329},
    {LINE_NAME, 255, TEXT_330},
    {LINE_ROLE, 255, TEXT_331},
    {LINE_NAME, 255, TEXT_251},
    {LINE_NAME, 255, TEXT_332},
    {LINE_NAME, 255, TEXT_289},
    {LINE_ROLE, 255, TEXT_333},
    {LINE_NAME, 255, TEXT_334},
    {LINE_NAME, 255, TEXT_335},
    {LINE_NAME, 255, TEXT_336},
    {LINE_NAME, 255, TEXT_337},
    {LINE_ROLE, 255, TEXT_338},
    {LINE_NAME, 255, TEXT_339},
    {LINE_NAME, 255, TEXT_340},
    {LINE_NAME, 255, TEXT_341},
    {LINE_NAME, 255, TEXT_342},
    {LINE_NAME, 255, TEXT_343},
    {LINE_NAME, 255, TEXT_344},
    {LINE_BREAK, 3, TEXT_345},
    {LINE_ROLE, 255, TEXT_346},
    {LINE_NAME, 255, TEXT_270},
    {LINE_NAME, 255, TEXT_327},
    {LINE_ROLE, 255, TEXT_347},
    {LINE_NAME, 255, TEXT_330},
    {LINE_ROLE, 255, TEXT_348},
    {LINE_NAME, 255, TEXT_264},
    {LINE_ROLE, 255, TEXT_349},
    {LINE_NAME, 255, TEXT_332},
    {LINE_ROLE, 255, TEXT_350},
    {LINE_NAME, 255, TEXT_306},
    {LINE_NAME, 255, TEXT_351},
    {LINE_ROLE, 255, TEXT_352},
    {LINE_NAME, 255, TEXT_353},
    {LINE_ROLE, 255, TEXT_354},
    {LINE_NAME, 255, TEXT_355},
    {LINE_NAME, 255, TEXT_356},
    {LINE_ROLE, 255, TEXT_357},
    {LINE_NAME, 255, TEXT_270},
    {LINE_ROLE, 255, TEXT_358},
    {LINE_NAME, 255, TEXT_359},
    {LINE_BREAK, 4, TEXT_360},
    {LINE_ROLE, 255, TEXT_361},
    {LINE_NAME, 255, TEXT_362},
    {LINE_ROLE, 255, TEXT_363},
    {LINE_NAME, 255, TEXT_364},
    {LINE_ROLE, 255, TEXT_365},
    {LINE_NAME, 255, TEXT_362},
    {LINE_ROLE, 255, TEXT_366},
    {LINE_NAME, 255, TEXT_367},
    {LINE_NAME, 255, TEXT_368},
    {LINE_NAME, 255, TEXT_369},
    {LINE_ROLE, 255, TEXT_370},
    {LINE_NAME, 255, TEXT_279},
    {LINE_ROLE, 255, TEXT_371},
    {LINE_NAME, 255, TEXT_372},
    {LINE_NAME, 255, TEXT_373},
    {LINE_ROLE, 255, TEXT_374},
    {LINE_NAME, 255, TEXT_364},
    {LINE_NAME, 255, TEXT_375},
    {LINE_ROLE, 255, TEXT_376},
    {LINE_NAME, 255, TEXT_377},
    {LINE_ROLE, 255, TEXT_378},
    {LINE_NAME, 255, TEXT_379},
    {LINE_NAME, 255, TEXT_380},
    {LINE_ROLE, 255, TEXT_354},
    {LINE_NAME, 255, TEXT_381},
    {LINE_NAME, 255, TEXT_382},
    {LINE_NAME, 255, TEXT_383},
    {LINE_NAME, 255, TEXT_384},
    {LINE_NAME, 255, TEXT_385},
    {LINE_NAME, 255, TEXT_386},
    {LINE_BREAK, 5, TEXT_387},
    {LINE_ROLE, 255, TEXT_388},
    {LINE_NAME, 255, TEXT_364},
    {LINE_ROLE, 255, TEXT_389},
    {LINE_NAME, 255, TEXT_390},
    {LINE_ROLE, 255, TEXT_391},
    {LINE_NAME, 255, TEXT_392},
    {LINE_ROLE, 255, TEXT_393},
    {LINE_NAME, 255, TEXT_394},
    {LINE_NAME, 255, TEXT_395},
    {LINE_NAME, 255, TEXT_396},
    {LINE_NAME, 255, TEXT_397},
    {LINE_NAME, 255, TEXT_398},
    {LINE_NAME, 255, TEXT_399},
    {LINE_NAME, 255, TEXT_400},
    {LINE_NAME, 255, TEXT_401},
    {LINE_NAME, 255, TEXT_402},
    {LINE_ROLE, 255, TEXT_403},
    {LINE_NAME, 255, TEXT_364},
    {LINE_ROLE, 255, TEXT_404},
    {LINE_NAME, 255, TEXT_405},
    {LINE_ROLE, 255, TEXT_406},
    {LINE_NAME, 255, TEXT_407},
    {LINE_BREAK, 6, TEXT_408},
    {LINE_ROLE, 255, TEXT_409},
    {LINE_NAME, 255, TEXT_410},
    {LINE_ROLE, 255, TEXT_411},
    {LINE_NAME, 255, TEXT_278},
    {LINE_ROLE, 255, TEXT_412},
    {LINE_NAME, 255, TEXT_413},
    {LINE_ROLE, 255, TEXT_414},
    {LINE_NAME, 255, TEXT_275},
    {LINE_ROLE, 255, TEXT_415},
    {LINE_NAME, 255, TEXT_262},
    {LINE_NAME, 255, TEXT_416},
    {LINE_NAME, 255, TEXT_265},
    {LINE_ROLE, 255, TEXT_417},
    {LINE_NAME, 255, TEXT_418},
    {LINE_NAME, 255, TEXT_419},
    {LINE_NAME, 255, TEXT_420},
    {LINE_BREAK, 7, TEXT_421},
    {LINE_ROLE, 255, TEXT_422},
    {LINE_NAME, 255, TEXT_423},
    {LINE_ROLE, 255, TEXT_424},
    {LINE_NAME, 255, TEXT_255},
    {LINE_ROLE, 255, TEXT_425},
    {LINE_NAME, 255, TEXT_426},
    {LINE_NAME, 255, TEXT_427},
    {LINE_ROLE, 255, TEXT_428},
    {LINE_NAME, 255, TEXT_429},
    {LINE_NAME, 255, TEXT_430},
    {LINE_NAME, 255, TEXT_431},
    {LINE_NAME, 255, TEXT_432},
    {LINE_ROLE, 255, TEXT_433},
    {LINE_NAME, 255, TEXT_255},
    {LINE_NAME, 255, TEXT_434},
    {LINE_ROLE, 255, TEXT_435},
    {LINE_NAME, 255, TEXT_436},
    {LINE_NAME, 255, TEXT_437},
    {LINE_NAME, 255, TEXT_438},
    {LINE_NAME, 255, TEXT_439},
    {LINE_NAME, 255, TEXT_440},
    {LINE_NAME, 255, TEXT_441},
    {LINE_NAME, 255, TEXT_442},
    {LINE_NAME, 255, TEXT_443},
    {LINE_ROLE, 255, TEXT_444},
    {LINE_NAME, 255, TEXT_445},
    {LINE_NAME, 255, TEXT_446},
    {LINE_NAME, 255, TEXT_447},
    {LINE_NAME, 255, TEXT_448},
    {LINE_NAME, 255, TEXT_449},
    {LINE_NAME, 255, TEXT_450},
    {LINE_ROLE, 255, TEXT_451},
    {LINE_NAME, 255, TEXT_452},
    {LINE_NAME, 255, TEXT_453},
    {LINE_NAME, 255, TEXT_454},
    {LINE_ROLE, 255, TEXT_455},
    {LINE_NAME, 255, TEXT_456},
    {LINE_NAME, 255, TEXT_457},
    {LINE_ROLE, 255, TEXT_458},
    {LINE_NAME, 255, TEXT_459},
    {LINE_NAME, 255, TEXT_460},
    {LINE_BREAK, 8, TEXT_461},
    {LINE_ROLE, 255, TEXT_462},
    {LINE_NAME, 255, TEXT_463},
    {LINE_NAME, 255, TEXT_464},
    {LINE_NAME, 255, TEXT_465},
    {LINE_NAME, 255, TEXT_466},
    {LINE_NAME, 255, TEXT_467},
    {LINE_NAME, 255, TEXT_468},
    {LINE_NAME, 255, TEXT_469},
    {LINE_NAME, 255, TEXT_470},
    {LINE_NAME, 255, TEXT_471},
    {LINE_ROLE, 255, TEXT_472},
    {LINE_NAME, 255, TEXT_255},
    {LINE_NAME, 255, TEXT_473},
    {LINE_NAME, 255, TEXT_474},
    {LINE_NAME, 255, TEXT_475},
    {LINE_NAME, 255, TEXT_426},
    {LINE_ROLE, 255, TEXT_476},
    {LINE_NAME, 255, TEXT_477},
    {LINE_NAME, 255, TEXT_478},
    {LINE_NAME, 255, TEXT_479},
    {LINE_NAME, 255, TEXT_480},
    {LINE_NAME, 255, TEXT_255},
    {LINE_NAME, 255, TEXT_426},
    {LINE_ROLE, 255, TEXT_481},
    {LINE_NAME, 255, TEXT_482},
    {LINE_NAME, 255, TEXT_483},
    {LINE_ROLE, 255, TEXT_484},
    {LINE_NAME, 255, TEXT_485},
    {LINE_NAME, 255, TEXT_431},
    {LINE_NAME, 255, TEXT_255},
    {LINE_ROLE, 255, TEXT_486},
    {LINE_NAME, 255, TEXT_487},
    {LINE_ROLE, 255, TEXT_488},
    {LINE_NAME, 255, TEXT_489},
    {LINE_ROLE, 255, TEXT_490},
    {LINE_NAME, 255, TEXT_491},
    {LINE_ROLE, 255, TEXT_492},
    {LINE_NAME, 255, TEXT_493},
    {LINE_ROLE, 255, TEXT_494},
    {LINE_NAME, 255, TEXT_495},
    {LINE_BREAK, 9, TEXT_496},
    {LINE_ROLE, 255, TEXT_497},
    {LINE_NAME, 255, TEXT_498},
    {LINE_NAME, 255, TEXT_499},
    {LINE_NAME, 255, TEXT_500},
    {LINE_NAME, 255, TEXT_501},
    {LINE_NAME, 255, TEXT_502},
    {LINE_NAME, 255, TEXT_503},
    {LINE_NAME, 255, TEXT_504},
    {LINE_NAME, 255, TEXT_505},
    {LINE_NAME, 255, TEXT_506},
    {LINE_NAME, 255, TEXT_507},
    {LINE_ROLE, 255, TEXT_508},
    {LINE_NAME, 255, TEXT_509},
    {LINE_NAME, 255, TEXT_510},
    {LINE_ROLE, 255, TEXT_511},
    {LINE_NAME, 255, TEXT_512},
    {LINE_NAME, 255, TEXT_510},
    {LINE_ROLE, 255, TEXT_513},
    {LINE_NAME, 255, TEXT_514},
    {LINE_BLANK, 255, ""},
    {LINE_ROLE, 255, TEXT_515},
    {LINE_NAME, 255, TEXT_516},
    {LINE_NAME, 255, TEXT_517},
    {LINE_NAME, 255, TEXT_518},
    {LINE_NAME, 255, TEXT_519},
    {LINE_NAME, 255, TEXT_520},
    {LINE_ROLE, 255, TEXT_521},
    {LINE_NAME, 255, TEXT_522},
    {LINE_NAME, 255, TEXT_523},
    {LINE_NAME, 255, TEXT_524},
    {LINE_NAME, 255, TEXT_525},
    {LINE_NAME, 255, TEXT_526},
    {LINE_NAME, 255, TEXT_527},
    {LINE_ROLE, 255, TEXT_528},
    {LINE_NAME, 255, TEXT_529},
    {LINE_ROLE, 255, TEXT_530},
    {LINE_NAME, 255, TEXT_531},
    {LINE_NAME, 255, TEXT_532},
    {LINE_NAME, 255, TEXT_533},
    {LINE_NAME, 255, TEXT_534},
    {LINE_NAME, 255, TEXT_535},
    {LINE_NAME, 255, TEXT_337},
    {LINE_NAME, 255, TEXT_536},
    {LINE_NAME, 255, TEXT_537},
    {LINE_ROLE, 255, TEXT_538},
    {LINE_NAME, 255, TEXT_539},
    {LINE_NAME, 255, TEXT_540},
    {LINE_NAME, 255, TEXT_541},
    {LINE_ROLE, 255, TEXT_542},
    {LINE_NAME, 255, TEXT_543},
    {LINE_NAME, 255, TEXT_544},
    {LINE_NAME, 255, TEXT_545},
    {LINE_NAME, 255, TEXT_546},
    {LINE_NAME, 255, TEXT_547},
    {LINE_NAME, 255, TEXT_548},
    {LINE_NAME, 255, TEXT_549},
    {LINE_NAME, 255, TEXT_550},
    {LINE_NAME, 255, TEXT_551},
    {LINE_NAME, 255, TEXT_552},
    {LINE_NAME, 255, TEXT_553},
    {LINE_NAME, 255, TEXT_554},
    {LINE_NAME, 255, TEXT_555},
    {LINE_NAME, 255, TEXT_556},
    {LINE_ROLE, 255, TEXT_557},
    {LINE_NAME, 255, TEXT_558},
    {LINE_NAME, 255, TEXT_559},
    {LINE_NAME, 255, TEXT_560},
    {LINE_NAME, 255, TEXT_561},
    {LINE_NAME, 255, TEXT_562},
    {LINE_NAME, 255, TEXT_563},
    {LINE_NAME, 255, TEXT_564},
    {LINE_NAME, 255, TEXT_565},
    {LINE_NAME, 255, TEXT_566},
    {LINE_NAME, 255, TEXT_567},
    {LINE_NAME, 255, TEXT_568},
    {LINE_NAME, 255, TEXT_552},
    {LINE_NAME, 255, TEXT_569},
    {LINE_ROLE, 255, TEXT_570},
    {LINE_NAME, 255, TEXT_571},
    {LINE_NAME, 255, TEXT_572},
    {LINE_NAME, 255, TEXT_573},
    {LINE_ROLE, 255, TEXT_574},
    {LINE_NAME, 255, TEXT_575},
    {LINE_NAME, 255, TEXT_576},
    {LINE_NAME, 255, TEXT_577},
    {LINE_ROLE, 255, TEXT_578},
    {LINE_NAME, 255, TEXT_579},
    {LINE_NAME, 255, TEXT_580},
    {LINE_NAME, 255, TEXT_581},
    {LINE_NAME, 255, TEXT_582},
    {LINE_ROLE, 255, TEXT_583},
    {LINE_NAME, 255, TEXT_584},
    {LINE_NAME, 255, TEXT_585},
    {LINE_NAME, 255, TEXT_586},
    {LINE_NAME, 255, TEXT_587},
    {LINE_ROLE, 255, TEXT_588},
    {LINE_NAME, 255, TEXT_589},
    {LINE_NAME, 255, TEXT_590},
    {LINE_NAME, 255, TEXT_591},
    {LINE_NAME, 255, TEXT_592},
    {LINE_NAME, 255, TEXT_593},
    {LINE_NAME, 255, TEXT_594},
    {LINE_NAME, 255, TEXT_595},
    {LINE_NAME, 255, TEXT_596},
    {LINE_ROLE, 255, TEXT_597},
    {LINE_NAME, 255, TEXT_598},
    {LINE_STILLS_END, 255, ""},
    {LINE_LOGO, 2, TEXT_599},
    {LINE_LOGO, 0, TEXT_600},
    {LINE_LOGO, 3, TEXT_601},
    {LINE_BLANK, 255, ""},
    {LINE_LOGO, 1, TEXT_602},
    {LINE_BLANK, 255, ""},
    {LINE_CENTERED, 255, TEXT_603},
    {LINE_CLOSING, 255, TEXT_604},
};
/* clang-format on */
static struct staff_script staff_roll_script = {staff_roll_lines, 414};
static char *sonic_stills_files[3] = {
    "endbg_sonic_0",
    "endbg_sonic_1",
    "endbg_sonic_2",
};
static NJS_TEXLIST *sonic_stills_texlists[3] = {
    &sonic_stills_0_texlist,
    &sonic_stills_1_texlist,
    &sonic_stills_2_texlist,
};
static struct picture_parts staff_roll_last_parts = {
    8,
    {{0, 0, 0},
     {1, 0, 256},
     {2, 256, 0},
     {3, 256, 256},
     {4, 512, 0},
     {5, 512, 128},
     {6, 512, 256},
     {7, 512, 384}},
};
static char *tails_stills_files[3] = {
    "endbg_tails_0",
    "endbg_tails_1",
    "endbg_tails_2",
};
static NJS_TEXLIST *tails_stills_texlists[3] = {
    &tails_stills_0_texlist,
    &tails_stills_1_texlist,
    &tails_stills_2_texlist,
};
static char *knuckles_stills_files[3] = {
    "endbg_knuckles_0",
    "endbg_knuckles_1",
    "endbg_knuckles_2",
};
static NJS_TEXLIST *knuckles_stills_texlists[3] = {
    &knuckles_stills_0_texlist,
    &knuckles_stills_1_texlist,
    &knuckles_stills_2_texlist,
};
static char *amy_stills_files[3] = {
    "endbg_amy_0",
    "endbg_amy_1",
    "endbg_amy_2",
};
static NJS_TEXLIST *amy_stills_texlists[3] = {
    &amy_stills_0_texlist,
    &amy_stills_1_texlist,
    &amy_stills_2_texlist,
};
static char *big_stills_files[3] = {
    "endbg_big_0",
    "endbg_big_1",
    "endbg_big_2",
};
static NJS_TEXLIST *big_stills_texlists[3] = {
    &big_stills_0_texlist,
    &big_stills_1_texlist,
    &big_stills_2_texlist,
};
static char *e102_stills_files[3] = {
    "endbg_e102_0",
    "endbg_e102_1",
    "endbg_e102_2",
};
static NJS_TEXLIST *e102_stills_texlists[3] = {
    &e102_stills_0_texlist,
    &e102_stills_1_texlist,
    &e102_stills_2_texlist,
};
static char *supersonic_stills_files[4] = {
    "endbg_supersonic_0",
    "endbg_supersonic_1",
    "endbg_supersonic_2",
    "endbg_supersonic_3",
};
static NJS_TEXLIST *supersonic_stills_texlists[4] = {
    &supersonic_stills_0_texlist,
    &supersonic_stills_1_texlist,
    &supersonic_stills_2_texlist,
    &supersonic_stills_3_texlist,
};
static struct staff_stills staff_roll_stills[STORIES] = {
    {sonic_stills_files, sonic_stills_texlists, 3, "endbg_last_sonic",
     &sonic_last_texlist, &staff_roll_last_parts},
    {tails_stills_files, tails_stills_texlists, 3, "endbg_last_tails",
     &tails_last_texlist, &staff_roll_last_parts},
    {knuckles_stills_files, knuckles_stills_texlists, 3, "endbg_last_knuckles",
     &knuckles_last_texlist, &staff_roll_last_parts},
    {amy_stills_files, amy_stills_texlists, 3, "endbg_last_amy",
     &amy_last_texlist, &staff_roll_last_parts},
    {big_stills_files, big_stills_texlists, 3, "endbg_last_big",
     &big_last_texlist, &staff_roll_last_parts},
    {e102_stills_files, e102_stills_texlists, 3, "endbg_last_e102",
     &e102_last_texlist, &staff_roll_last_parts},
    {supersonic_stills_files, supersonic_stills_texlists, 4,
     "endbg_last_supersonic", &supersonic_last_texlist, &staff_roll_last_parts},
};
/* the emblem count's task, or null before the roll starts one.
 * staff_roll_step releases the logo's textures as it starts the count */
static struct task *staff_roll_emblem_task = NULL;
/* each story's character, whose story emblem the roll sets. Super Sonic's
 * story takes CHARACTER_COUNT, for which emblem_story_number gives the
 * emblem after Big's */
static Uint8 staff_roll_characters[STORIES] = {
    CHARACTER_SONIC, CHARACTER_TAILS, CHARACTER_KNUCKLES, CHARACTER_AMY,
    CHARACTER_BIG,   CHARACTER_E102,  CHARACTER_COUNT,
};
static Uint8 staff_roll_tunes[STORIES] = {
    MUSIC_SONIC, MUSIC_TAILS, MUSIC_KNUCKLES, MUSIC_AMY,
    MUSIC_BIG,   MUSIC_E102,  MUSIC_MAINTHEM,
};
/* the steps each story's credits take to scroll the roll's height */
static int staff_roll_steps[STORIES] = {
    15300, 13200, 15300, 17400, 13860, 16020, 15840,
};

/* staff_roll_start sets dat_8c907d18, dat_8c907d1c, dat_8c907d24 and
 * dat_8c907d2c, and staff_roll.c never reads them */
static Float dat_8c907d18;
static Float dat_8c907d1c;
static enum staff_roll_mode staff_roll_mode;
static int dat_8c907d24;
static int staff_roll_timer;
static int dat_8c907d2c;
static struct task *staff_roll_credits_task;
static struct task *staff_roll_stills_task;

static void staff_roll_music_step(struct task *task);
static struct task *credits_create(struct staff_script *script, int story);
static struct task *credit_line_create(struct task *credits);
static Uint32 credits_line_height(struct staff_script *script,
                                  unsigned int index);
static Uint32 credits_height(struct staff_script *script);
static Uint32 credits_stills_height(struct staff_script *script);
static struct task *stills_create(int level, struct staff_stills *stills,
                                  int story, struct staff_script *script);
static void stills_show_last(struct task *task);
static int stills_count(struct staff_stills *stills);
static void stills_load(struct staff_stills *stills);
static void stills_release(struct staff_stills *stills);

int staff_roll_start(void *unused)
{
    int story;

    stream_destroy();
    texture_release_characters();
    njReleaseTextureAll();
    heap_init();
    task_init();
    frame_pacing_init(1, 1);
    bitmap_clear_queue();
    njSetBorderColor(0xff000000);
    njSetBackColor(0xff000000, 0xff000000, 0xff000000);
    njFogDisable();
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    screen_quad_init();
    screen_cover_set_color(0);
    dat_8c907d18 = 1.0F;
    dat_8c907d1c = 0.1F;
    story = menu_get_story(get_character());
    advertise_font_load(&staffroll_font);
    advertise_font_use(&staffroll_font);
    texture_load_pvm("endbg_logo", &staff_roll_logo_texlist);
    stills_load(&staff_roll_stills[story]);
    texture_load_pvm("con_regular", &con_regular_texlist);
    stream_reset();
    task_create(0, 4, staff_roll_music_step);
    staff_roll_mode = ROLL_START;
    dat_8c907d24 = 0;
    staff_roll_timer = 0;
    dat_8c907d2c = 0;
    staff_roll_emblem_task = NULL;
    staff_roll_credits_task = credits_create(&staff_roll_script, story);
    staff_roll_stills_task =
        stills_create(2, staff_roll_stills, story, &staff_roll_script);
    return 1;
}

void staff_roll_end(void *unused)
{
    int story;

    draw_off = TRUE;
    story = menu_get_story(get_character());
    bitmap_clear_queue();
    stream_destroy();
    if (staff_roll_credits_task) {
        task_kill(staff_roll_credits_task);
        staff_roll_credits_task = NULL;
    }
    task_destroy_all();
    task_step_all();
    stills_release(&staff_roll_stills[story]);
    if (!staff_roll_emblem_task) {
        njReleaseTexture(&staff_roll_logo_texlist);
    }
    advertise_font_release(&staffroll_font);
    njReleaseTextureAll();
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    draw_off = FALSE;
}

/* @hack spelling: credits_done as a function, which SHC inlines into
 * staff_roll_step with its result copied to R5 (MOV R0,R5 at 0x8c901dfc).
 * read in place there, 53 of staff_roll_step's 201 words differ */
static int credits_done(struct task *task)
{
    struct credits *credits = task->extra;

    return credits->done;
}

/* @hack spelling: stills_done as a function, which SHC inlines into
 * staff_roll_step with its result copied to R5 (MOV R0,R5 at 0x8c901e2c).
 * read in place there, 112 of staff_roll_step's 201 words differ */
static int stills_done(struct task *task)
{
    struct stills *stills = task->extra;

    return stills->done;
}

int staff_roll_step(void *unused)
{
    int result = OVERLAY_RUNNING;
    int character;
    struct task *task;

    pad_scan();
    pad_enable();
    task_step_all();
    task_draw_all();
    switch (staff_roll_mode) {
    case ROLL_START:
        staff_roll_mode = ROLL_CREDITS;
        break;
    case ROLL_CREDITS:
        task = staff_roll_credits_task;
        if (!task) {
            break;
        }
        if (credits_done(task) == 1) {
            task_kill(task);
            staff_roll_credits_task = NULL;
            staff_roll_mode = ROLL_LAST;
            stills_show_last(staff_roll_stills_task);
        }
        break;
    case ROLL_LAST:
        task = staff_roll_stills_task;
        if (!task) {
            break;
        }
        if (stills_done(task) == 1) {
            task_kill(task);
            staff_roll_stills_task = NULL;
            staff_roll_mode = ROLL_EMBLEM;
            staff_roll_timer = 0;
            stream_destroy();
        }
        break;
    case ROLL_END:
        result = OVERLAY_DONE;
        staff_roll_end(NULL);
        staff_roll_mode = ROLL_DONE;
        break;
    case ROLL_DONE:
        break;
    case ROLL_EMBLEM:
        if (++staff_roll_timer > EMBLEM_DELAY) {
            staff_roll_timer = 0;
            character = staff_roll_characters[menu_get_story(0)];
            if (!emblem_check_story(&saves[save_slot], character)) {
                emblem_set_story(&saves[save_slot], character);
                njReleaseTexture(&staff_roll_logo_texlist);
                staff_roll_emblem_task = emblem_start_count_display();
                staff_roll_mode = ROLL_EMBLEM_COUNT;
            } else {
                staff_roll_mode = ROLL_END;
            }
        }
        break;
    case ROLL_EMBLEM_COUNT:
        staff_roll_timer++;
        if (staff_roll_timer > EMBLEM_COUNT_STEPS) {
            staff_roll_timer = 0;
            staff_roll_mode = ROLL_SAVE;
            save_write();
        }
        break;
    case ROLL_SAVE:
        staff_roll_timer++;
        if (staff_roll_timer > SAVE_STEPS) {
            staff_roll_timer = 0;
            staff_roll_mode = ROLL_END;
        }
        break;
    default:
        break;
    }
    if (pad_quit_pressed(0)) {
        staff_roll_end(NULL);
        return OVERLAY_RESET;
    }
    return result;
}

/* request the story's music, and kill the task */
static void staff_roll_music_step(struct task *task)
{
    int story = menu_get_story(get_character());

    if (story != -1) {
        stream_request_music(staff_roll_tunes[story]);
    }
    task_kill(task);
}

/* scroll the roll, and start a task for each line as the scroll passes its
 * place, one line a step. once every line has started, scroll on until
 * their tasks end, then set done */
static void credits_step(struct task *task)
{
    struct credits *credits = task->extra;

    credits->done = 0;
    switch (credits->mode) {
    case CREDITS_START:
        credits->mode++;
        break;
    case CREDITS_SCROLL:
        if (credits->next_y < credits->scroll) {
            credit_line_create(task);
            credits->next_y +=
                credits_line_height(credits->script, credits->next_line);
            credits->next_line++;
        }
        credits->scroll += credits->speed;
        if (credits->next_line >= credits->script->count) {
            credits->mode = CREDITS_CLEAR;
        }
        break;
    case CREDITS_CLEAR:
        if (task->child) {
            credits->scroll += credits->speed;
        } else {
            credits->mode = CREDITS_DONE;
        }
        break;
    case CREDITS_DONE:
        credits->done = 1;
        break;
    default:
        break;
    }
    credits->step++;
    task_step_children(task);
}

/* @hack spelling: credits_steps as a function, which SHC inlines into
 * credits_create and stills_create with story copied as its argument (MOV
 * R5,R4 at 0x8c9020ec, MOV R4,R5 at 0x8c9029f4). read in place, 125 of
 * their 233 words differ */
static int credits_steps(int story)
{
    return staff_roll_steps[story];
}

/* create the credits task in level 1. it scrolls the roll's height, to
 * its last line, over the story's staff_roll_steps. return the task, or
 * null when task_create fails */
static struct task *credits_create(struct staff_script *script, int story)
{
    struct task *task = task_create(TASK_WORK, 1, credits_step);
    struct credits *credits;

    if (task) {
        credits = task->extra = heap_calloc(1, sizeof(struct credits));
        credits->script = script;
        credits->story = story;
        credits->steps = credits_steps(story);
        credits->next_y = 32;
        credits->speed =
            (Float)credits_height(credits->script) / credits->steps;
        credits->margin = 16.0F;
        credits->logo_texlist = &staff_roll_logo_texlist;
    }
    return task;
}

/* a line draws in its step, so its draw does nothing */
static void credit_line_draw(struct task *task)
{
}

/* draw the line where the scroll has brought it, and end its task once the
 * line has scrolled off the top. a break's task ends at once */
static void credit_line_step(struct task *task)
{
    struct credit_line *credit = task->extra;
    struct credits *credits = credit->credits_task->extra;
    Float y;
    Float depth;
    NJS_POLYGON_VTX poly[4];
    int text_width;
    /* @hack type: height signed, though screen_texture_size writes it as a
     * Uint32, so SHC converts it with FLOAT and FTRC (0x8c9022d6,
     * 0x8c9022da). a texture's height is below 2^31, where the two types
     * hold the same value. a Uint32 converts through __utof and __ftou,
     * which SUMMARY doesn't link; as a Uint32, count unmeasured */
    Sint32 height;

    if (credit->line->kind == LINE_BREAK) {
        task_kill(task);
        return;
    }
    y = -credit->y - credits->scroll + SCREEN_HEIGHT;
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    switch (credit->line->kind) {
    case LINE_TITLE:
    case LINE_ROLE:
    case LINE_NAME:
    case LINE_PLAIN:
    case LINE_CENTERED:
    case LINE_CLOSING:
        advertise_font_draw_string(credit->line->text, credit->x, y, credit->z);
        if (credit->line->kind == LINE_ROLE ||
            credit->line->kind == LINE_TITLE) {
            text_width = advertise_font_string_width(credit->line->text);
            depth = 1.0F / (credit->z + 10.0F);
            poly[0].x = credit->x;
            poly[0].y = y + 26.0F;
            poly[0].z = depth;
            poly[0].col = 0xffff0000;
            poly[1].x = credit->x;
            poly[1].y = poly[0].y + 2.0F;
            poly[1].z = depth;
            poly[1].col = 0xffff0000;
            poly[2].x = credit->x + text_width;
            poly[2].y = poly[0].y;
            poly[2].z = depth;
            poly[2].col = 0xffff0000;
            poly[3].x = poly[2].x;
            poly[3].y = poly[1].y;
            poly[3].z = depth;
            poly[3].col = 0xffff0000;
            njDrawPolygon(poly, 4, 0);
        }
        height = 24;
        break;
    case LINE_LOGO:
        njSetTexture(credit->logo_texlist);
        screen_quad_color(0xffffffff);
        screen_quad_draw_scaled(credit->line->parameter | QUAD_TOP, credit->x,
                                y, credit->z, LOGO_SCALE, LOGO_SCALE);
        screen_texture_size(credit->logo_texlist, credit->line->parameter, NULL,
                            (Uint32 *)&height);
        height *= LOGO_SCALE;
        /* the last case takes no break. with one, the test below loads
         * height into R1, not R3 (0x8c9022e0) */
    }
    /* @bug uninitialized: a blank, a gap and the stills' end match no case
     * and set no height, so their test reads whatever the stack holds at
     * height's word (MOV.L @R15,R3 at 0x8c9022e0) */
    /* @hack spelling: height taken from 0.0F, not negated, which subtracts
     * it in floats (FLDI0 FR2 at 0x8c9022e2, FSUB FR3,FR2 at 0x8c9022e8).
     * as -height, SHC negates the integer, and 16 of 200 words differ */
    if (y < 0.0F - height) {
        task_kill(task);
    }
}

/* start a task for the credits' next line, as credits_task's child, and
 * place the line by its kind. it reads a logo's size and leaves it unused.
 * return the task, or null when task_create_child fails */
static struct task *credit_line_create(struct task *credits_task)
{
    struct task *task =
        task_create_child(TASK_WORK, credit_line_step, credits_task);
    struct credits *credits;
    struct credit_line *credit;
    Uint32 width;
    Uint32 height;
    int text_width;

    if (!task) {
        return NULL;
    }
    credits = credits_task->extra;
    task->draw = credit_line_draw;
    credit = task->extra = heap_calloc(1, sizeof(struct credit_line));
    credit->credits_task = credits_task;
    credit->line = &credits->script->lines[credits->next_line];
    credit->logo_texlist = credits->logo_texlist;
    switch (credit->line->kind) {
    case LINE_NAME:
        credit->x = credits->margin + 32.0F;
        break;
    case LINE_TITLE:
    case LINE_CENTERED:
    case LINE_CLOSING:
        text_width = advertise_font_string_width(credit->line->text);
        credit->x = (SCREEN_WIDTH - text_width) * 0.5F;
        break;
    case LINE_LOGO:
        screen_texture_size(credit->logo_texlist, credit->line->parameter,
                            &width, &height);
        credit->x = SCREEN_MIDDLE_X;
        break;
    default:
        credit->x = credits->margin;
        break;
    }
    credit->y = -(Float)credits->next_y;
    credit->z = 1000.0F;
    return task;
}

/* the pixels line index takes down the roll to the next line, by its kind.
 * before a break it takes 96, and before a role 32 more, unless it is a
 * blank. an index past the script takes 96 */
static Uint32 credits_line_height(struct staff_script *script,
                                  unsigned int index)
{
    Uint8 kind;
    Uint8 next;
    int height;

    if (index >= (unsigned int)script->count) {
        return 96;
    }
    kind = script->lines[index].kind;
    /* @bug memory: for the script's last line, next is read past
     * the script (MOV.B @R5,R3 at 0x8c902498). past staff_roll_lines lies
     * staff_roll_script, whose first byte, 0x8c, matches no case */
    next = script->lines[index + 1].kind;
    switch (kind) {
    case LINE_TITLE:
        height = 32;
        break;
    case LINE_BREAK:
        height = 64;
        break;
    case LINE_ROLE:
        height = 40;
        break;
    case LINE_NAME:
        height = 32;
        break;
    case LINE_LOGO:
        height = 256;
        break;
    case LINE_CENTERED:
        height = 32;
        break;
    case LINE_CLOSING:
        height = 480;
        break;
    case LINE_BLANK:
        height = 480;
        break;
    case LINE_GAP:
        height = script->lines[index].parameter;
        break;
    case LINE_STILLS_END:
        height = 480;
        break;
    default:
        height = 0;
        break;
    }
    switch (next) {
    case LINE_BREAK:
        height = 96;
        break;
    case LINE_ROLE:
        if (kind != LINE_BLANK) {
            height += 32;
        }
        break;
    /* @hack redundant: a case for LINE_NAME that only breaks, which keeps the
     * switch's CMP/EQ #3 at 0x8c902546; without it, 18 of 81 words differ */
    case LINE_NAME:
        break;
    }
    return height;
}

/* the roll's height: the pixels from its first line down to its last */
static Uint32 credits_height(struct staff_script *script)
{
    Uint32 height = 0;
    int i;

    for (i = 0; i < script->count - 1; i++) {
        height += credits_line_height(script, i);
    }
    return height;
}

/* the pixels from the roll's first line down past its LINE_STILLS_END, or
 * to its last line without one */
static Uint32 credits_stills_height(struct staff_script *script)
{
    Uint32 height = 0;
    int i;

    for (i = 0; i < script->count - 1; i++) {
        height += credits_line_height(script, i);
        if (script->lines[i].kind == LINE_STILLS_END) {
            break;
        }
    }
    return height;
}

static void stills_step(struct task *task)
{
    struct stills *stills = task->extra;
    struct staff_stills *own;

    stills->done = 0;
    switch (stills->mode) {
    case STILLS_START:
        stills->mode = STILLS_FADE_IN;
        stills->colors[0].a = 0.0F;
        /* fall through */
    case STILLS_FADE_IN:
        stills->colors[0].a =
            (stills->step - stills->start) / (stills->full - stills->start);
        if (stills->colors[0].a > 1.0F) {
            stills->colors[0].a = 1.0F;
        }
        stills->step += 1.0F;
        if (stills->step > stills->full) {
            stills->mode = STILLS_HOLD;
        }
        break;
    case STILLS_HOLD:
        stills->colors[0].a = 1.0F;
        stills->step += 1.0F;
        if (stills->step > stills->fade_out) {
            stills->mode = STILLS_FADE_OUT;
        }
        break;
    case STILLS_FADE_OUT:
        stills->colors[0].a = (stills->duration - stills->step) /
                              (stills->duration - stills->fade_out);
        if (stills->colors[0].a < 0.0F) {
            stills->colors[0].a = 0.0F;
        }
        stills->step += 1.0F;
        if (stills->step > stills->duration) {
            stills->mode = STILLS_NEXT;
            stills->step = 0.0F;
        }
        break;
    case STILLS_NEXT:
        own = stills->stills;
        stills->shown++;
        stills->texture++;
        stills->mode = STILLS_FADE_IN;
        if (stills->texture >= stills->texlist->nbTexture) {
            stills->texture = 0;
            stills->pvm++;
            if (stills->pvm >= own->count) {
                stills->mode = STILLS_WAIT;
            } else {
                /* @hack spelling: stills->stills read again, though own holds
                 * the same pointer (MOV.L @(R0,R5),R0 at 0x8c9026fe);
                 * through own, 48 of 212 words differ */
                stills->texlist = stills->stills->texlists[stills->pvm];
            }
        }
        break;
    case STILLS_WAIT:
        break;
    case LAST_FADE_IN:
        stills->colors[0].a = stills->step / stills->fade_steps;
        if (stills->colors[0].a > 1.0F) {
            stills->colors[0].a = 1.0F;
        }
        stills->step += 1.0F;
        if (stills->step > stills->fade_steps) {
            stills->mode = LAST_HOLD;
            stills->step = 0.0F;
        }
        break;
    case LAST_HOLD:
        stills->step += 1.0F;
        if (stills->step > LAST_HOLD_STEPS) {
            stills->step = 0.0F;
            stills->mode = LAST_FADE_OUT;
        }
        break;
    case LAST_FADE_OUT:
        stills->colors[0].a =
            (stills->fade_steps - stills->step) / stills->fade_steps;
        if (stills->colors[0].a < 0.0F) {
            stills->colors[0].a = 0.0F;
        }
        stills->step += 1.0F;
        if (stills->step > stills->fade_steps) {
            stills->mode = STILLS_DONE;
            stills->step = 0.0F;
        }
        break;
    case STILLS_DONE:
        task->draw = NULL;
        stills->done = 1;
        break;
    }
}

/* draw the still or the last picture, as the mode says, in colors[0].
 * whatever the mode, cover the screen behind it in one color. the cover is
 * texlist's first texture, at u and v 1 in every corner */
static void stills_draw(struct task *task)
{
    struct stills *stills = task->extra;
    NJS_TEXTURE_VTX vertices[4];
    NJS_TEXTURE_VTX *v0;
    NJS_TEXTURE_VTX *v1;
    NJS_TEXTURE_VTX *v2;
    NJS_TEXTURE_VTX *v3;
    Float depth;
    Float left;
    Float top;
    Float right;
    Float bottom;
    Uint32 texture;

    screen_quad_color(color_from_argb_unit(&stills->colors[0]));
    if (stills->mode >= STILLS_FADE_IN && stills->mode <= STILLS_FADE_OUT) {
        njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
        njSetTexture(stills->texlist);
        screen_quad_draw_scaled(stills->texture | QUAD_CENTERED,
                                SCREEN_MIDDLE_X, 277.5F, stills->pos.z, 1.5F,
                                1.5F);
    } else if (stills->mode >= LAST_FADE_IN && stills->mode <= LAST_FADE_OUT) {
        njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
        njSetTexture(stills->stills->last_texlist);
        screen_parts_draw((struct screen_parts *)stills->stills->last_parts,
                          0.0F, 0.0F, stills->pos.z);
    }
    left = 0.0F;
    top = 0.0F;
    right = SCREEN_WIDTH;
    bottom = SCREEN_HEIGHT;
    depth = 1.0F / (stills->pos.z + 10.0F);
    /* @hack redundant: a store to bottom that never runs. SHC drops its code
     * but not its weight on the registers, which keeps the loads at
     * 0x8c902850 and 0x8c902854 in FR8 and FR5, and the copy of FR15 at
     * 0x8c902872 in FR6; without it, the three rotate and 11 of 148 words
     * differ */
    if (0) {
        bottom = 2.0F;
    }
    v0 = &vertices[0];
    v0->x = left;
    v0->y = top;
    v1 = &vertices[1];
    v1->x = left;
    v1->y = bottom;
    /* @hack spelling: v2 set inside the store of its x, which keeps it in R7
     * and v3 in R5 (ADD #48,R7 at 0x8c902870, ADD #72,R5 at 0x8c902878);
     * set apart, the two swap and 19 of 148 words differ */
    (v2 = &vertices[2])->x = right;
    v2->y = top;
    v3 = &vertices[3];
    v3->x = right;
    v3->y = bottom;
    v0->z = v1->z = v2->z = v3->z = depth;
    v0->u = 1.0F;
    v0->v = 1.0F;
    v1->u = 1.0F;
    v1->v = 1.0F;
    v2->u = 1.0F;
    v2->v = 1.0F;
    v3->u = 1.0F;
    v3->v = 1.0F;
    v0->col = v1->col = v2->col = v3->col = 0xffffffff;
    texture = screen_texture_index(stills->texlist, 0);
    njSetTexture(stills->texlist);
    njDrawTexture(vertices, 4, texture, 0);
}

/* create the task that shows story's stills, in level. each still takes
 * an even share of the steps the credits take to scroll past
 * LINE_STILLS_END. each of its fades takes a tenth of that share. return
 * the task, or null when task_create fails */
static struct task *stills_create(int level, struct staff_stills *stills,
                                  int story, struct staff_script *script)
{
    struct task *task = task_create(TASK_WORK, level, stills_step);
    struct stills *work;
    struct staff_stills *own;
    /* @hack type: the two heights as an NJS_POINT2, which keeps them in the
     * frame through R12 (FMOV.S FR3,@R12 at 0x8c9029ce); as two Float
     * locals, 166 of 173 words differ */
    NJS_POINT2 heights;
    Float steps;
    Float per_still;
    int count;
    int i;

    if (!task) {
        /* @hack spelling: a return of task, null here, in place of NULL, which
         * branches to the success path's MOV R13,R0 (0x8c902a8a); returning
         * NULL, 172 of 173 words differ */
        return task;
    }
    work = task->extra = heap_calloc(1, sizeof(struct stills));
    work->stills = &stills[story];
    work->pos.x = 352.0F;
    work->pos.y = 32.0F;
    work->pos.z = 20000.0F;
    /* @hack spelling: own set inside the load of its first texlist, which
     * keeps own in level's frame slot (MOV.L R2,@R15 at 0x8c902988); set
     * apart, own takes R11 and 163 of 173 words differ */
    work->texlist = (own = &stills[story])->texlists[0];
    work->texture = 0;
    heights.x = credits_height(script);
    heights.y = credits_stills_height(script);
    steps = credits_steps(story);
    steps *= heights.y / heights.x;
    count = stills_count(own);
    per_still = steps / count;
    work->hold_steps = per_still * 0.8F;
    work->fade_steps = per_still * 0.1F;
    work->start = 0.0F;
    work->full = work->start + work->fade_steps;
    work->fade_out = work->full + work->hold_steps;
    work->duration = per_still;
    for (i = 0; i < 2; i++) {
        work->colors[i].a = work->colors[i].r = work->colors[i].g =
            work->colors[i].b = 1.0F;
    }
    task->draw = stills_draw;
    return task;
}

/* move the stills to STILLS_FADE_OUT, and keep value in unk_06. nothing
 * calls it, and SHC would leave it out as a static */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void stills_fade_out(struct task *task, int value)
{
    /* @bug memory: task's extra is read before task is tested for null (MOV.L
     * @(44,R4),R6 at 0x8c902a9e) */
    struct stills *stills = task->extra;

    if (task) {
        stills->unk_06 = value;
        stills->mode = STILLS_FADE_OUT;
    }
}

/* start the last picture's fade in. the picture holds LAST_HOLD_STEPS
 * between its fades, which take LAST_FADE_STEPS each */
static void stills_show_last(struct task *task)
{
    struct stills *stills = task->extra;

    stills->fade_steps = LAST_FADE_STEPS;
    stills->mode = LAST_FADE_IN;
    /* @hack redundant: a return at the end, which keeps staff_roll_step's call
     * a BSR (0x8c901e10); without it, SHC inlines the function there, and
     * 41 of staff_roll_step's 201 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* the number of stills in the story's pvms, one to a texture */
static int stills_count(struct staff_stills *stills)
{
    int total = 0;
    int i;
    NJS_TEXLIST *texlist;

    for (i = 0; i < stills->count; i++) {
        texlist = stills->texlists[i];
        total += texlist->nbTexture;
    }
    return total;
}

static void stills_load(struct staff_stills *stills)
{
    int i;

    for (i = 0; i < stills->count; i++) {
        texture_load_pvm(stills->files[i], stills->texlists[i]);
    }
    if (stills->last_file && stills->last_texlist) {
        texture_load_pvm(stills->last_file, stills->last_texlist);
    }
}

static void stills_release(struct staff_stills *stills)
{
    int i;

    for (i = 0; i < stills->count; i++) {
        if (stills->texlists[i]) {
            njReleaseTexture(stills->texlists[i]);
        }
    }
    if (stills->last_texlist) {
        njReleaseTexture(stills->last_texlist);
    }
}

/* nothing calls it, and SHC would leave it out as a static */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void fun_8c902bb0(void)
{
}
