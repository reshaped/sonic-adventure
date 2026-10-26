/* the kinds of object the stage places: each one's task, and its name */
#include "stg09/kinds.h"
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
#include "object/switch.h"
#include "object/tikal.h"
#include "object/toge_bubble.h"
#include "stg09/arm.h"
#include "stg09/banju_door.h"
#include "stg09/big_star.h"
#include "stg09/black_wall.h"
#include "stg09/bumper.h"
#include "stg09/burst_statue.h"
#include "stg09/cannon.h"
#include "stg09/card_sign.h"
#include "stg09/card_uv.h"
#include "stg09/cards.h"
#include "stg09/casino_flame.h"
#include "stg09/chao.h"
#include "stg09/counter.h"
#include "stg09/cowgirl.h"
#include "stg09/crystal.h"
#include "stg09/damage_sphere.h"
#include "stg09/decor_arrow.h"
#include "stg09/dice.h"
#include "stg09/drop_target.h"
#include "stg09/emerald.h"
#include "stg09/emergency.h"
#include "stg09/fan.h"
#include "stg09/flies.h"
#include "stg09/flipper.h"
#include "stg09/flower_bed.h"
#include "stg09/gear.h"
#include "stg09/gold.h"
#include "stg09/gold_coin.h"
#include "stg09/guide_sphere.h"
#include "stg09/hand.h"
#include "stg09/hook.h"
#include "stg09/idea.h"
#include "stg09/invisible.h"
#include "stg09/jdai.h"
#include "stg09/kaz.h"
#include "stg09/kk_door.h"
#include "stg09/ladder.h"
#include "stg09/lamp.h"
#include "stg09/land_anim.h"
#include "stg09/lanes.h"
#include "stg09/launcher.h"
#include "stg09/light.h"
#include "stg09/lion.h"
#include "stg09/loop.h"
#include "stg09/manf.h"
#include "stg09/mirror_ball.h"
#include "stg09/mud.h"
#include "stg09/neon.h"
#include "stg09/neon_sign.h"
#include "stg09/obstacle.h"
#include "stg09/pinball.h"
#include "stg09/pinball_camera.h"
#include "stg09/pirate.h"
#include "stg09/props.h"
#include "stg09/rides.h"
#include "stg09/roll.h"
#include "stg09/roll_up.h"
#include "stg09/route_egg.h"
#include "stg09/shower.h"
#include "stg09/shutter.h"
#include "stg09/sign.h"
#include "stg09/six_slot.h"
#include "stg09/sling.h"
#include "stg09/slot_sign.h"
#include "stg09/sonic_statue.h"
#include "stg09/sr_door.h"
#include "stg09/stopper.h"
#include "stg09/table_lamps.h"
#include "stg09/table_neon.h"
#include "stg09/tanemono.h"
#include "stg09/transfer_gate.h"
#include "stg09/ts_door.h"
#include "stg09/tube.h"
#include "stg09/water_drops.h"
#include "stg09/water_spray.h"

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
#define TEXT_15_LENGTH 12
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 8
#define TEXT_24_LENGTH 12
#define TEXT_25_LENGTH 7
#define TEXT_26_LENGTH 8
#define TEXT_27_LENGTH 10
#define TEXT_28_LENGTH 6
#define TEXT_29_LENGTH 7
#define TEXT_30_LENGTH 7
#define TEXT_31_LENGTH 7
#define TEXT_32_LENGTH 8
#define TEXT_33_LENGTH 3
#define TEXT_34_LENGTH 9
#define TEXT_35_LENGTH 8
#define TEXT_36_LENGTH 9
#define TEXT_37_LENGTH 8
#define TEXT_38_LENGTH 9
#define TEXT_39_LENGTH 7
#define TEXT_40_LENGTH 7
#define TEXT_41_LENGTH 6
#define TEXT_42_LENGTH 6
#define TEXT_43_LENGTH 7
#define TEXT_44_LENGTH 7
#define TEXT_45_LENGTH 7
#define TEXT_46_LENGTH 4
#define TEXT_47_LENGTH 5
#define TEXT_48_LENGTH 5
#define TEXT_49_LENGTH 7
#define TEXT_50_LENGTH 8
#define TEXT_51_LENGTH 7
#define TEXT_52_LENGTH 7
#define TEXT_53_LENGTH 9
#define TEXT_54_LENGTH 5
#define TEXT_55_LENGTH 9
#define TEXT_56_LENGTH 9
#define TEXT_57_LENGTH 4
#define TEXT_58_LENGTH 8
#define TEXT_59_LENGTH 8
#define TEXT_60_LENGTH 8
#define TEXT_61_LENGTH 8
#define TEXT_62_LENGTH 7
#define TEXT_63_LENGTH 7
#define TEXT_64_LENGTH 7
#define TEXT_65_LENGTH 4
#define TEXT_66_LENGTH 4
#define TEXT_67_LENGTH 8
#define TEXT_68_LENGTH 4
#define TEXT_69_LENGTH 5
#define TEXT_70_LENGTH 7
#define TEXT_71_LENGTH 4
#define TEXT_72_LENGTH 10
#define TEXT_73_LENGTH 8
#define TEXT_74_LENGTH 8
#define TEXT_75_LENGTH 8
#define TEXT_76_LENGTH 8
#define TEXT_77_LENGTH 8
#define TEXT_78_LENGTH 8
#define TEXT_79_LENGTH 6
#define TEXT_80_LENGTH 8
#define TEXT_81_LENGTH 8
#define TEXT_82_LENGTH 8
#define TEXT_83_LENGTH 9
#define TEXT_84_LENGTH 9
#define TEXT_85_LENGTH 7
#define TEXT_86_LENGTH 9
#define TEXT_87_LENGTH 6
#define TEXT_88_LENGTH 7
#define TEXT_89_LENGTH 7
#define TEXT_90_LENGTH 7
#define TEXT_91_LENGTH 7
#define TEXT_92_LENGTH 9
#define TEXT_93_LENGTH 10
#define TEXT_94_LENGTH 11
#define TEXT_95_LENGTH 8
#define TEXT_96_LENGTH 8
#define TEXT_97_LENGTH 6
#define TEXT_98_LENGTH 10
#define TEXT_99_LENGTH 6
#define TEXT_100_LENGTH 9
#define TEXT_101_LENGTH 9
#define TEXT_102_LENGTH 9
#define TEXT_103_LENGTH 6
#define TEXT_104_LENGTH 6
#define TEXT_105_LENGTH 6
#define TEXT_106_LENGTH 6
#define TEXT_107_LENGTH 6
#define TEXT_108_LENGTH 6
#define TEXT_109_LENGTH 6
#define TEXT_110_LENGTH 6
#define TEXT_111_LENGTH 5
#define TEXT_112_LENGTH 7
#define TEXT_113_LENGTH 6
#define TEXT_114_LENGTH 6
#define TEXT_115_LENGTH 10
#define TEXT_116_LENGTH 8
#define TEXT_117_LENGTH 8
#define TEXT_118_LENGTH 8
#define TEXT_119_LENGTH 8
#define TEXT_120_LENGTH 8
#define TEXT_121_LENGTH 8
#define TEXT_122_LENGTH 4
#define TEXT_123_LENGTH 6
#define TEXT_124_LENGTH 6
#define TEXT_125_LENGTH 7
#define TEXT_126_LENGTH 6
#define TEXT_127_LENGTH 6
#define TEXT_128_LENGTH 7
#define TEXT_129_LENGTH 7
#define TEXT_130_LENGTH 5
#define TEXT_131_LENGTH 6
#define TEXT_132_LENGTH 6
#define TEXT_133_LENGTH 5
#define TEXT_134_LENGTH 5
#define TEXT_135_LENGTH 5
#define TEXT_136_LENGTH 5
#define TEXT_137_LENGTH 5
#define TEXT_138_LENGTH 5
#define TEXT_139_LENGTH 5
#define TEXT_140_LENGTH 5
#define TEXT_141_LENGTH 5
#define TEXT_142_LENGTH 5
#define TEXT_143_LENGTH 5
#define TEXT_144_LENGTH 5
#define TEXT_145_LENGTH 5
#define TEXT_146_LENGTH 5
#define TEXT_147_LENGTH 8
#define TEXT_148_LENGTH 6
#define TEXT_149_LENGTH 8
#define TEXT_150_LENGTH 4
#define TEXT_151_LENGTH 7
#define TEXT_152_LENGTH 7
#define TEXT_153_LENGTH 9
#define TEXT_154_LENGTH 9
#define TEXT_155_LENGTH 7
#define TEXT_156_LENGTH 5
#define TEXT_157_LENGTH 7
#define TEXT_158_LENGTH 7
#define TEXT_159_LENGTH 5
#define TEXT_160_LENGTH 5
#define TEXT_161_LENGTH 7
#define TEXT_162_LENGTH 7
#define TEXT_163_LENGTH 7
#define TEXT_164_LENGTH 7
#define TEXT_165_LENGTH 8
#define TEXT_166_LENGTH 8
#define TEXT_167_LENGTH 9
#define TEXT_168_LENGTH 9
#define TEXT_169_LENGTH 7
#define TEXT_170_LENGTH 8
#define TEXT_171_LENGTH 6
#define TEXT_172_LENGTH 11
#define TEXT_173_LENGTH 9
#define TEXT_174_LENGTH 7
#define TEXT_175_LENGTH 7
#define TEXT_176_LENGTH 9
#define TEXT_177_LENGTH 8
#define TEXT_178_LENGTH 7
#define TEXT_179_LENGTH 6
#define TEXT_180_LENGTH 7
#define TEXT_181_LENGTH 8
#define TEXT_182_LENGTH 8
#define TEXT_183_LENGTH 8
#define TEXT_184_LENGTH 6
#define TEXT_185_LENGTH 6
#define TEXT_186_LENGTH 5
#define TEXT_187_LENGTH 6
#define TEXT_188_LENGTH 5
#define TEXT_189_LENGTH 8
#define TEXT_190_LENGTH 8
#define TEXT_191_LENGTH 7
#define TEXT_192_LENGTH 7
#define TEXT_193_LENGTH 7
#define TEXT_194_LENGTH 7
#define TEXT_195_LENGTH 5
#define TEXT_196_LENGTH 7
#define TEXT_197_LENGTH 9
#define TEXT_198_LENGTH 10
#define TEXT_199_LENGTH 10
#define TEXT_200_LENGTH 9
#define TEXT_201_LENGTH 9
#define TEXT_202_LENGTH 7
#define TEXT_203_LENGTH 7
#define TEXT_204_LENGTH 7
#define TEXT_205_LENGTH 7
#define TEXT_206_LENGTH 7
#include "stg09/kinds/text.inc"
#include "stg09/kinds/values.inc"

