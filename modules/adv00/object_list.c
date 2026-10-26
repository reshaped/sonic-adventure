/* the kinds of object Station Square places, their names, and the list that
 * counts them */
#include "adv00/object_list.h"
#include "adv00/back_door.h"
#include "adv00/barricade.h"
#include "adv00/boat.h"
#include "adv00/casino_button.h"
#include "adv00/casino_door.h"
#include "adv00/chao_card.h"
#include "adv00/clock.h"
#include "adv00/container.h"
#include "adv00/crowd.h"
#include "adv00/egg_stand.h"
#include "adv00/eggs.h"
#include "adv00/elevator_box.h"
#include "adv00/elevator_sign.h"
#include "adv00/elevators.h"
#include "adv00/employee_card.h"
#include "adv00/extinguisher.h"
#include "adv00/hidden_door.h"
#include "adv00/highway_shutter.h"
#include "adv00/hotel_doors.h"
#include "adv00/ice_stone.h"
#include "adv00/kart_changer.h"
#include "adv00/kart_door.h"
#include "adv00/kart_pass.h"
#include "adv00/kart_sign.h"
#include "adv00/key_block.h"
#include "adv00/light_changer.h"
#include "adv00/mannequin.h"
#include "adv00/office_barricade.h"
#include "adv00/office_door.h"
#include "adv00/parasol.h"
#include "adv00/parked_car.h"
#include "adv00/planets.h"
#include "adv00/pole.h"
#include "adv00/pool_chair.h"
#include "adv00/power_box.h"
#include "adv00/props.h"
#include "adv00/scene_change.h"
#include "adv00/shop1_chairs.h"
#include "adv00/shop1_door.h"
#include "adv00/shop1_lamp.h"
#include "adv00/shop1_salt.h"
#include "adv00/shop1_sign.h"
#include "adv00/shop2_blue_robot.h"
#include "adv00/shop2_box.h"
#include "adv00/shop2_box_lid.h"
#include "adv00/shop2_door.h"
#include "adv00/shop2_fence.h"
#include "adv00/shop2_fossil.h"
#include "adv00/shop2_frame.h"
#include "adv00/shop2_radio.h"
#include "adv00/shop2_robot.h"
#include "adv00/signal.h"
#include "adv00/station_back.h"
#include "adv00/station_door.h"
#include "adv00/street_light.h"
#include "adv00/target_fence.h"
#include "adv00/train.h"
#include "adv00/twinkle_a_door.h"
#include "adv00/twinkle_b_door.h"
#include "adv00/twinkle_door.h"
#include "adv00/twinkle_elevator.h"
#include "adv00/twinkle_handrail.h"
#include "adv00/twinkle_sign.h"
#include "collision/invisible.h"
#include "object/balloon.h"
#include "object/capsule.h"
#include "object/dash_panel.h"
#include "object/dash_ring.h"
#include "object/hint.h"
#include "object/iron_ball.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/kusa.h"
#include "object/level_item.h"
#include "object/line_ring.h"
#include "object/line_ring_v.h"
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
#include "progress/emblem.h"

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
#define TEXT_15_LENGTH 5
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 12
#define TEXT_19_LENGTH 7
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 7
#define TEXT_24_LENGTH 6
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 9
#define TEXT_30_LENGTH 9
#define TEXT_31_LENGTH 9
#define TEXT_32_LENGTH 10
#define TEXT_33_LENGTH 11
#define TEXT_34_LENGTH 12
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 12
#define TEXT_38_LENGTH 12
#define TEXT_39_LENGTH 12
#define TEXT_40_LENGTH 15
#define TEXT_41_LENGTH 12
#define TEXT_42_LENGTH 8
#define TEXT_43_LENGTH 8
#define TEXT_44_LENGTH 8
#define TEXT_45_LENGTH 8
#define TEXT_46_LENGTH 8
#define TEXT_47_LENGTH 8
#define TEXT_48_LENGTH 6
#define TEXT_49_LENGTH 10
#define TEXT_50_LENGTH 11
#define TEXT_51_LENGTH 10
#define TEXT_52_LENGTH 10
#define TEXT_53_LENGTH 10
#define TEXT_54_LENGTH 11
#define TEXT_55_LENGTH 10
#define TEXT_56_LENGTH 10
#define TEXT_57_LENGTH 12
#define TEXT_58_LENGTH 6
#define TEXT_59_LENGTH 12
#define TEXT_60_LENGTH 12
#define TEXT_61_LENGTH 10
#define TEXT_62_LENGTH 8
#define TEXT_63_LENGTH 10
#define TEXT_64_LENGTH 8
#define TEXT_65_LENGTH 11
#define TEXT_66_LENGTH 9
#define TEXT_67_LENGTH 12
#define TEXT_68_LENGTH 8
#define TEXT_69_LENGTH 7
#define TEXT_70_LENGTH 9
#define TEXT_71_LENGTH 11
#define TEXT_72_LENGTH 12
#define TEXT_73_LENGTH 10
#define TEXT_74_LENGTH 7
#define TEXT_75_LENGTH 12
#define TEXT_76_LENGTH 13
#define TEXT_77_LENGTH 12
#define TEXT_78_LENGTH 12
#define TEXT_79_LENGTH 15
#define TEXT_80_LENGTH 14
#define TEXT_81_LENGTH 13
#define TEXT_82_LENGTH 15
#define TEXT_83_LENGTH 8
#define TEXT_84_LENGTH 10
#define TEXT_85_LENGTH 18
#define TEXT_86_LENGTH 13
#define TEXT_87_LENGTH 11
#define TEXT_88_LENGTH 10
#define TEXT_89_LENGTH 8
#define TEXT_90_LENGTH 6
#define TEXT_91_LENGTH 8
#define TEXT_92_LENGTH 10
#define TEXT_93_LENGTH 6
#define TEXT_94_LENGTH 7
#define TEXT_95_LENGTH 12
#define TEXT_96_LENGTH 11
#define TEXT_97_LENGTH 9
#define TEXT_98_LENGTH 10
#define TEXT_99_LENGTH 9
#define TEXT_100_LENGTH 9
#define TEXT_101_LENGTH 10
#define TEXT_102_LENGTH 12
#define TEXT_103_LENGTH 14
#define TEXT_104_LENGTH 13
#define TEXT_105_LENGTH 7
#define TEXT_106_LENGTH 8
#define TEXT_107_LENGTH 11
#define TEXT_108_LENGTH 8
#include "adv00/object_list/text.inc"
#include "adv00/object_list/values.inc"

