/* the kinds of object Hot Shelter's act 1 can place, their names, and the
 * list that counts them */
#include "stg12/act1_object_list.h"
#include "collision/invisible.h"
#include "enemy/leon.h"
#include "enemy/robo.h"
#include "enemy/saru.h"
#include "object/balloon.h"
#include "object/capsule.h"
#include "object/dash_panel.h"
#include "object/dash_ring.h"
#include "object/hint.h"
#include "object/iron_ball.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/kusa.h"
#include "object/panel.h"
#include "object/piece.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/rocket.h"
#include "object/savepoint.h"
#include "object/spring.h"
#include "object/switch.h"
#include "object/target.h"
#include "object/tikal.h"
#include "object/toge_bubble.h"
#include "object/zero_trigger.h"
#include "stage/scene_changer.h"
#include "stg12/banji.h"
#include "stg12/blue_light.h"
#include "stg12/box.h"
#include "stg12/box_hole.h"
#include "stg12/box_switch.h"
#include "stg12/bridge.h"
#include "stg12/chimney.h"
#include "stg12/computer.h"
#include "stg12/container.h"
#include "stg12/crash_tv.h"
#include "stg12/daruma.h"
#include "stg12/drum_can.h"
#include "stg12/dryer.h"
#include "stg12/elevator.h"
#include "stg12/fence.h"
#include "stg12/floating_island.h"
#include "stg12/gate_1.h"
#include "stg12/gate_2.h"
#include "stg12/gate_side.h"
#include "stg12/gate_switch.h"
#include "stg12/gear_1.h"
#include "stg12/gear_2.h"
#include "stg12/gear_3.h"
#include "stg12/gear_4.h"
#include "stg12/gear_5.h"
#include "stg12/gear_6.h"
#include "stg12/glow.h"
#include "stg12/handle.h"
#include "stg12/handle_lift.h"
#include "stg12/handle_meter.h"
#include "stg12/ladder.h"
#include "stg12/light_1.h"
#include "stg12/light_2.h"
#include "stg12/light_3.h"
#include "stg12/light_4.h"
#include "stg12/lighting.h"
#include "stg12/ornament_1.h"
#include "stg12/ornament_2.h"
#include "stg12/propeller.h"
#include "stg12/robo_tv.h"
#include "stg12/spark.h"
#include "stg12/stairs.h"
#include "stg12/sub_tv.h"
#include "stg12/support_1a.h"
#include "stg12/support_1b.h"
#include "stg12/support_1c.h"
#include "stg12/support_1d.h"
#include "stg12/switch.h"
#include "stg12/toilet_door.h"
#include "stg12/turntable.h"
#include "stg12/vent.h"
#include "stg12/wc_mark_1.h"
#include "stg12/wc_mark_2.h"

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
#define TEXT_22_LENGTH 7
#define TEXT_23_LENGTH 8
#define TEXT_24_LENGTH 14
#define TEXT_25_LENGTH 12
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 7
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 8
#define TEXT_30_LENGTH 8
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 6
#define TEXT_33_LENGTH 7
#define TEXT_34_LENGTH 6
#define TEXT_35_LENGTH 6
#define TEXT_36_LENGTH 6
#define TEXT_37_LENGTH 9
#define TEXT_38_LENGTH 8
#define TEXT_39_LENGTH 8
#define TEXT_40_LENGTH 8
#define TEXT_41_LENGTH 11
#define TEXT_42_LENGTH 11
#define TEXT_43_LENGTH 13
#define TEXT_44_LENGTH 10
#define TEXT_45_LENGTH 7
#define TEXT_46_LENGTH 10
#define TEXT_47_LENGTH 8
#define TEXT_48_LENGTH 9
#define TEXT_49_LENGTH 9
#define TEXT_50_LENGTH 9
#define TEXT_51_LENGTH 9
#define TEXT_52_LENGTH 11
#define TEXT_53_LENGTH 11
#define TEXT_54_LENGTH 11
#define TEXT_55_LENGTH 11
#define TEXT_56_LENGTH 11
#define TEXT_57_LENGTH 11
#define TEXT_58_LENGTH 7
#define TEXT_59_LENGTH 9
#define TEXT_60_LENGTH 11
#define TEXT_61_LENGTH 8
#define TEXT_62_LENGTH 9
#define TEXT_63_LENGTH 9
#define TEXT_64_LENGTH 9
#define TEXT_65_LENGTH 9
#define TEXT_66_LENGTH 10
#define TEXT_67_LENGTH 11
#define TEXT_68_LENGTH 12
#define TEXT_69_LENGTH 8
#define TEXT_70_LENGTH 6
#define TEXT_71_LENGTH 9
#define TEXT_72_LENGTH 8
#define TEXT_73_LENGTH 10
#define TEXT_74_LENGTH 11
#define TEXT_75_LENGTH 8
#define TEXT_76_LENGTH 8
#define TEXT_77_LENGTH 6
#define TEXT_78_LENGTH 7
#define TEXT_79_LENGTH 9
#define TEXT_80_LENGTH 9
#define TEXT_81_LENGTH 14
#define TEXT_82_LENGTH 6
#define TEXT_83_LENGTH 9
#define TEXT_84_LENGTH 7
#define TEXT_85_LENGTH 8
#define TEXT_86_LENGTH 9
#define TEXT_87_LENGTH 10
#define TEXT_88_LENGTH 9
#define TEXT_89_LENGTH 12
#include "stg12/act1_object_list/text.inc"
#include "stg12/act1_object_list/values.inc"

