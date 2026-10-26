/* the kinds of object the stage places: each one's task, and its name */
#include "stg04/kinds.h"
#include "collision/invisible.h"
#include "enemy/spina.h"
#include "enemy/uni.h"
#include "object/balloon.h"
#include "object/capsule.h"
#include "object/dash_panel.h"
#include "object/dash_ring.h"
#include "object/hint.h"
#include "object/iron_ball.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/knuckles_hint.h"
#include "object/kusa.h"
#include "object/panel.h"
#include "object/piece.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/ring_group.h"
#include "object/rocket.h"
#include "object/savepoint.h"
#include "object/spring.h"
#include "object/spring_h.h"
#include "object/switch.h"
#include "object/tikal.h"
#include "object/toge_bubble.h"
#include "stg04/antenna.h"
#include "stg04/arcade.h"
#include "stg04/barrier.h"
#include "stg04/bell.h"
#include "stg04/bench.h"
#include "stg04/billboard.h"
#include "stg04/bucket.h"
#include "stg04/clock.h"
#include "stg04/cone.h"
#include "stg04/corner_light.h"
#include "stg04/crane.h"
#include "stg04/curb.h"
#include "stg04/escalator.h"
#include "stg04/fence.h"
#include "stg04/fence_2.h"
#include "stg04/fountain.h"
#include "stg04/gate.h"
#include "stg04/glass.h"
#include "stg04/green.h"
#include "stg04/group.h"
#include "stg04/heli.h"
#include "stg04/hydrant.h"
#include "stg04/lamp.h"
#include "stg04/lamp_a.h"
#include "stg04/lawn.h"
#include "stg04/missile.h"
#include "stg04/neon.h"
#include "stg04/news_box.h"
#include "stg04/paternoster.h"
#include "stg04/pinpin.h"
#include "stg04/platform.h"
#include "stg04/police.h"
#include "stg04/poster.h"
#include "stg04/raft.h"
#include "stg04/ring_2.h"
#include "stg04/searchlight.h"
#include "stg04/shop_sign.h"
#include "stg04/sign.h"
#include "stg04/street_plant_1.h"
#include "stg04/street_plant_2.h"
#include "stg04/tank.h"
#include "stg04/turn_platform.h"
#include "stg04/wall_poster.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 7
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 7
#define TEXT_24_LENGTH 7
#define TEXT_25_LENGTH 7
#define TEXT_26_LENGTH 7
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 8
#define TEXT_29_LENGTH 11
#define TEXT_30_LENGTH 11
#define TEXT_31_LENGTH 7
#define TEXT_32_LENGTH 7
#define TEXT_33_LENGTH 9
#define TEXT_34_LENGTH 8
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 10
#define TEXT_37_LENGTH 10
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
#define TEXT_54_LENGTH 9
#define TEXT_55_LENGTH 9
#define TEXT_56_LENGTH 9
#define TEXT_57_LENGTH 7
#define TEXT_58_LENGTH 9
#define TEXT_59_LENGTH 9
#define TEXT_60_LENGTH 8
#define TEXT_61_LENGTH 9
#define TEXT_62_LENGTH 7
#define TEXT_63_LENGTH 8
#define TEXT_64_LENGTH 8
#define TEXT_65_LENGTH 8
#define TEXT_66_LENGTH 6
#define TEXT_67_LENGTH 8
#define TEXT_68_LENGTH 8
#define TEXT_69_LENGTH 8
#define TEXT_70_LENGTH 8
#define TEXT_71_LENGTH 12
#define TEXT_72_LENGTH 12
#define TEXT_73_LENGTH 8
#define TEXT_74_LENGTH 7
#define TEXT_75_LENGTH 7
#define TEXT_76_LENGTH 6
#define TEXT_77_LENGTH 9
#define TEXT_78_LENGTH 8
#define TEXT_79_LENGTH 8
#define TEXT_80_LENGTH 8
#define TEXT_81_LENGTH 7
#define TEXT_82_LENGTH 6
#define TEXT_83_LENGTH 4
#define TEXT_84_LENGTH 4
#define TEXT_85_LENGTH 11
#define TEXT_86_LENGTH 11
#define TEXT_87_LENGTH 7
#define TEXT_88_LENGTH 7
#define TEXT_89_LENGTH 9
#define TEXT_90_LENGTH 9
#define TEXT_91_LENGTH 8
#define TEXT_92_LENGTH 7
#define TEXT_93_LENGTH 7
#define TEXT_94_LENGTH 7
#define TEXT_95_LENGTH 7
#define TEXT_96_LENGTH 8
#define TEXT_97_LENGTH 7
#define TEXT_98_LENGTH 7
#define TEXT_99_LENGTH 8
#define TEXT_100_LENGTH 10
#define TEXT_101_LENGTH 6
#define TEXT_102_LENGTH 7
#define TEXT_103_LENGTH 7
#define TEXT_104_LENGTH 6
#define TEXT_105_LENGTH 7
#define TEXT_106_LENGTH 8
#define TEXT_107_LENGTH 7
#define TEXT_108_LENGTH 10
#define TEXT_109_LENGTH 9
#define TEXT_110_LENGTH 5
#define TEXT_111_LENGTH 8
#define TEXT_112_LENGTH 8
#define TEXT_113_LENGTH 8
#define TEXT_114_LENGTH 8
#define TEXT_115_LENGTH 8
#define TEXT_116_LENGTH 9
#define TEXT_117_LENGTH 12
#define TEXT_118_LENGTH 7
#include "stg04/kinds/text.inc"
#include "stg04/kinds/values.inc"

