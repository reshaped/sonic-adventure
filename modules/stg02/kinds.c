/* the kinds of object the stage places: each one's task, and its name */
#include "stg02/kinds.h"
#include "collision/invisible.h"
#include "enemy/leon.h"
#include "enemy/sai.h"
#include "enemy/snake.h"
#include "object/balloon.h"
#include "object/capsule.h"
#include "object/dash_panel.h"
#include "object/dash_ring.h"
#include "object/iron_ball.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
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
#include "object/target.h"
#include "object/tikal.h"
#include "object/toge_bubble.h"
#include "stg02/ball.h"
#include "stg02/breakables.h"
#include "stg02/bridge.h"
#include "stg02/cubes.h"
#include "stg02/dandelion.h"
#include "stg02/debris.h"
#include "stg02/e103.h"
#include "stg02/eagle.h"
#include "stg02/emerald.h"
#include "stg02/fences.h"
#include "stg02/gate.h"
#include "stg02/grass.h"
#include "stg02/rock.h"
#include "stg02/rope_bridge.h"
#include "stg02/rotors.h"
#include "stg02/stones.h"
#include "stg02/swirl.h"
#include "stg02/tornado.h"
#include "stg02/trampoline.h"
#include "stg02/tree.h"
#include "stg02/weathervane.h"
#include "stg02/wind.h"

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
#define TEXT_24_LENGTH 10
#define TEXT_25_LENGTH 6
#define TEXT_26_LENGTH 7
#define TEXT_27_LENGTH 7
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 7
#define TEXT_30_LENGTH 7
#define TEXT_31_LENGTH 7
#define TEXT_32_LENGTH 7
#define TEXT_33_LENGTH 8
#define TEXT_34_LENGTH 8
#define TEXT_35_LENGTH 7
#define TEXT_36_LENGTH 7
#define TEXT_37_LENGTH 7
#define TEXT_38_LENGTH 7
#define TEXT_39_LENGTH 8
#define TEXT_40_LENGTH 9
#define TEXT_41_LENGTH 7
#define TEXT_42_LENGTH 7
#define TEXT_43_LENGTH 7
#define TEXT_44_LENGTH 7
#define TEXT_45_LENGTH 6
#define TEXT_46_LENGTH 8
#define TEXT_47_LENGTH 6
#define TEXT_48_LENGTH 8
#define TEXT_49_LENGTH 7
#define TEXT_50_LENGTH 7
#define TEXT_51_LENGTH 7
#define TEXT_52_LENGTH 7
#define TEXT_53_LENGTH 7
#define TEXT_54_LENGTH 8
#define TEXT_55_LENGTH 8
#define TEXT_56_LENGTH 9
#define TEXT_57_LENGTH 7
#define TEXT_58_LENGTH 7
#define TEXT_59_LENGTH 7
#define TEXT_60_LENGTH 7
#define TEXT_61_LENGTH 7
#define TEXT_62_LENGTH 6
#define TEXT_63_LENGTH 7
#define TEXT_64_LENGTH 7
#define TEXT_65_LENGTH 7
#define TEXT_66_LENGTH 7
#define TEXT_67_LENGTH 7
#define TEXT_68_LENGTH 8
#define TEXT_69_LENGTH 8
#define TEXT_70_LENGTH 6
#define TEXT_71_LENGTH 8
#define TEXT_72_LENGTH 8
#define TEXT_73_LENGTH 6
#define TEXT_74_LENGTH 8
#define TEXT_75_LENGTH 11
#define TEXT_76_LENGTH 11
#define TEXT_77_LENGTH 8
#define TEXT_78_LENGTH 7
#define TEXT_79_LENGTH 12
#define TEXT_80_LENGTH 10
#include "stg02/kinds/text.inc"
#include "stg02/kinds/values.inc"

/* each kind's create flags, level, attr and range come from the disc,
 * through values.inc. every kind's param, which its task's work->vars.p[0]
 * would hold, is NULL */
