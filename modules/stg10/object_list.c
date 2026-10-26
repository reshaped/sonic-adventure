/* the kinds of object the stage places, their names, and the list that
 * counts them */
#include "stg10/object_list.h"
#include "collision/invisible.h"
#include "enemy/hiyoko.h"
#include "enemy/robo.h"
#include "enemy/spina.h"
#include "object/balloon.h"
#include "object/capsule.h"
#include "object/dash_panel.h"
#include "object/dash_ring.h"
#include "object/iron_ball.h"
#include "object/item_box.h"
#include "object/kusa.h"
#include "object/panel.h"
#include "object/piece.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/rocket.h"
#include "object/savepoint.h"
#include "object/spring.h"
#include "object/switch.h"
#include "object/tikal.h"
#include "object/toge_bubble.h"
#include "object/zero_trigger.h"
#include "stage/scene_changer.h"
#include "stg10/amy_drum.h"
#include "stg10/arm_wall.h"
#include "stg10/big_light.h"
#include "stg10/blue_light.h"
#include "stg10/clear_trigger.h"
#include "stg10/color_door.h"
#include "stg10/container.h"
#include "stg10/conveyor_belt.h"
#include "stg10/conveyor_stop.h"
#include "stg10/discharge_lamp.h"
#include "stg10/double_gate.h"
#include "stg10/drum.h"
#include "stg10/egg_sign.h"
#include "stg10/elastic_fence.h"
#include "stg10/elevator1.h"
#include "stg10/elevator2.h"
#include "stg10/fan.h"
#include "stg10/floating_island.h"
#include "stg10/gacha_machine.h"
#include "stg10/gacha_robot.h"
#include "stg10/gate_a.h"
#include "stg10/gate_b.h"
#include "stg10/gate_c.h"
#include "stg10/gate_d.h"
#include "stg10/gate_e.h"
#include "stg10/gate_f.h"
#include "stg10/gate_g.h"
#include "stg10/hall_gate.h"
#include "stg10/hammer.h"
#include "stg10/intake.h"
#include "stg10/ladder.h"
#include "stg10/lamp.h"
#include "stg10/light1.h"
#include "stg10/light2.h"
#include "stg10/light_camera.h"
#include "stg10/outlet.h"
#include "stg10/pursuit_camera.h"
#include "stg10/robo_shutter.h"
#include "stg10/robo_wall.h"
#include "stg10/rotating_door.h"
#include "stg10/set_step.h"
#include "stg10/shutter.h"
#include "stg10/side_arm.h"
#include "stg10/side_arm_fixed.h"
#include "stg10/spin_tube_big.h"
#include "stg10/spin_tube_medium.h"
#include "stg10/spin_tube_small.h"
#include "stg10/stand_light.h"
#include "stg10/target.h"
#include "stg10/texture.h"
#include "stg10/thorn_roller.h"
#include "stg10/up_arm.h"
#include "stg10/up_arm_fixed.h"
#include "stg10/updraft.h"
#include "stg10/vertical_pipe.h"
#include "stg10/zero_box.h"

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
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 10
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 7
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 12
#define TEXT_30_LENGTH 14
#define TEXT_31_LENGTH 11
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 12
#define TEXT_34_LENGTH 12
#define TEXT_35_LENGTH 11
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 12
#define TEXT_38_LENGTH 12
#define TEXT_39_LENGTH 12
#define TEXT_40_LENGTH 8
#define TEXT_41_LENGTH 9
#define TEXT_42_LENGTH 13
#define TEXT_43_LENGTH 14
#define TEXT_44_LENGTH 12
#define TEXT_45_LENGTH 8
#define TEXT_46_LENGTH 8
#define TEXT_47_LENGTH 8
#define TEXT_48_LENGTH 8
#define TEXT_49_LENGTH 8
#define TEXT_50_LENGTH 8
#define TEXT_51_LENGTH 8
#define TEXT_52_LENGTH 13
#define TEXT_53_LENGTH 11
#define TEXT_54_LENGTH 14
#define TEXT_55_LENGTH 6
#define TEXT_56_LENGTH 10
#define TEXT_57_LENGTH 7
#define TEXT_58_LENGTH 7
#define TEXT_59_LENGTH 11
#define TEXT_60_LENGTH 9
#define TEXT_61_LENGTH 11
#define TEXT_62_LENGTH 10
#define TEXT_63_LENGTH 11
#define TEXT_64_LENGTH 5
#define TEXT_65_LENGTH 8
#define TEXT_66_LENGTH 9
#define TEXT_67_LENGTH 11
#define TEXT_68_LENGTH 11
#define TEXT_69_LENGTH 12
#define TEXT_70_LENGTH 12
#define TEXT_71_LENGTH 10
#define TEXT_72_LENGTH 11
#define TEXT_73_LENGTH 13
#define TEXT_74_LENGTH 12
#define TEXT_75_LENGTH 8
#define TEXT_76_LENGTH 11
#define TEXT_77_LENGTH 9
#define TEXT_78_LENGTH 10
#define TEXT_79_LENGTH 12
#define TEXT_80_LENGTH 10
#define TEXT_81_LENGTH 10
#define TEXT_82_LENGTH 11
#define TEXT_83_LENGTH 9
#define TEXT_84_LENGTH 6
#define TEXT_85_LENGTH 10
#define TEXT_86_LENGTH 13
#define TEXT_87_LENGTH 7
#define TEXT_88_LENGTH 8
#define TEXT_89_LENGTH 10
#define TEXT_90_LENGTH 6
#define TEXT_91_LENGTH 7
#define TEXT_92_LENGTH 7
#define TEXT_93_LENGTH 14
#define TEXT_94_LENGTH 8
#define TEXT_95_LENGTH 8
#define TEXT_96_LENGTH 10
#define TEXT_97_LENGTH 9
#include "stg10/object_list/text.inc"
#include "stg10/object_list/values.inc"

