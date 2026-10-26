/* the kinds of object the stage places: each one's task, and its name */
#include "stg03/kinds.h"
#include "collision/invisible.h"
#include "enemy/amenbo.h"
#include "enemy/saru.h"
#include "minicart/ship.h"
#include "object/balloon.h"
#include "object/capsule.h"
#include "object/dash_panel.h"
#include "object/hint.h"
#include "object/iron_ball.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/line_ring.h"
#include "object/line_ring_v.h"
#include "object/panel.h"
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
#include "object/zero_trigger.h"
#include "player/fish.h"
#include "stg01/fish.h"
#include "stg03/arch.h"
#include "stg03/barrel.h"
#include "stg03/bowling.h"
#include "stg03/buyon.h"
#include "stg03/cart.h"
#include "stg03/cart_dash_panel.h"
#include "stg03/cart_jump_panel.h"
#include "stg03/cart_stopper.h"
#include "stg03/catapult.h"
#include "stg03/coaster.h"
#include "stg03/doors.h"
#include "stg03/go_round.h"
#include "stg03/monitor.h"
#include "stg03/panel.h"
#include "stg03/pirates.h"
#include "stg03/pit.h"
#include "stg03/planets.h"
#include "stg03/platforms.h"
#include "stg03/rocket.h"
#include "stg03/roof.h"
#include "stg03/satellite.h"
#include "stg03/scenery.h"
#include "stg03/shutter.h"
#include "stg03/tp_ring.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 14
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 13
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 13
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 11
#define TEXT_20_LENGTH 10
#define TEXT_21_LENGTH 11
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 6
#define TEXT_24_LENGTH 8
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 6
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 17
#define TEXT_29_LENGTH 7
#define TEXT_30_LENGTH 6
#define TEXT_31_LENGTH 7
#define TEXT_32_LENGTH 6
#define TEXT_33_LENGTH 7
#define TEXT_34_LENGTH 6
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 14
#define TEXT_37_LENGTH 19
#define TEXT_38_LENGTH 19
#define TEXT_39_LENGTH 8
#define TEXT_40_LENGTH 8
#define TEXT_41_LENGTH 9
#define TEXT_42_LENGTH 9
#define TEXT_43_LENGTH 13
#define TEXT_44_LENGTH 13
#define TEXT_45_LENGTH 13
#define TEXT_46_LENGTH 9
#define TEXT_47_LENGTH 7
#define TEXT_48_LENGTH 10
#define TEXT_49_LENGTH 10
#define TEXT_50_LENGTH 10
#define TEXT_51_LENGTH 6
#define TEXT_52_LENGTH 8
#define TEXT_53_LENGTH 11
#define TEXT_54_LENGTH 6
#define TEXT_55_LENGTH 9
#define TEXT_56_LENGTH 6
#define TEXT_57_LENGTH 8
#define TEXT_58_LENGTH 12
#define TEXT_59_LENGTH 9
#define TEXT_60_LENGTH 10
#define TEXT_61_LENGTH 10
#define TEXT_62_LENGTH 10
#define TEXT_63_LENGTH 9
#define TEXT_64_LENGTH 6
#define TEXT_65_LENGTH 8
#define TEXT_66_LENGTH 13
#define TEXT_67_LENGTH 9
#define TEXT_68_LENGTH 9
#define TEXT_69_LENGTH 8
#define TEXT_70_LENGTH 8
#define TEXT_71_LENGTH 13
#define TEXT_72_LENGTH 7
#define TEXT_73_LENGTH 7
#define TEXT_74_LENGTH 10
#define TEXT_75_LENGTH 10
#define TEXT_76_LENGTH 8
#define TEXT_77_LENGTH 6
#define TEXT_78_LENGTH 9
#define TEXT_79_LENGTH 5
#define TEXT_80_LENGTH 9
#define TEXT_81_LENGTH 9
#define TEXT_82_LENGTH 9
#define TEXT_83_LENGTH 9
#define TEXT_84_LENGTH 9
#define TEXT_85_LENGTH 9
#define TEXT_86_LENGTH 9
#define TEXT_87_LENGTH 6
#define TEXT_88_LENGTH 6
#define TEXT_89_LENGTH 10
#define TEXT_90_LENGTH 6
#define TEXT_91_LENGTH 8
#define TEXT_92_LENGTH 9
#include "stg03/kinds/text.inc"
#include "stg03/kinds/values.inc"

/* each kind's create flags, level, attr and range come from the disc,
 * through values.inc */