/* each kind's create flags, level, attr and range are the disc's, as values;
 * no kind here has a param */
struct object_kind casino_object_kinds[CASINO_KINDS] = {
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
    {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, item_box_air_step, TEXT_15},
    {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, hint_init, TEXT_16},
    {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, tikal_init, TEXT_17},
    {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, knuckles_hint_init, TEXT_18},
    {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, rocket_h_step, TEXT_19},
    {VALUE_77, VALUE_78, VALUE_79, VALUE_80, NULL, rocket_hs_step, TEXT_20},
    {VALUE_81, VALUE_82, VALUE_83, VALUE_84, NULL, rocket_v_step, TEXT_21},
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, rocket_vs_step, TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, panel_start, TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, savepoint_step, TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, invisible_wall_step,
     TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, invisible_sphere_step,
     TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, invisible_cylinder_step,
     TEXT_27},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, invisible_box_step,
     TEXT_28},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, ottotto_step, TEXT_29},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, ring_group_step,
     TEXT_30},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, casino_emerald_step,
     TEXT_31},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, invisible_dry_step,
     TEXT_32},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, pinball_camera_step,
     TEXT_33},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, start_shot_step,
     TEXT_34},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL, flipper_left_step,
     TEXT_35},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL, flipper_left_x_step,
     TEXT_36},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL, flipper_right_step,
     TEXT_37},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL, flipper_right_x_step,
     TEXT_38},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL, bumper_step, TEXT_39},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL, bumper2_step, TEXT_40},
    {VALUE_161, VALUE_162, VALUE_163, VALUE_164, NULL, sling_left_step,
     TEXT_41},
    {VALUE_165, VALUE_166, VALUE_167, VALUE_168, NULL, sling_right_step,
     TEXT_42},
    {VALUE_169, VALUE_170, VALUE_171, VALUE_172, NULL, stopper_c_step, TEXT_43},
    {VALUE_173, VALUE_174, VALUE_175, VALUE_176, NULL, stopper_l_step, TEXT_44},
    {VALUE_177, VALUE_178, VALUE_179, VALUE_180, NULL, stopper_r_step, TEXT_45},
    {VALUE_181, VALUE_182, VALUE_183, VALUE_184, NULL, roll_step, TEXT_46},
    {VALUE_185, VALUE_186, VALUE_187, VALUE_188, NULL, jdai_left_step, TEXT_47},
    {VALUE_189, VALUE_190, VALUE_191, VALUE_192, NULL, jdai_right_step,
     TEXT_48},
    {VALUE_193, VALUE_194, VALUE_195, VALUE_196, NULL, drop_target_step,
     TEXT_49},
    {VALUE_197, VALUE_198, VALUE_199, VALUE_200, NULL, drop_target2_step,
     TEXT_50},
    {VALUE_201, VALUE_202, VALUE_203, VALUE_204, NULL, roll_up_right_step,
     TEXT_51},
    {VALUE_205, VALUE_206, VALUE_207, VALUE_208, NULL, roll_up_left_step,
     TEXT_52},
    {VALUE_209, VALUE_210, VALUE_211, VALUE_212, NULL, roll_down_left_step,
     TEXT_53},
    {VALUE_213, VALUE_214, VALUE_215, VALUE_216, NULL, lanes_step, TEXT_54},
    {VALUE_217, VALUE_218, VALUE_219, VALUE_220, NULL, right_lane_step,
     TEXT_55},
    {VALUE_221, VALUE_222, VALUE_223, VALUE_224, NULL, table_lamps_step,
     TEXT_56},
    {VALUE_225, VALUE_226, VALUE_227, VALUE_228, NULL, table_neon_step,
     TEXT_57},
    {VALUE_229, VALUE_230, VALUE_231, VALUE_232, NULL, table_neon_wall_step,
     TEXT_58},
    {VALUE_233, VALUE_234, VALUE_235, VALUE_236, NULL, black_wall_step,
     TEXT_59},
    {VALUE_237, VALUE_238, VALUE_239, VALUE_240, NULL, obstacle_left_step,
     TEXT_60},
    {VALUE_241, VALUE_242, VALUE_243, VALUE_244, NULL, obstacle_right_step,
     TEXT_61},
    {VALUE_245, VALUE_246, VALUE_247, VALUE_248, NULL, cannon_left_step,
     TEXT_62},
    {VALUE_249, VALUE_250, VALUE_251, VALUE_252, NULL, cannon_right_step,
     TEXT_63},
    {VALUE_253, VALUE_254, VALUE_255, VALUE_256, NULL, ideacap_step, TEXT_64},
    {VALUE_257, VALUE_258, VALUE_259, VALUE_260, NULL, idea_step, TEXT_65},
    {VALUE_261, VALUE_262, VALUE_263, VALUE_264, NULL, card_step, TEXT_66},
    {VALUE_265, VALUE_266, VALUE_267, VALUE_268, NULL, card_display_step,
     TEXT_67},
    {VALUE_269, VALUE_270, VALUE_271, VALUE_272, NULL, ride_step, TEXT_68},
    {VALUE_273, VALUE_274, VALUE_275, VALUE_276, NULL, dice_step, TEXT_69},
    {VALUE_277, VALUE_278, VALUE_279, VALUE_280, NULL, big_star_step, TEXT_70},
    {VALUE_281, VALUE_282, VALUE_283, VALUE_284, NULL, loop_step, TEXT_71},
    {VALUE_285, VALUE_286, VALUE_287, VALUE_288, NULL, decor_arrow_step,
     TEXT_72},
    {VALUE_289, VALUE_290, VALUE_291, VALUE_292, NULL, tanemono_step, TEXT_73},
    {VALUE_293, VALUE_294, VALUE_295, VALUE_296, NULL, casino_chao_egg_step,
     TEXT_74},
    {VALUE_297, VALUE_298, VALUE_299, VALUE_300, NULL, casino_chao_clap_step,
     TEXT_75},
    {VALUE_301, VALUE_302, VALUE_303, VALUE_304, NULL, casino_chao_walk_step,
     TEXT_76},
    {VALUE_305, VALUE_306, VALUE_307, VALUE_308, NULL, casino_chao_fish_step,
     TEXT_77},
    {VALUE_309, VALUE_310, VALUE_311, VALUE_312, NULL, casino_chao_sleep_step,
     TEXT_78},
    {VALUE_313, VALUE_314, VALUE_315, VALUE_316, NULL, card_uv_step, TEXT_79},
    {VALUE_317, VALUE_318, VALUE_319, VALUE_320, NULL, neon_star_step, TEXT_80},
    {VALUE_321, VALUE_322, VALUE_323, VALUE_324, NULL, neon_arrow_step,
     TEXT_81},
    {VALUE_325, VALUE_326, VALUE_327, VALUE_328, NULL, neon_lights_step,
     TEXT_82},
    {VALUE_329, VALUE_330, VALUE_331, VALUE_332, NULL, emergency_step, TEXT_83},
    {VALUE_333, VALUE_334, VALUE_335, VALUE_336, NULL, route_egg_lower_step,
     TEXT_84},
    {VALUE_337, VALUE_338, VALUE_339, VALUE_340, NULL, route_egg_upper_step,
     TEXT_85},
    {VALUE_341, VALUE_342, VALUE_343, VALUE_344, NULL, banju_door_step,
     TEXT_86},
    {VALUE_345, VALUE_346, VALUE_347, VALUE_348, NULL, NULL, TEXT_87},
    {VALUE_349, VALUE_350, VALUE_351, VALUE_352, NULL, tube_a_step, TEXT_88},
    {VALUE_353, VALUE_354, VALUE_355, VALUE_356, NULL, tube_b_step, TEXT_89},
    {VALUE_357, VALUE_358, VALUE_359, VALUE_360, NULL, tube_c_step, TEXT_90},
    {VALUE_361, VALUE_362, VALUE_363, VALUE_364, NULL, six_slot_step, TEXT_91},
    {VALUE_365, VALUE_366, VALUE_367, VALUE_368, NULL, casino_crystal_step,
     TEXT_92},
    {VALUE_369, VALUE_370, VALUE_371, VALUE_372, NULL, gear_step, TEXT_93},
    {VALUE_373, VALUE_374, VALUE_375, VALUE_376, NULL, damage_sphere_step,
     TEXT_94},
    {VALUE_377, VALUE_378, VALUE_379, VALUE_380, NULL, kk_door_step, TEXT_95},
    {VALUE_381, VALUE_382, VALUE_383, VALUE_384, NULL, sr_door_step, TEXT_96},
    {VALUE_385, VALUE_386, VALUE_387, VALUE_388, NULL, manf_step, TEXT_97},
    {VALUE_389, VALUE_390, VALUE_391, VALUE_392, NULL, counter_step, TEXT_98},
    {VALUE_393, VALUE_394, VALUE_395, VALUE_396, NULL, lion_step, TEXT_99},
    {VALUE_397, VALUE_398, VALUE_399, VALUE_400, NULL, telepot_step, TEXT_100},
    {VALUE_401, VALUE_402, VALUE_403, VALUE_404, NULL, flower_a_step, TEXT_101},
    {VALUE_405, VALUE_406, VALUE_407, VALUE_408, NULL, flower_b_step, TEXT_102},
    {VALUE_409, VALUE_410, VALUE_411, VALUE_412, NULL, kaz_a_step, TEXT_103},
    {VALUE_413, VALUE_414, VALUE_415, VALUE_416, NULL, kaz_b_step, TEXT_104},
    {VALUE_417, VALUE_418, VALUE_419, VALUE_420, NULL, kaz_c_step, TEXT_105},
    {VALUE_421, VALUE_422, VALUE_423, VALUE_424, NULL, kaz_d_step, TEXT_106},
    {VALUE_425, VALUE_426, VALUE_427, VALUE_428, NULL, kaz_e_step, TEXT_107},
    {VALUE_429, VALUE_430, VALUE_431, VALUE_432, NULL, kaz_f_step, TEXT_108},
    {VALUE_433, VALUE_434, VALUE_435, VALUE_436, NULL, kaz_g_step, TEXT_109},
    {VALUE_437, VALUE_438, VALUE_439, VALUE_440, NULL, hand_step, TEXT_110},
    {VALUE_441, VALUE_442, VALUE_443, VALUE_444, NULL, gold_heap_step,
     TEXT_111},
    {VALUE_445, VALUE_446, VALUE_447, VALUE_448, NULL, gold_coin_step,
     TEXT_112},
    {VALUE_449, VALUE_450, VALUE_451, VALUE_452, NULL, shower_step, TEXT_113},
    {VALUE_453, VALUE_454, VALUE_455, VALUE_456, NULL, sonic_statue_step,
     TEXT_114},
    {VALUE_457, VALUE_458, VALUE_459, VALUE_460, NULL,
     sonic_statue_knuckles_step, TEXT_115},
    {VALUE_461, VALUE_462, VALUE_463, VALUE_464, NULL, statue_shard_1_step,
     TEXT_116},
    {VALUE_465, VALUE_466, VALUE_467, VALUE_468, NULL, statue_shard_2_step,
     TEXT_117},
    {VALUE_469, VALUE_470, VALUE_471, VALUE_472, NULL, statue_shard_3_step,
     TEXT_118},
    {VALUE_473, VALUE_474, VALUE_475, VALUE_476, NULL, statue_shard_4_step,
     TEXT_119},
    {VALUE_477, VALUE_478, VALUE_479, VALUE_480, NULL, statue_shard_5_step,
     TEXT_120},
    {VALUE_481, VALUE_482, VALUE_483, VALUE_484, NULL, statue_shard_6_step,
     TEXT_121},
    {VALUE_485, VALUE_486, VALUE_487, VALUE_488, NULL, casino_flame_step,
     TEXT_122},
    {VALUE_489, VALUE_490, VALUE_491, VALUE_492, NULL, saku_step, TEXT_123},
    {VALUE_493, VALUE_494, VALUE_495, VALUE_496, NULL, taru_step, TEXT_124},
    {VALUE_497, VALUE_498, VALUE_499, VALUE_500, NULL, NULL, TEXT_125},
    {VALUE_501, VALUE_502, VALUE_503, VALUE_504, NULL, hako_step, TEXT_126},
    {VALUE_505, VALUE_506, VALUE_507, VALUE_508, NULL, gbox_step, TEXT_127},
    {VALUE_509, VALUE_510, VALUE_511, VALUE_512, NULL, gomi_b_step, TEXT_128},
    {VALUE_513, VALUE_514, VALUE_515, VALUE_516, NULL, gomi_r_step, TEXT_129},
    {VALUE_517, VALUE_518, VALUE_519, VALUE_520, NULL, rlt_step, TEXT_130},
    {VALUE_521, VALUE_522, VALUE_523, VALUE_524, NULL, yukh_step, TEXT_131},
    {VALUE_525, VALUE_526, VALUE_527, VALUE_528, NULL, rlti_step, TEXT_132},
    {VALUE_529, VALUE_530, VALUE_531, VALUE_532, NULL, kb_b_step, TEXT_133},
    {VALUE_533, VALUE_534, VALUE_535, VALUE_536, NULL, kb_r_step, TEXT_134},
    {VALUE_537, VALUE_538, VALUE_539, VALUE_540, NULL, kb_1_step, TEXT_135},
    {VALUE_541, VALUE_542, VALUE_543, VALUE_544, NULL, kb_2_step, TEXT_136},
    {VALUE_545, VALUE_546, VALUE_547, VALUE_548, NULL, kb_3_step, TEXT_137},
    {VALUE_549, VALUE_550, VALUE_551, VALUE_552, NULL, kb_4_step, TEXT_138},
    {VALUE_553, VALUE_554, VALUE_555, VALUE_556, NULL, sign_c_step, TEXT_139},
    {VALUE_557, VALUE_558, VALUE_559, VALUE_560, NULL, sign_s_step, TEXT_140},
    {VALUE_561, VALUE_562, VALUE_563, VALUE_564, NULL, lamp_blue_step,
     TEXT_141},
    {VALUE_565, VALUE_566, VALUE_567, VALUE_568, NULL, lamp_green_step,
     TEXT_142},
    {VALUE_569, VALUE_570, VALUE_571, VALUE_572, NULL, lamp_red_step, TEXT_143},
    {VALUE_573, VALUE_574, VALUE_575, VALUE_576, NULL, lamp_yellow_step,
     TEXT_144},
    {VALUE_577, VALUE_578, VALUE_579, VALUE_580, NULL, fk_a_step, TEXT_145},
    {VALUE_581, VALUE_582, VALUE_583, VALUE_584, NULL, fk_b_step, TEXT_146},
    {VALUE_585, VALUE_586, VALUE_587, VALUE_588, NULL, tdenki_step, TEXT_147},
    {VALUE_589, VALUE_590, VALUE_591, VALUE_592, NULL, arm_step, TEXT_148},
    {VALUE_593, VALUE_594, VALUE_595, VALUE_596, NULL, ts_door_step, TEXT_149},
    {VALUE_597, VALUE_598, VALUE_599, VALUE_600, NULL, mirror_ball_step,
     TEXT_150},
    {VALUE_601, VALUE_602, VALUE_603, VALUE_604, NULL, shutter_step, TEXT_151},
    {VALUE_605, VALUE_606, VALUE_607, VALUE_608, NULL, shutter_tall_step,
     TEXT_152},
    {VALUE_609, VALUE_610, VALUE_611, VALUE_612, NULL, sdfuti_r_step, TEXT_153},
    {VALUE_613, VALUE_614, VALUE_615, VALUE_616, NULL, sdfuti_b_step, TEXT_154},
    {VALUE_617, VALUE_618, VALUE_619, VALUE_620, NULL, map2f_step, TEXT_155},
    {VALUE_621, VALUE_622, VALUE_623, VALUE_624, NULL, cfo_step, TEXT_156},
    {VALUE_625, VALUE_626, VALUE_627, VALUE_628, NULL, mizu_a_step, TEXT_157},
    {VALUE_629, VALUE_630, VALUE_631, VALUE_632, NULL, mizu_b_step, TEXT_158},
    {VALUE_633, VALUE_634, VALUE_635, VALUE_636, NULL, sy_a_step, TEXT_159},
    {VALUE_637, VALUE_638, VALUE_639, VALUE_640, NULL, sy_b_step, TEXT_160},
    {VALUE_641, VALUE_642, VALUE_643, VALUE_644, NULL, transfer_gate_card_step,
     TEXT_161},
    {VALUE_645, VALUE_646, VALUE_647, VALUE_648, NULL, transfer_gate_slot_step,
     TEXT_162},
    {VALUE_649, VALUE_650, VALUE_651, VALUE_652, NULL, shwrl_step, TEXT_163},
    {VALUE_653, VALUE_654, VALUE_655, VALUE_656, NULL, shwhd_step, TEXT_164},
    {VALUE_657, VALUE_658, VALUE_659, VALUE_660, NULL, neokab_step, TEXT_165},
    {VALUE_661, VALUE_662, VALUE_663, VALUE_664, NULL, neon_sign_step,
     TEXT_166},
    {VALUE_665, VALUE_666, VALUE_667, VALUE_668, NULL, card_sign_step,
     TEXT_167},
    {VALUE_669, VALUE_670, VALUE_671, VALUE_672, NULL, slot_sign_step,
     TEXT_168},
    {VALUE_673, VALUE_674, VALUE_675, VALUE_676, NULL, ladder_step, TEXT_169},
    {VALUE_677, VALUE_678, VALUE_679, VALUE_680, NULL, fan_step, TEXT_170},
    {VALUE_681, VALUE_682, VALUE_683, VALUE_684, NULL, hook_step, TEXT_171},
    {VALUE_685, VALUE_686, VALUE_687, VALUE_688, NULL, land_anim_step,
     TEXT_172},
    {VALUE_689, VALUE_690, VALUE_691, VALUE_692, NULL, cowgirl_step, TEXT_173},
    {VALUE_693, VALUE_694, VALUE_695, VALUE_696, NULL, flower_bed_large_step,
     TEXT_174},
    {VALUE_697, VALUE_698, VALUE_699, VALUE_700, NULL, flower_bed_medium_step,
     TEXT_175},
    {VALUE_701, VALUE_702, VALUE_703, VALUE_704, NULL, slotisu_step, TEXT_176},
    {VALUE_705, VALUE_706, VALUE_707, VALUE_708, NULL,
     invisible_rectangle2_step, TEXT_177},
    {VALUE_709, VALUE_710, VALUE_711, VALUE_712, NULL, invisible_cylinder2_step,
     TEXT_178},
    {VALUE_713, VALUE_714, VALUE_715, VALUE_716, NULL, invisible_capsule_step,
     TEXT_179},
    {VALUE_717, VALUE_718, VALUE_719, VALUE_720, NULL, dento_step, TEXT_180},
    {VALUE_721, VALUE_722, VALUE_723, VALUE_724, NULL, light_a_step, TEXT_181},
    {VALUE_725, VALUE_726, VALUE_727, VALUE_728, NULL, light_b_step, TEXT_182},
    {VALUE_729, VALUE_730, VALUE_731, VALUE_732, NULL, six_slot2_step,
     TEXT_183},
    {VALUE_733, VALUE_734, VALUE_735, VALUE_736, NULL, light_r_step, TEXT_184},
    {VALUE_737, VALUE_738, VALUE_739, VALUE_740, NULL, light_g_step, TEXT_185},
    {VALUE_741, VALUE_742, VALUE_743, VALUE_744, NULL, water_spray_step,
     TEXT_186},
    {VALUE_745, VALUE_746, VALUE_747, VALUE_748, NULL, water_spray2_step,
     TEXT_187},
    {VALUE_749, VALUE_750, VALUE_751, VALUE_752, NULL, flies_step, TEXT_188},
    {VALUE_753, VALUE_754, VALUE_755, VALUE_756, NULL, mud_out_step, TEXT_189},
    {VALUE_757, VALUE_758, VALUE_759, VALUE_760, NULL, guide_sphere_step,
     TEXT_190},
    {VALUE_761, VALUE_762, VALUE_763, VALUE_764, NULL, pirate_a_step, TEXT_191},
    {VALUE_765, VALUE_766, VALUE_767, VALUE_768, NULL, pirate_b_step, TEXT_192},
    {VALUE_769, VALUE_770, VALUE_771, VALUE_772, NULL, pirate_c_step, TEXT_193},
    {VALUE_773, VALUE_774, VALUE_775, VALUE_776, NULL, pirate_s_step, TEXT_194},
    {VALUE_777, VALUE_778, VALUE_779, VALUE_780, NULL, mud_step, TEXT_195},
    {VALUE_781, VALUE_782, VALUE_783, VALUE_784, NULL, launcher_step, TEXT_196},
    {VALUE_785, VALUE_786, VALUE_787, VALUE_788, NULL, water_drops_step,
     TEXT_197},
    {VALUE_789, VALUE_790, VALUE_791, VALUE_792, NULL, flower_al_step,
     TEXT_198},
    {VALUE_793, VALUE_794, VALUE_795, VALUE_796, NULL, flower_bl_step,
     TEXT_199},
    {VALUE_797, VALUE_798, VALUE_799, VALUE_800, NULL, gold_wall_step,
     TEXT_200},
    {VALUE_801, VALUE_802, VALUE_803, VALUE_804, NULL, statue_collision_step,
     TEXT_201},
    {VALUE_805, VALUE_806, VALUE_807, VALUE_808, NULL, uni_a_start, TEXT_202},
    {VALUE_809, VALUE_810, VALUE_811, VALUE_812, NULL, uni_b_start, TEXT_203},
    {VALUE_813, VALUE_814, VALUE_815, VALUE_816, NULL, spina_a_step, TEXT_204},
    {VALUE_817, VALUE_818, VALUE_819, VALUE_820, NULL, spina_b_step, TEXT_205},
    {VALUE_821, VALUE_822, VALUE_823, VALUE_824, NULL, spina_c_step, TEXT_206},
};