/* each kind's create flags, level, attr and range are values the build reads
 * from the disc */
static struct object_kind shelter_act1_object_kinds[89] = {
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
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, bubble_stream_init, TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, target_start, TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, scene_changer_init, TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, item_box_air_step, TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, zero_trigger_start,
     TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, tikal_init, TEXT_27},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, hint_init, TEXT_28},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, shelter_switch_step,
     TEXT_29},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, invisible_sphere_step,
     TEXT_30},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, invisible_cylinder_step,
     TEXT_31},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, invisible_box_step,
     TEXT_32},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, ottotto_step, TEXT_33},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, saru_start, TEXT_34},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL, robo_init, TEXT_35},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL, leon_start, TEXT_36},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL, chimney_step, TEXT_37},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL, shelter_light_1_step,
     TEXT_38},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL, shelter_light_2_step,
     TEXT_39},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL, shelter_light_3_step,
     TEXT_40},
    {VALUE_161, VALUE_162, VALUE_163, VALUE_164, NULL, shelter_container_step,
     TEXT_41},
    {VALUE_165, VALUE_166, VALUE_167, VALUE_168, NULL, handle_step, TEXT_42},
    {VALUE_169, VALUE_170, VALUE_171, VALUE_172, NULL, handle_meter_step,
     TEXT_43},
    {VALUE_173, VALUE_174, VALUE_175, VALUE_176, NULL, shelter_spark_step,
     TEXT_44},
    {VALUE_177, VALUE_178, VALUE_179, VALUE_180, NULL, gate_1_step, TEXT_45},
    {VALUE_181, VALUE_182, VALUE_183, VALUE_184, NULL, shelter_elevator_step,
     TEXT_46},
    {VALUE_185, VALUE_186, VALUE_187, VALUE_188, NULL, shelter_bridge_step,
     TEXT_47},
    {VALUE_189, VALUE_190, VALUE_191, VALUE_192, NULL, support_1a_step,
     TEXT_48},
    {VALUE_193, VALUE_194, VALUE_195, VALUE_196, NULL, support_1b_step,
     TEXT_49},
    {VALUE_197, VALUE_198, VALUE_199, VALUE_200, NULL, support_1c_step,
     TEXT_50},
    {VALUE_201, VALUE_202, VALUE_203, VALUE_204, NULL, support_1d_step,
     TEXT_51},
    {VALUE_205, VALUE_206, VALUE_207, VALUE_208, NULL, gear_1_step, TEXT_52},
    {VALUE_209, VALUE_210, VALUE_211, VALUE_212, NULL, gear_2_step, TEXT_53},
    {VALUE_213, VALUE_214, VALUE_215, VALUE_216, NULL, gear_3_step, TEXT_54},
    {VALUE_217, VALUE_218, VALUE_219, VALUE_220, NULL, gear_4_step, TEXT_55},
    {VALUE_221, VALUE_222, VALUE_223, VALUE_224, NULL, gear_5_step, TEXT_56},
    {VALUE_225, VALUE_226, VALUE_227, VALUE_228, NULL, gear_6_step, TEXT_57},
    {VALUE_229, VALUE_230, VALUE_231, VALUE_232, NULL, gate_2_step, TEXT_58},
    {VALUE_233, VALUE_234, VALUE_235, VALUE_236, NULL, vent_step, TEXT_59},
    {VALUE_237, VALUE_238, VALUE_239, VALUE_240, NULL, shelter_propeller_step,
     TEXT_60},
    {VALUE_241, VALUE_242, VALUE_243, VALUE_244, NULL, stairs_step, TEXT_61},
    {VALUE_245, VALUE_246, VALUE_247, VALUE_248, NULL, drum_can_step, TEXT_62},
    {VALUE_249, VALUE_250, VALUE_251, VALUE_252, NULL, ornament_1_step,
     TEXT_63},
    {VALUE_253, VALUE_254, VALUE_255, VALUE_256, NULL, ornament_2_step,
     TEXT_64},
    {VALUE_257, VALUE_258, VALUE_259, VALUE_260, NULL, lighting_step, TEXT_65},
    {VALUE_261, VALUE_262, VALUE_263, VALUE_264, NULL, computer_step, TEXT_66},
    {VALUE_265, VALUE_266, VALUE_267, VALUE_268, NULL, turntable_step, TEXT_67},
    {VALUE_269, VALUE_270, VALUE_271, VALUE_272, NULL, toilet_door_step,
     TEXT_68},
    {VALUE_273, VALUE_274, VALUE_275, VALUE_276, NULL, daruma_step, TEXT_69},
    {VALUE_277, VALUE_278, VALUE_279, VALUE_280, NULL, shelter_box_step,
     TEXT_70},
    {VALUE_281, VALUE_282, VALUE_283, VALUE_284, NULL, floating_island_step,
     TEXT_71},
    {VALUE_285, VALUE_286, VALUE_287, VALUE_288, NULL, shelter_ladder_start,
     TEXT_72},
    {VALUE_289, VALUE_290, VALUE_291, VALUE_292, NULL, gate_side_step, TEXT_73},
    {VALUE_293, VALUE_294, VALUE_295, VALUE_296, NULL, shelter_blue_light_step,
     TEXT_74},
    {VALUE_297, VALUE_298, VALUE_299, VALUE_300, NULL, shelter_light_4_step,
     TEXT_75},
    {VALUE_301, VALUE_302, VALUE_303, VALUE_304, NULL, shelter_glow_step,
     TEXT_76},
    {VALUE_305, VALUE_306, VALUE_307, VALUE_308, NULL, shelter_fence_step,
     TEXT_77},
    {VALUE_309, VALUE_310, VALUE_311, VALUE_312, NULL, dryer_step, TEXT_78},
    {VALUE_313, VALUE_314, VALUE_315, VALUE_316, NULL, wc_mark_1_step, TEXT_79},
    {VALUE_317, VALUE_318, VALUE_319, VALUE_320, NULL, wc_mark_2_step, TEXT_80},
    {VALUE_321, VALUE_322, VALUE_323, VALUE_324, NULL, handle_lift_step,
     TEXT_81},
    {VALUE_325, VALUE_326, VALUE_327, VALUE_328, NULL, gate_13_step, TEXT_82},
    {VALUE_329, VALUE_330, VALUE_331, VALUE_332, NULL, gate_switch_step,
     TEXT_83},
    {VALUE_333, VALUE_334, VALUE_335, VALUE_336, NULL, banji_step, TEXT_84},
    {VALUE_337, VALUE_338, VALUE_339, VALUE_340, NULL, sub_tv_step, TEXT_85},
    {VALUE_341, VALUE_342, VALUE_343, VALUE_344, NULL, robo_tv_step, TEXT_86},
    {VALUE_345, VALUE_346, VALUE_347, VALUE_348, NULL, crash_tv_step, TEXT_87},
    {VALUE_349, VALUE_350, VALUE_351, VALUE_352, NULL, box_hole_step, TEXT_88},
    {VALUE_353, VALUE_354, VALUE_355, VALUE_356, NULL, box_switch_step,
     TEXT_89},
};
struct object_list shelter_act1_object_list = {
#include "stg12/act1_object_list/shelter_act1_object_list.inc"
};