/* each kind's create flags, level, attr and range are values the build reads
 * from the disc. no kind here has a param */
static struct object_kind final_object_kinds[97] = {
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
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, hiyoko_step, TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, spina_a_step, TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, spina_b_step, TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, spina_c_step, TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, robo_init, TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, up_arm_step, TEXT_27},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, side_arm_step, TEXT_28},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, up_arm_fixed_step,
     TEXT_29},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, side_arm_fixed_step,
     TEXT_30},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, final_container_step,
     TEXT_31},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, set_step_step, TEXT_32},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, thorn_roller_step,
     TEXT_33},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, discharge_lamp_step,
     TEXT_34},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL, conveyor_belt_step,
     TEXT_35},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL, conveyor_stop_step,
     TEXT_36},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL, spin_tube_big_step,
     TEXT_37},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL, spin_tube_medium_step,
     TEXT_38},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL, spin_tube_small_step,
     TEXT_39},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL, hammer_step, TEXT_40},
    {VALUE_161, VALUE_162, VALUE_163, VALUE_164, NULL, rotating_door_step,
     TEXT_41},
    {VALUE_165, VALUE_166, VALUE_167, VALUE_168, NULL, pursuit_camera_step,
     TEXT_42},
    {VALUE_169, VALUE_170, VALUE_171, VALUE_172, NULL, light_camera_step,
     TEXT_43},
    {VALUE_173, VALUE_174, VALUE_175, VALUE_176, NULL, blue_light_step,
     TEXT_44},
    {VALUE_177, VALUE_178, VALUE_179, VALUE_180, NULL, gate_a_step, TEXT_45},
    {VALUE_181, VALUE_182, VALUE_183, VALUE_184, NULL, gate_b_step, TEXT_46},
    {VALUE_185, VALUE_186, VALUE_187, VALUE_188, NULL, gate_c_step, TEXT_47},
    {VALUE_189, VALUE_190, VALUE_191, VALUE_192, NULL, gate_d_step, TEXT_48},
    {VALUE_193, VALUE_194, VALUE_195, VALUE_196, NULL, gate_e_step, TEXT_49},
    {VALUE_197, VALUE_198, VALUE_199, VALUE_200, NULL, gate_f_step, TEXT_50},
    {VALUE_201, VALUE_202, VALUE_203, VALUE_204, NULL, gate_g_step, TEXT_51},
    {VALUE_205, VALUE_206, VALUE_207, VALUE_208, NULL, double_gate_step,
     TEXT_52},
    {VALUE_209, VALUE_210, VALUE_211, VALUE_212, NULL, robo_wall_step, TEXT_53},
    {VALUE_213, VALUE_214, VALUE_215, VALUE_216, NULL, robo_shutter_step,
     TEXT_54},
    {VALUE_217, VALUE_218, VALUE_219, VALUE_220, NULL, final_drum_step,
     TEXT_55},
    {VALUE_221, VALUE_222, VALUE_223, VALUE_224, NULL, arm_wall_step, TEXT_56},
    {VALUE_225, VALUE_226, VALUE_227, VALUE_228, NULL, gacha_robot_step,
     TEXT_57},
    {VALUE_229, VALUE_230, VALUE_231, VALUE_232, NULL, gacha_machine_step,
     TEXT_58},
    {VALUE_233, VALUE_234, VALUE_235, VALUE_236, NULL, gacha_wall_step,
     TEXT_59},
    {VALUE_237, VALUE_238, VALUE_239, VALUE_240, NULL, intake_step, TEXT_60},
    {VALUE_241, VALUE_242, VALUE_243, VALUE_244, NULL, outlet_step, TEXT_61},
    {VALUE_245, VALUE_246, VALUE_247, VALUE_248, NULL, floating_island_step,
     TEXT_62},
    {VALUE_249, VALUE_250, VALUE_251, VALUE_252, NULL, big_light_step, TEXT_63},
    {VALUE_253, VALUE_254, VALUE_255, VALUE_256, NULL, updraft_step, TEXT_64},
    {VALUE_257, VALUE_258, VALUE_259, VALUE_260, NULL, moving_target_step,
     TEXT_65},
    {VALUE_261, VALUE_262, VALUE_263, VALUE_264, NULL, final_texture_panel_step,
     TEXT_66},
    {VALUE_265, VALUE_266, VALUE_267, VALUE_268, NULL, elevator1_step, TEXT_67},
    {VALUE_269, VALUE_270, VALUE_271, VALUE_272, NULL, hall_gate_step, TEXT_68},
    {VALUE_273, VALUE_274, VALUE_275, VALUE_276, NULL, pop_up_target_step,
     TEXT_69},
    {VALUE_277, VALUE_278, VALUE_279, VALUE_280, NULL, target_box_step,
     TEXT_70},
    {VALUE_281, VALUE_282, VALUE_283, VALUE_284, NULL, pin_lamp_step, TEXT_71},
    {VALUE_285, VALUE_286, VALUE_287, VALUE_288, NULL, side_lamp_step, TEXT_72},
    {VALUE_289, VALUE_290, VALUE_291, VALUE_292, NULL, stand_light_step,
     TEXT_73},
    {VALUE_293, VALUE_294, VALUE_295, VALUE_296, NULL, egg_sign_step, TEXT_74},
    {VALUE_297, VALUE_298, VALUE_299, VALUE_300, NULL, final_ladder_start,
     TEXT_75},
    {VALUE_301, VALUE_302, VALUE_303, VALUE_304, NULL, elevator2_step, TEXT_76},
    {VALUE_305, VALUE_306, VALUE_307, VALUE_308, NULL, vertical_pipe_step,
     TEXT_77},
    {VALUE_309, VALUE_310, VALUE_311, VALUE_312, NULL, final_amy_drum_step,
     TEXT_78},
    {VALUE_313, VALUE_314, VALUE_315, VALUE_316, NULL, color_door_step,
     TEXT_79},
    {VALUE_317, VALUE_318, VALUE_319, VALUE_320, NULL, fake_amy_step, TEXT_80},
    {VALUE_321, VALUE_322, VALUE_323, VALUE_324, NULL, zero_appear_box_step,
     TEXT_81},
    {VALUE_325, VALUE_326, VALUE_327, VALUE_328, NULL, zero_kill_box_step,
     TEXT_82},
    {VALUE_329, VALUE_330, VALUE_331, VALUE_332, NULL, shutter_step, TEXT_83},
    {VALUE_333, VALUE_334, VALUE_335, VALUE_336, NULL, final_fan_step, TEXT_84},
    {VALUE_337, VALUE_338, VALUE_339, VALUE_340, NULL, gacha_shooter_step,
     TEXT_85},
    {VALUE_341, VALUE_342, VALUE_343, VALUE_344, NULL, color_door2_step,
     TEXT_86},
    {VALUE_345, VALUE_346, VALUE_347, VALUE_348, NULL, elastic_fence_start,
     TEXT_87},
    {VALUE_349, VALUE_350, VALUE_351, VALUE_352, NULL, invisible_sphere_step,
     TEXT_88},
    {VALUE_353, VALUE_354, VALUE_355, VALUE_356, NULL, invisible_cylinder_step,
     TEXT_89},
    {VALUE_357, VALUE_358, VALUE_359, VALUE_360, NULL, invisible_box_step,
     TEXT_90},
    {VALUE_361, VALUE_362, VALUE_363, VALUE_364, NULL, ottotto_step, TEXT_91},
    {VALUE_365, VALUE_366, VALUE_367, VALUE_368, NULL, tikal_init, TEXT_92},
    {VALUE_369, VALUE_370, VALUE_371, VALUE_372, NULL, scene_changer_init,
     TEXT_93},
    {VALUE_373, VALUE_374, VALUE_375, VALUE_376, NULL, light1_step, TEXT_94},
    {VALUE_377, VALUE_378, VALUE_379, VALUE_380, NULL, light2_step, TEXT_95},
    {VALUE_381, VALUE_382, VALUE_383, VALUE_384, NULL, zero_trigger_start,
     TEXT_96},
    {VALUE_385, VALUE_386, VALUE_387, VALUE_388, NULL, final_clear_trigger_step,
     TEXT_97},
};
struct object_list final_object_list = {
#include "stg10/object_list/final_object_list.inc"
};