/* each kind's create flags, level, attr and range come from the disc,
 * through values.inc. every kind's param, which its task's work->vars.p[0]
 * would hold, is NULL */
struct object_kind highway_object_kinds[HIGHWAY_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, spring_step, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, spring_b_step, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, dash_panel_step, TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, iron_ball_init, TEXT_5},
    {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, iron_ball_jg_init, TEXT_6},
    {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, toge_init, TEXT_7},
    {VALUE_29, VALUE_30, VALUE_31, VALUE_32, NULL, piece_start, TEXT_8},
    {VALUE_33, VALUE_34, VALUE_35, VALUE_36, NULL, capsule_start, TEXT_9},
    {VALUE_37, VALUE_38, VALUE_39, VALUE_40, NULL, switch_step, TEXT_10},
    {VALUE_41, VALUE_42, VALUE_43, VALUE_44, NULL, kusa_step, TEXT_11},
    {VALUE_45, VALUE_46, VALUE_47, VALUE_48, NULL, dash_ring_step, TEXT_12},
    {VALUE_49, VALUE_50, VALUE_51, VALUE_52, NULL, balloon_step, TEXT_13},
    {VALUE_53, VALUE_54, VALUE_55, VALUE_56, NULL, item_box_step, TEXT_14},
    {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, rocket_h_step, TEXT_15},
    {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, rocket_hs_step, TEXT_16},
    {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, rocket_v_step, TEXT_17},
    {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, rocket_vs_step, TEXT_18},
    {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, panel_start, TEXT_19},
    {VALUE_77, VALUE_78, VALUE_79, VALUE_80, NULL, savepoint_step, TEXT_20},
    {VALUE_81, VALUE_82, VALUE_83, VALUE_84, NULL, invisible_wall_step,
     TEXT_21},
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, knuckles_hint_init, TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, spring_h_start, TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, paternoster_step, TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, fountain_step, TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, crane_step, TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, glass_step, TEXT_27},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, glass_2_step, TEXT_28},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, highway_raft_a_step,
     TEXT_29},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, highway_raft_c_step,
     TEXT_30},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, tank_step, TEXT_31},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, billboard_step, TEXT_32},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, turn_platform_step,
     TEXT_33},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, searchlight_step,
     TEXT_34},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL, arcade_1_step, TEXT_35},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL, arcade_2_step, TEXT_36},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL, arcade_3_step, TEXT_37},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL, barrier_step, TEXT_38},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL, platform_sink_step,
     TEXT_39},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL, platform_step, TEXT_40},
    {VALUE_161, VALUE_162, VALUE_163, VALUE_164, NULL, platform_fly_step,
     TEXT_41},
    {VALUE_165, VALUE_166, VALUE_167, VALUE_168, NULL, wall_poster_1_step,
     TEXT_42},
    {VALUE_169, VALUE_170, VALUE_171, VALUE_172, NULL, wall_poster_4_step,
     TEXT_43},
    {VALUE_173, VALUE_174, VALUE_175, VALUE_176, NULL, news_box_1_step,
     TEXT_44},
    {VALUE_177, VALUE_178, VALUE_179, VALUE_180, NULL, news_box_2_step,
     TEXT_45},
    {VALUE_181, VALUE_182, VALUE_183, VALUE_184, NULL, news_box_3_step,
     TEXT_46},
    {VALUE_185, VALUE_186, VALUE_187, VALUE_188, NULL, news_box_4_step,
     TEXT_47},
    {VALUE_189, VALUE_190, VALUE_191, VALUE_192, NULL, bench_step, TEXT_48},
    {VALUE_193, VALUE_194, VALUE_195, VALUE_196, NULL, fence_step, TEXT_49},
    {VALUE_197, VALUE_198, VALUE_199, VALUE_200, NULL, neon_1_step, TEXT_50},
    {VALUE_201, VALUE_202, VALUE_203, VALUE_204, NULL, neon_2_step, TEXT_51},
    {VALUE_205, VALUE_206, VALUE_207, VALUE_208, NULL, neon_3_step, TEXT_52},
    {VALUE_209, VALUE_210, VALUE_211, VALUE_212, NULL, neon_4_step, TEXT_53},
    {VALUE_213, VALUE_214, VALUE_215, VALUE_216, NULL, highway_poster_0_step,
     TEXT_54},
    {VALUE_217, VALUE_218, VALUE_219, VALUE_220, NULL, highway_poster_1_step,
     TEXT_55},
    {VALUE_221, VALUE_222, VALUE_223, VALUE_224, NULL, highway_poster_2_step,
     TEXT_56},
    {VALUE_225, VALUE_226, VALUE_227, VALUE_228, NULL, highway_sign_step,
     TEXT_57},
    {VALUE_229, VALUE_230, VALUE_231, VALUE_232, NULL, shop_sign_a_step,
     TEXT_58},
    {VALUE_233, VALUE_234, VALUE_235, VALUE_236, NULL, shop_sign_b_step,
     TEXT_59},
    {VALUE_237, VALUE_238, VALUE_239, VALUE_240, NULL, bucket_step, TEXT_60},
    {VALUE_241, VALUE_242, VALUE_243, VALUE_244, NULL, hydrant_step, TEXT_61},
    {VALUE_245, VALUE_246, VALUE_247, VALUE_248, NULL, green_step, TEXT_62},
    {VALUE_249, VALUE_250, VALUE_251, VALUE_252, NULL, green_a_step, TEXT_63},
    {VALUE_253, VALUE_254, VALUE_255, VALUE_256, NULL, green_b_step, TEXT_64},
    {VALUE_257, VALUE_258, VALUE_259, VALUE_260, NULL, green_d_step, TEXT_65},
    {VALUE_261, VALUE_262, VALUE_263, VALUE_264, NULL, highway_lamp_step,
     TEXT_66},
    {VALUE_265, VALUE_266, VALUE_267, VALUE_268, NULL, corner_light_step,
     TEXT_67},
    {VALUE_269, VALUE_270, VALUE_271, VALUE_272, NULL, highway_lamp_1_step,
     TEXT_68},
    {VALUE_273, VALUE_274, VALUE_275, VALUE_276, NULL, highway_lamp_2_step,
     TEXT_69},
    {VALUE_277, VALUE_278, VALUE_279, VALUE_280, NULL, pinpin_step, TEXT_70},
    {VALUE_281, VALUE_282, VALUE_283, VALUE_284, NULL, escalator_1_step,
     TEXT_71},
    {VALUE_285, VALUE_286, VALUE_287, VALUE_288, NULL, escalator_2_step,
     TEXT_72},
    {VALUE_289, VALUE_290, VALUE_291, VALUE_292, NULL, antenna_step, TEXT_73},
    {VALUE_293, VALUE_294, VALUE_295, VALUE_296, NULL, cone_1_step, TEXT_74},
    {VALUE_297, VALUE_298, VALUE_299, VALUE_300, NULL, cone_2_step, TEXT_75},
    {VALUE_301, VALUE_302, VALUE_303, VALUE_304, NULL, curb_step, TEXT_76},
    {VALUE_305, VALUE_306, VALUE_307, VALUE_308, NULL, fence_2_step, TEXT_77},
    {VALUE_309, VALUE_310, VALUE_311, VALUE_312, NULL, green_e_step, TEXT_78},
    {VALUE_313, VALUE_314, VALUE_315, VALUE_316, NULL, lawn_1_step, TEXT_79},
    {VALUE_317, VALUE_318, VALUE_319, VALUE_320, NULL, lawn_2_step, TEXT_80},
    {VALUE_321, VALUE_322, VALUE_323, VALUE_324, NULL, highway_clock_step,
     TEXT_81},
    {VALUE_325, VALUE_326, VALUE_327, VALUE_328, NULL, highway_lamp_a_step,
     TEXT_82},
    {VALUE_329, VALUE_330, VALUE_331, VALUE_332, NULL, highway_gate_step,
     TEXT_83},
    {VALUE_333, VALUE_334, VALUE_335, VALUE_336, NULL, fence_ff_step, TEXT_84},
    {VALUE_337, VALUE_338, VALUE_339, VALUE_340, NULL, street_plant_1_step,
     TEXT_85},
    {VALUE_341, VALUE_342, VALUE_343, VALUE_344, NULL, street_plant_2_step,
     TEXT_86},
    {VALUE_345, VALUE_346, VALUE_347, VALUE_348, NULL, heli_a_step, TEXT_87},
    {VALUE_349, VALUE_350, VALUE_351, VALUE_352, NULL, heli_b_step, TEXT_88},
    {VALUE_353, VALUE_354, VALUE_355, VALUE_356, NULL, bell_step, TEXT_89},
    {VALUE_357, VALUE_358, VALUE_359, VALUE_360, NULL, heliport_light_step,
     TEXT_90},
    {VALUE_361, VALUE_362, VALUE_363, VALUE_364, NULL, highway_raft_joint_step,
     TEXT_91},
    {VALUE_365, VALUE_366, VALUE_367, VALUE_368, NULL, ring_2_step, TEXT_92},
    {VALUE_369, VALUE_370, VALUE_371, VALUE_372, NULL, spina_a_step, TEXT_93},
    {VALUE_373, VALUE_374, VALUE_375, VALUE_376, NULL, spina_b_step, TEXT_94},
    {VALUE_377, VALUE_378, VALUE_379, VALUE_380, NULL, spina_c_step, TEXT_95},
    {VALUE_381, VALUE_382, VALUE_383, VALUE_384, NULL, police_init, TEXT_96},
    {VALUE_385, VALUE_386, VALUE_387, VALUE_388, NULL, uni_a_start, TEXT_97},
    {VALUE_389, VALUE_390, VALUE_391, VALUE_392, NULL, uni_b_start, TEXT_98},
    {VALUE_393, VALUE_394, VALUE_395, VALUE_396, NULL, invisible_sphere_step,
     TEXT_99},
    {VALUE_397, VALUE_398, VALUE_399, VALUE_400, NULL, invisible_cylinder_step,
     TEXT_100},
    {VALUE_401, VALUE_402, VALUE_403, VALUE_404, NULL, invisible_box_step,
     TEXT_101},
    {VALUE_405, VALUE_406, VALUE_407, VALUE_408, NULL, ottotto_step, TEXT_102},
    {VALUE_409, VALUE_410, VALUE_411, VALUE_412, NULL, tikal_init, TEXT_103},
    {VALUE_413, VALUE_414, VALUE_415, VALUE_416, NULL, hint_init, TEXT_104},
    {VALUE_417, VALUE_418, VALUE_419, VALUE_420, NULL, ring_group_step,
     TEXT_105},
    {VALUE_421, VALUE_422, VALUE_423, VALUE_424, NULL, fence_group_step,
     TEXT_106},
    {VALUE_425, VALUE_426, VALUE_427, VALUE_428, NULL, curb_group_step,
     TEXT_107},
    {VALUE_429, VALUE_430, VALUE_431, VALUE_432, NULL, fence_2_group_step,
     TEXT_108},
    {VALUE_433, VALUE_434, VALUE_435, VALUE_436, NULL, pinpin_group_step,
     TEXT_109},
    {VALUE_437, VALUE_438, VALUE_439, VALUE_440, NULL, fence_ff_group_step,
     TEXT_110},
    {VALUE_441, VALUE_442, VALUE_443, VALUE_444, NULL, raft_a_group_step,
     TEXT_111},
    {VALUE_445, VALUE_446, VALUE_447, VALUE_448, NULL, raft_c_group_step,
     TEXT_112},
    {VALUE_449, VALUE_450, VALUE_451, VALUE_452, NULL, green_a_group_step,
     TEXT_113},
    {VALUE_453, VALUE_454, VALUE_455, VALUE_456, NULL, green_b_group_step,
     TEXT_114},
    {VALUE_457, VALUE_458, VALUE_459, VALUE_460, NULL, green_d_group_step,
     TEXT_115},
    {VALUE_461, VALUE_462, VALUE_463, VALUE_464, NULL, corner_light_group_step,
     TEXT_116},
    {VALUE_465, VALUE_466, VALUE_467, VALUE_468, NULL, item_box_air_step,
     TEXT_117},
    {VALUE_469, VALUE_470, VALUE_471, VALUE_472, NULL, highway_missile_step,
     TEXT_118},
};