struct object_kind windy_object_kinds[WINDY_KINDS] = {
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
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, ring_group_step, TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, invisible_sphere_step,
     TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, invisible_cylinder_step,
     TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, invisible_box_step,
     TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, ottotto_step, TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, tikal_init, TEXT_27},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, target_start, TEXT_28},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, sai_step, TEXT_29},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, snake_step, TEXT_30},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, leon_start, TEXT_31},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, e103_start, TEXT_32},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, windy_bridge_start,
     TEXT_33},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, windy_dust_source_step,
     TEXT_34},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL, windy_rock_step,
     TEXT_35},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL, windy_small_cube_step,
     TEXT_36},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL, windy_medium_cube_step,
     TEXT_37},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL, windy_large_cube_step,
     TEXT_38},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL,
     windy_floating_island_step, TEXT_39},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL, windy_dandelion_step,
     TEXT_40},
    {VALUE_161, VALUE_162, VALUE_163, VALUE_164, NULL, windy_leaf_blower_step,
     TEXT_41},
    {VALUE_165, VALUE_166, VALUE_167, VALUE_168, NULL, windy_column_wind_step,
     TEXT_42},
    {VALUE_169, VALUE_170, VALUE_171, VALUE_172, NULL, windy_box_wind_step,
     TEXT_43},
    {VALUE_173, VALUE_174, VALUE_175, VALUE_176, NULL, windy_rope_bridge_step,
     TEXT_44},
    {VALUE_177, VALUE_178, VALUE_179, VALUE_180, NULL, windy_yure_step,
     TEXT_45},
    {VALUE_181, VALUE_182, VALUE_183, VALUE_184, NULL, windy_bigfla_step,
     TEXT_46},
    {VALUE_185, VALUE_186, VALUE_187, VALUE_188, NULL, windy_popo_step,
     TEXT_47},
    {VALUE_189, VALUE_190, VALUE_191, VALUE_192, NULL, windy_trampoline_step,
     TEXT_48},
    {VALUE_193, VALUE_194, VALUE_195, VALUE_196, NULL, windy_fence_a_step,
     TEXT_49},
    {VALUE_197, VALUE_198, VALUE_199, VALUE_200, NULL, windy_fence_b_step,
     TEXT_50},
    {VALUE_201, VALUE_202, VALUE_203, VALUE_204, NULL, windy_tree_step,
     TEXT_51},
    {VALUE_205, VALUE_206, VALUE_207, VALUE_208, NULL, windy_fence_c_step,
     TEXT_52},
    {VALUE_209, VALUE_210, VALUE_211, VALUE_212, NULL, windy_hasie_step,
     TEXT_53},
    {VALUE_213, VALUE_214, VALUE_215, VALUE_216, NULL, windy_breakable_step,
     TEXT_54},
    {VALUE_217, VALUE_218, VALUE_219, VALUE_220, NULL,
     windy_falling_bridge_step, TEXT_55},
    {VALUE_221, VALUE_222, VALUE_223, VALUE_224, NULL,
     windy_falling_bridge_2_step, TEXT_56},
    {VALUE_225, VALUE_226, VALUE_227, VALUE_228, NULL,
     windy_breakable_cube_step, TEXT_57},
    {VALUE_229, VALUE_230, VALUE_231, VALUE_232, NULL, windy_stone_a_step,
     TEXT_58},
    {VALUE_233, VALUE_234, VALUE_235, VALUE_236, NULL, windy_stone_b_step,
     TEXT_59},
    {VALUE_237, VALUE_238, VALUE_239, VALUE_240, NULL, windy_flower_a_step,
     TEXT_60},
    {VALUE_241, VALUE_242, VALUE_243, VALUE_244, NULL, windy_flower_b_step,
     TEXT_61},
    {VALUE_245, VALUE_246, VALUE_247, VALUE_248, NULL, windy_gate_step,
     TEXT_62},
    {VALUE_249, VALUE_250, VALUE_251, VALUE_252, NULL, windy_grass_step,
     TEXT_63},
    {VALUE_253, VALUE_254, VALUE_255, VALUE_256, NULL, windy_tate_small_step,
     TEXT_64},
    {VALUE_257, VALUE_258, VALUE_259, VALUE_260, NULL, windy_tate_large_step,
     TEXT_65},
    {VALUE_261, VALUE_262, VALUE_263, VALUE_264, NULL, windy_propeller_a_step,
     TEXT_66},
    {VALUE_265, VALUE_266, VALUE_267, VALUE_268, NULL, windy_propeller_b_step,
     TEXT_67},
    {VALUE_269, VALUE_270, VALUE_271, VALUE_272, NULL, windy_sai_bridge_step,
     TEXT_68},
    {VALUE_273, VALUE_274, VALUE_275, VALUE_276, NULL, windy_weathervane_step,
     TEXT_69},
    {VALUE_277, VALUE_278, VALUE_279, VALUE_280, NULL, windy_ball_step,
     TEXT_70},
    {VALUE_281, VALUE_282, VALUE_283, VALUE_284, NULL, windy_big_flower_step,
     TEXT_71},
    {VALUE_285, VALUE_286, VALUE_287, VALUE_288, NULL, windy_stbrid_step,
     TEXT_72},
    {VALUE_289, VALUE_290, VALUE_291, VALUE_292, NULL, windy_eagle_step,
     TEXT_73},
    {VALUE_293, VALUE_294, VALUE_295, VALUE_296, NULL, windy_tornado_step,
     TEXT_74},
    {VALUE_297, VALUE_298, VALUE_299, VALUE_300, NULL,
     windy_grass_flower_a_step, TEXT_75},
    {VALUE_301, VALUE_302, VALUE_303, VALUE_304, NULL,
     windy_grass_flower_b_step, TEXT_76},
    {VALUE_305, VALUE_306, VALUE_307, VALUE_308, NULL, windy_swirl_step,
     TEXT_77},
    {VALUE_309, VALUE_310, VALUE_311, VALUE_312, NULL, windy_emerald_step,
     TEXT_78},
    {VALUE_313, VALUE_314, VALUE_315, VALUE_316, NULL, item_box_air_step,
     TEXT_79},
    {VALUE_317, VALUE_318, VALUE_319, VALUE_320, NULL, spring_h_start, TEXT_80},
};