/* each kind's create flags, level, attr and range are values the build reads
 * from the disc. no kind here has a param */
static struct object_kind station_square_object_kinds[109] = {
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
    {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, rocket_v_step, TEXT_15},
    {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, rocket_vs_step, TEXT_16},
    {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, panel_start, TEXT_17},
    {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, savepoint_step, TEXT_18},
    {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, invisible_wall_step,
     TEXT_19},
    {VALUE_77, VALUE_78, VALUE_79, VALUE_80, NULL,
     station_square_scene_change_init, TEXT_20},
    {VALUE_81, VALUE_82, VALUE_83, VALUE_84, NULL, line_ring_v_init, TEXT_21},
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, line_ring_init, TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, station_square_train_init,
     TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, pole_init, TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, kart_door_init, TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, shop1_door_init,
     TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, shop2_door_init,
     TEXT_27},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, twinkle_door_init,
     TEXT_28},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, back_door_init, TEXT_29},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, twinkle_a_door_init,
     TEXT_30},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, twinkle_b_door_init,
     TEXT_31},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, twinkle_elevator_init,
     TEXT_32},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, elevator_box_in_init,
     TEXT_33},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, elevator_box_out_init,
     TEXT_34},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL,
     station_square_egg_stand_init, TEXT_35},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL, prop_bottle_init,
     TEXT_36},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL, prop_exit_init, TEXT_37},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL, prop_chair_a_init,
     TEXT_38},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL, prop_chair_b_init,
     TEXT_39},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL, prop_fluorescent_init,
     TEXT_40},
    {VALUE_161, VALUE_162, VALUE_163, VALUE_164, NULL, prop_room_light_init,
     TEXT_41},
    {VALUE_165, VALUE_166, VALUE_167, VALUE_168, NULL, clock_step, TEXT_42},
    {VALUE_169, VALUE_170, VALUE_171, VALUE_172, NULL, shop2_fence_step,
     TEXT_43},
    {VALUE_173, VALUE_174, VALUE_175, VALUE_176, NULL, shop1_lamp_step,
     TEXT_44},
    {VALUE_177, VALUE_178, VALUE_179, VALUE_180, NULL, shop1_salt_step,
     TEXT_45},
    {VALUE_181, VALUE_182, VALUE_183, VALUE_184, NULL, shop1_sign_step,
     TEXT_46},
    {VALUE_185, VALUE_186, VALUE_187, VALUE_188, NULL, signal_step, TEXT_47},
    {VALUE_189, VALUE_190, VALUE_191, VALUE_192, NULL, employee_card_init,
     TEXT_48},
    {VALUE_193, VALUE_194, VALUE_195, VALUE_196, NULL, shop1_chairs_init,
     TEXT_49},
    {VALUE_197, VALUE_198, VALUE_199, VALUE_200, NULL, shop2_fossil_init,
     TEXT_50},
    {VALUE_201, VALUE_202, VALUE_203, VALUE_204, NULL, shop2_radio_init,
     TEXT_51},
    {VALUE_205, VALUE_206, VALUE_207, VALUE_208, NULL, shop2_robot_init,
     TEXT_52},
    {VALUE_209, VALUE_210, VALUE_211, VALUE_212, NULL, shop2_frame_init,
     TEXT_53},
    {VALUE_213, VALUE_214, VALUE_215, VALUE_216, NULL, elevator_sign_init,
     TEXT_54},
    {VALUE_217, VALUE_218, VALUE_219, VALUE_220, NULL, twinkle_handrail_init,
     TEXT_55},
    {VALUE_221, VALUE_222, VALUE_223, VALUE_224, NULL, twinkle_sign_init,
     TEXT_56},
    {VALUE_225, VALUE_226, VALUE_227, VALUE_228, NULL, kart_sign_init, TEXT_57},
    {VALUE_229, VALUE_230, VALUE_231, VALUE_232, NULL, station_square_boat_init,
     TEXT_58},
    {VALUE_233, VALUE_234, VALUE_235, VALUE_236, NULL, shop2_blue_robot_init,
     TEXT_59},
    {VALUE_237, VALUE_238, VALUE_239, VALUE_240, NULL, shop2_box_lid_init,
     TEXT_60},
    {VALUE_241, VALUE_242, VALUE_243, VALUE_244, NULL, shop2_box_init, TEXT_61},
    {VALUE_245, VALUE_246, VALUE_247, VALUE_248, NULL, station_door_init,
     TEXT_62},
    {VALUE_249, VALUE_250, VALUE_251, VALUE_252, NULL, extinguisher_init,
     TEXT_63},
    {VALUE_253, VALUE_254, VALUE_255, VALUE_256, NULL, street_light_init,
     TEXT_64},
    {VALUE_257, VALUE_258, VALUE_259, VALUE_260, NULL, pool_chair_init,
     TEXT_65},
    {VALUE_261, VALUE_262, VALUE_263, VALUE_264, NULL,
     station_square_parasol_init, TEXT_66},
    {VALUE_265, VALUE_266, VALUE_267, VALUE_268, NULL, highway_shutter_init,
     TEXT_67},
    {VALUE_269, VALUE_270, VALUE_271, VALUE_272, NULL, target_fence_init,
     TEXT_68},
    {VALUE_273, VALUE_274, VALUE_275, VALUE_276, NULL, parked_car_init,
     TEXT_69},
    {VALUE_277, VALUE_278, VALUE_279, VALUE_280, NULL, planets_init, TEXT_70},
    {VALUE_281, VALUE_282, VALUE_283, VALUE_284, NULL, hotel_elevator_init,
     TEXT_71},
    {VALUE_285, VALUE_286, VALUE_287, VALUE_288, NULL, lookout_elevator_init,
     TEXT_72},
    {VALUE_289, VALUE_290, VALUE_291, VALUE_292, NULL, highway_elevator_init,
     TEXT_73},
    {VALUE_293, VALUE_294, VALUE_295, VALUE_296, NULL, corridor_step, TEXT_74},
    {VALUE_297, VALUE_298, VALUE_299, VALUE_300, NULL, barricade_init, TEXT_75},
    {VALUE_301, VALUE_302, VALUE_303, VALUE_304, NULL, hotel_front_init,
     TEXT_76},
    {VALUE_305, VALUE_306, VALUE_307, VALUE_308, NULL, hotel_back_init,
     TEXT_77},
    {VALUE_309, VALUE_310, VALUE_311, VALUE_312, NULL, hotel_pool_init,
     TEXT_78},
    {VALUE_313, VALUE_314, VALUE_315, VALUE_316, NULL,
     station_square_scene_change_init, TEXT_79},
    {VALUE_317, VALUE_318, VALUE_319, VALUE_320, NULL, station_back_init,
     TEXT_80},
    {VALUE_321, VALUE_322, VALUE_323, VALUE_324, NULL, casino_door_init,
     TEXT_81},
    {VALUE_325, VALUE_326, VALUE_327, VALUE_328, NULL, casino_button_init,
     TEXT_82},
    {VALUE_329, VALUE_330, VALUE_331, VALUE_332, NULL, ice_stone_init, TEXT_83},
    {VALUE_333, VALUE_334, VALUE_335, VALUE_336, NULL, kart_pass_init, TEXT_84},
    {VALUE_337, VALUE_338, VALUE_339, VALUE_340, NULL, office_barricade_init,
     TEXT_85},
    {VALUE_341, VALUE_342, VALUE_343, VALUE_344, NULL, office_door_init,
     TEXT_86},
    {VALUE_345, VALUE_346, VALUE_347, VALUE_348, NULL,
     station_square_container_init, TEXT_87},
    {VALUE_349, VALUE_350, VALUE_351, VALUE_352, NULL, key_block_init, TEXT_88},
    {VALUE_353, VALUE_354, VALUE_355, VALUE_356, NULL, crowd_person_init,
     TEXT_89},
    {VALUE_357, VALUE_358, VALUE_359, VALUE_360, NULL, hint_init, TEXT_90},
    {VALUE_361, VALUE_362, VALUE_363, VALUE_364, NULL, invisible_sphere_step,
     TEXT_91},
    {VALUE_365, VALUE_366, VALUE_367, VALUE_368, NULL, invisible_cylinder_step,
     TEXT_92},
    {VALUE_369, VALUE_370, VALUE_371, VALUE_372, NULL, invisible_box_step,
     TEXT_93},
    {VALUE_373, VALUE_374, VALUE_375, VALUE_376, NULL, invisible_wall_step,
     TEXT_19},
    {VALUE_377, VALUE_378, VALUE_379, VALUE_380, NULL, ottotto_step, TEXT_94},
    {VALUE_381, VALUE_382, VALUE_383, VALUE_384, NULL, item_box_air_step,
     TEXT_95},
    {VALUE_385, VALUE_386, VALUE_387, VALUE_388, NULL, level_item_step,
     TEXT_96},
    {VALUE_389, VALUE_390, VALUE_391, VALUE_392, NULL, ring_step, TEXT_97},
    {VALUE_393, VALUE_394, VALUE_395, VALUE_396, NULL, power_box_init, TEXT_98},
    {VALUE_397, VALUE_398, VALUE_399, VALUE_400, NULL, real_egg_init, TEXT_99},
    {VALUE_401, VALUE_402, VALUE_403, VALUE_404, NULL, fake_egg_init, TEXT_100},
    {VALUE_405, VALUE_406, VALUE_407, VALUE_408, NULL, chao_card_init,
     TEXT_101},
    {VALUE_409, VALUE_410, VALUE_411, VALUE_412, NULL, hidden_door_init,
     TEXT_102},
    {VALUE_413, VALUE_414, VALUE_415, VALUE_416, NULL, light_changer_init,
     TEXT_103},
    {VALUE_417, VALUE_418, VALUE_419, VALUE_420, NULL, kart_changer_init,
     TEXT_104},
    {VALUE_421, VALUE_422, VALUE_423, VALUE_424, NULL, tikal_init, TEXT_105},
    {VALUE_425, VALUE_426, VALUE_427, VALUE_428, NULL, invisible_jump_step,
     TEXT_106},
    {VALUE_429, VALUE_430, VALUE_431, VALUE_432, NULL, mannequin_step,
     TEXT_107},
    {VALUE_433, VALUE_434, VALUE_435, VALUE_436, NULL, emblem_step, TEXT_108},
};
struct object_list station_square_object_list = {
#include "adv00/object_list/station_square_object_list.inc"
};