struct object_kind twinkle_object_kinds[TWINKLE_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, ring_group_step, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, line_ring_v_init, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, line_ring_init, TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, spring_step, TEXT_5},
    {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, spring_b_step, TEXT_6},
    {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, spring_h_start, TEXT_7},
    {VALUE_29, VALUE_30, VALUE_31, VALUE_32, NULL, dash_panel_step, TEXT_8},
    {VALUE_33, VALUE_34, VALUE_35, VALUE_36, NULL, panel_start, TEXT_9},
    {VALUE_37, VALUE_38, VALUE_39, VALUE_40, NULL, iron_ball_init, TEXT_10},
    {VALUE_41, VALUE_42, VALUE_43, VALUE_44, NULL, iron_ball_jg_init, TEXT_11},
    {VALUE_45, VALUE_46, VALUE_47, VALUE_48, NULL, toge_init, TEXT_12},
    {VALUE_49, VALUE_50, VALUE_51, VALUE_52, NULL, capsule_start, TEXT_13},
    {VALUE_53, VALUE_54, VALUE_55, VALUE_56, NULL, switch_step, TEXT_14},
    {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, balloon_step, TEXT_15},
    {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, item_box_step, TEXT_16},
    {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, item_box_air_step, TEXT_17},
    {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, rocket_h_step, TEXT_18},
    {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, rocket_hs_step, TEXT_19},
    {VALUE_77, VALUE_78, VALUE_79, VALUE_80, NULL, rocket_v_step, TEXT_20},
    {VALUE_81, VALUE_82, VALUE_83, VALUE_84, NULL, rocket_vs_step, TEXT_21},
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, savepoint_step, TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, invisible_wall_step,
     TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, invisible_sphere_step,
     TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, invisible_cylinder_step,
     TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, invisible_box_step,
     TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, ottotto_step, TEXT_27},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, invisible_dry_step,
     TEXT_28},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, tikal_init, TEXT_29},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, hint_init, TEXT_30},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, amenbo_start, TEXT_31},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, cart_step, TEXT_32},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, buyon_step, TEXT_33},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, saru_start, TEXT_34},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL, zero_trigger_start,
     TEXT_35},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL, cart_stopper_start,
     TEXT_36},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL, cart_dash_panel_step,
     TEXT_37},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL, cart_jump_panel_step,
     TEXT_38},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL, twinkle_arch_1_step,
     TEXT_39},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL, twinkle_arch_2_step,
     TEXT_40},
    {VALUE_161, VALUE_162, VALUE_163, VALUE_164, NULL, twinkle_light_1_step,
     TEXT_41},
    {VALUE_165, VALUE_166, VALUE_167, VALUE_168, NULL, twinkle_light_2_step,
     TEXT_42},
    {VALUE_169, VALUE_170, VALUE_171, VALUE_172, NULL, meteorite_1_step,
     TEXT_43},
    {VALUE_173, VALUE_174, VALUE_175, VALUE_176, NULL, meteorite_2_step,
     TEXT_44},
    {VALUE_177, VALUE_178, VALUE_179, VALUE_180, NULL, meteorite_3_step,
     TEXT_45},
    {VALUE_181, VALUE_182, VALUE_183, VALUE_184, NULL, monitor_step, TEXT_46},
    {VALUE_185, VALUE_186, VALUE_187, VALUE_188, NULL, twinkle_panel_step,
     TEXT_47},
    {VALUE_189, VALUE_190, VALUE_191, VALUE_192, NULL, planet_1_step, TEXT_48},
    {VALUE_193, VALUE_194, VALUE_195, VALUE_196, NULL, planet_2_step, TEXT_49},
    {VALUE_197, VALUE_198, VALUE_199, VALUE_200, NULL, planet_3_step, TEXT_50},
    {VALUE_201, VALUE_202, VALUE_203, VALUE_204, NULL, twinkle_pole_step,
     TEXT_51},
    {VALUE_205, VALUE_206, VALUE_207, VALUE_208, NULL, twinkle_rocket_step,
     TEXT_52},
    {VALUE_209, VALUE_210, VALUE_211, VALUE_212, NULL, satellite_step, TEXT_53},
    {VALUE_213, VALUE_214, VALUE_215, VALUE_216, NULL, ship_step, TEXT_54},
    {VALUE_217, VALUE_218, VALUE_219, VALUE_220, NULL, twinkle_shutter_step,
     TEXT_55},
    {VALUE_221, VALUE_222, VALUE_223, VALUE_224, NULL, bowling_ball_step,
     TEXT_56},
    {VALUE_225, VALUE_226, VALUE_227, VALUE_228, NULL, barrel_step, TEXT_57},
    {VALUE_229, VALUE_230, VALUE_231, VALUE_232, NULL, twinkle_bow_window_step,
     TEXT_58},
    {VALUE_233, VALUE_234, VALUE_235, VALUE_236, NULL, bowling_step, TEXT_59},
    {VALUE_237, VALUE_238, VALUE_239, VALUE_240, NULL, twinkle_candle_1_step,
     TEXT_60},
    {VALUE_241, VALUE_242, VALUE_243, VALUE_244, NULL, twinkle_candle_2_step,
     TEXT_61},
    {VALUE_245, VALUE_246, VALUE_247, VALUE_248, NULL, catapult_step, TEXT_62},
    {VALUE_249, VALUE_250, VALUE_251, VALUE_252, NULL, twinkle_coaster_step,
     TEXT_63},
    {VALUE_253, VALUE_254, VALUE_255, VALUE_256, NULL, door_step, TEXT_64},
    {VALUE_257, VALUE_258, VALUE_259, VALUE_260, NULL, door_b_step, TEXT_65},
    {VALUE_261, VALUE_262, VALUE_263, VALUE_264, NULL, double_door_step,
     TEXT_66},
    {VALUE_265, VALUE_266, VALUE_267, VALUE_268, NULL, twinkle_fence_1_step,
     TEXT_67},
    {VALUE_269, VALUE_270, VALUE_271, VALUE_272, NULL, twinkle_fence_2_step,
     TEXT_68},
    {VALUE_273, VALUE_274, VALUE_275, VALUE_276, NULL, twinkle_flag_1_step,
     TEXT_69},
    {VALUE_277, VALUE_278, VALUE_279, VALUE_280, NULL, twinkle_flag_2_step,
     TEXT_70},
    {VALUE_281, VALUE_282, VALUE_283, VALUE_284, NULL, twinkle_flag_lamp_step,
     TEXT_71},
    {VALUE_285, VALUE_286, VALUE_287, VALUE_288, NULL, twinkle_float_step,
     TEXT_72},
    {VALUE_289, VALUE_290, VALUE_291, VALUE_292, NULL, twinkle_flyer_step,
     TEXT_73},
    {VALUE_293, VALUE_294, VALUE_295, VALUE_296, NULL, twinkle_foothold_step,
     TEXT_74},
    {VALUE_297, VALUE_298, VALUE_299, VALUE_300, NULL, go_round_step, TEXT_75},
    {VALUE_301, VALUE_302, VALUE_303, VALUE_304, NULL, twinkle_girder_step,
     TEXT_76},
    {VALUE_305, VALUE_306, VALUE_307, VALUE_308, NULL, twinkle_lamp_step,
     TEXT_77},
    {VALUE_309, VALUE_310, VALUE_311, VALUE_312, NULL, pirates_step, TEXT_78},
    {VALUE_313, VALUE_314, VALUE_315, VALUE_316, NULL, pit_step, TEXT_79},
    {VALUE_317, VALUE_318, VALUE_319, VALUE_320, NULL, twinkle_plant_1_step,
     TEXT_80},
    {VALUE_321, VALUE_322, VALUE_323, VALUE_324, NULL, twinkle_plant_2_step,
     TEXT_81},
    {VALUE_325, VALUE_326, VALUE_327, VALUE_328, NULL, twinkle_plant_3_step,
     TEXT_82},
    {VALUE_329, VALUE_330, VALUE_331, VALUE_332, NULL, twinkle_plant_4_step,
     TEXT_83},
    {VALUE_333, VALUE_334, VALUE_335, VALUE_336, NULL, twinkle_plant_5_step,
     TEXT_84},
    {VALUE_337, VALUE_338, VALUE_339, VALUE_340, NULL, twinkle_plant_6_step,
     TEXT_85},
    {VALUE_341, VALUE_342, VALUE_343, VALUE_344, NULL, tp_ring_step, TEXT_86},
    {VALUE_345, VALUE_346, VALUE_347, VALUE_348, NULL, roof_step, TEXT_87},
    {VALUE_349, VALUE_350, VALUE_351, VALUE_352, NULL, fish_bass_step, TEXT_88},
    {VALUE_353, VALUE_354, VALUE_355, VALUE_356, NULL, frog_step, TEXT_89},
    {VALUE_357, VALUE_358, VALUE_359, VALUE_360, NULL, fish_generator_step,
     TEXT_90},
    {VALUE_361, VALUE_362, VALUE_363, VALUE_364, NULL, bubble_init, TEXT_91},
    {VALUE_365, VALUE_366, VALUE_367, VALUE_368, NULL, bubble_stream_init,
     TEXT_92},
};
