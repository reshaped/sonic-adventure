/* the kinds of object placed in the Mystic Ruins: each one's task, and its
 * name */
#include "adv02/kinds.h"
#include "adv02/ambient_sound.h"
#include "adv02/big_bed.h"
#include "adv02/block_entry.h"
#include "adv02/butterfly.h"
#include "adv02/chaos_emerald.h"
#include "adv02/crowd.h"
#include "adv02/dig_place_1.h"
#include "adv02/dig_place_2.h"
#include "adv02/egg_stand.h"
#include "adv02/eggman_door.h"
#include "adv02/final_egg.h"
#include "adv02/grass.h"
#include "adv02/handy_key.h"
#include "adv02/hidden_gate.h"
#include "adv02/icecap_door.h"
#include "adv02/island_door.h"
#include "adv02/item_stand.h"
#include "adv02/kill_switch.h"
#include "adv02/knuckles_statue.h"
#include "adv02/ladder.h"
#include "adv02/lock.h"
#include "adv02/long_ladder.h"
#include "adv02/master_emerald.h"
#include "adv02/monkey_cage.h"
#include "adv02/mountain_door.h"
#include "adv02/palm.h"
#include "adv02/raft.h"
#include "adv02/sand_entry.h"
#include "adv02/sand_switch.h"
#include "adv02/scene_change.h"
#include "adv02/tornado2.h"
#include "adv02/train.h"
#include "adv02/tree.h"
#include "adv02/truck.h"
#include "adv02/truck_door.h"
#include "adv02/windy.h"
#include "collision/invisible.h"
#include "object/dash_ring.h"
#include "object/hint.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/kusa.h"
#include "object/level_item.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/sample.h"
#include "object/savepoint.h"
#include "object/spring.h"
#include "object/switch.h"
#include "object/tikal.h"
#include "progress/emblem.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 13
#define TEXT_12_LENGTH 13
#define TEXT_13_LENGTH 13
#define TEXT_14_LENGTH 13
#define TEXT_15_LENGTH 13
#define TEXT_16_LENGTH 15
#define TEXT_17_LENGTH 13
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 14
#define TEXT_20_LENGTH 15
#define TEXT_21_LENGTH 16
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 6
#define TEXT_24_LENGTH 13
#define TEXT_25_LENGTH 13
#define TEXT_26_LENGTH 17
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 7
#define TEXT_30_LENGTH 7
#define TEXT_31_LENGTH 11
#define TEXT_32_LENGTH 10
#define TEXT_33_LENGTH 11
#define TEXT_34_LENGTH 10
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 10
#define TEXT_37_LENGTH 10
#define TEXT_38_LENGTH 10
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 6
#define TEXT_41_LENGTH 8
#define TEXT_42_LENGTH 10
#define TEXT_43_LENGTH 6
#define TEXT_44_LENGTH 7
#define TEXT_45_LENGTH 7
#define TEXT_46_LENGTH 12
#define TEXT_47_LENGTH 11
#define TEXT_48_LENGTH 9
#define TEXT_49_LENGTH 12
#define TEXT_50_LENGTH 11
#define TEXT_51_LENGTH 6
#define TEXT_52_LENGTH 6
#define TEXT_53_LENGTH 4
#define TEXT_54_LENGTH 10
#define TEXT_55_LENGTH 7
#define TEXT_56_LENGTH 11
#define TEXT_57_LENGTH 9
#define TEXT_58_LENGTH 9
#define TEXT_59_LENGTH 9
#define TEXT_60_LENGTH 13
#define TEXT_61_LENGTH 8
#define TEXT_62_LENGTH 7
#define TEXT_63_LENGTH 8
#define TEXT_64_LENGTH 8
#define TEXT_65_LENGTH 10
#define TEXT_66_LENGTH 8
#include "adv02/kinds/text.inc"
#include "adv02/kinds/values.inc"

/* each kind's create flags, level, attr and range are the disc's, which the
 * generated values.inc gives as its VALUE_ macros */
struct object_kind mystic_ruins_object_kinds[MYSTIC_RUINS_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, spring_step, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, spring_b_step, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, switch_step, TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, dash_ring_step, TEXT_5},
    {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, item_box_step, TEXT_6},
    {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, savepoint_step, TEXT_7},
    {VALUE_29, VALUE_30, VALUE_31, VALUE_32, NULL, item_stand_step, TEXT_8},
    {VALUE_33, VALUE_34, VALUE_35, VALUE_36, NULL, dig_place_1_step, TEXT_9},
    {VALUE_37, VALUE_38, VALUE_39, VALUE_40, NULL, dig_place_2_step, TEXT_10},
    {VALUE_41, VALUE_42, VALUE_43, VALUE_44, NULL, kill_switch_step, TEXT_11},
    {VALUE_45, VALUE_46, VALUE_47, VALUE_48, NULL, monkey_cage_step, TEXT_12},
    {VALUE_49, VALUE_50, VALUE_51, VALUE_52, NULL, island_door_step, TEXT_13},
    {VALUE_53, VALUE_54, VALUE_55, VALUE_56, NULL, eggman_door_step, TEXT_14},
    {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, sand_switch_step, TEXT_15},
    {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, mountain_door_step, TEXT_16},
    {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, icecap_door_step, TEXT_17},
    {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, truck_step, TEXT_18},
    {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, truck_door_step, TEXT_19},
    {VALUE_77, VALUE_78, VALUE_79, VALUE_80, NULL,
     mystic_ruins_chaos_emerald_step, TEXT_20},
    {VALUE_81, VALUE_82, VALUE_83, VALUE_84, NULL,
     mystic_ruins_master_emerald_step, TEXT_21},
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, handy_key_step, TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, lock_step, TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, hidden_gate_step, TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, block_entry_step, TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, knuckles_statue_step,
     TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, ladder_start, TEXT_27},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, butterfly_step, TEXT_28},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, train_start, TEXT_29},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, raft_step, TEXT_30},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, windy_entrance_step,
     TEXT_31},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, windy_cave_up_step,
     TEXT_32},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, windy_cave_down_step,
     TEXT_33},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL,
     mystic_ruins_scene_change_step, TEXT_34},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL,
     mystic_ruins_final_egg_step, TEXT_35},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL, sample_small_start,
     TEXT_36},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL, sample_medium_start,
     TEXT_37},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL, sample_large_start,
     TEXT_38},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL, sample_shake_start,
     TEXT_39},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL, hint_init, TEXT_40},
    {VALUE_161, VALUE_162, VALUE_163, VALUE_164, NULL, invisible_sphere_step,
     TEXT_41},
    {VALUE_165, VALUE_166, VALUE_167, VALUE_168, NULL, invisible_cylinder_step,
     TEXT_42},
    {VALUE_169, VALUE_170, VALUE_171, VALUE_172, NULL, invisible_box_step,
     TEXT_43},
    {VALUE_173, VALUE_174, VALUE_175, VALUE_176, NULL, invisible_wall_step,
     TEXT_44},
    {VALUE_177, VALUE_178, VALUE_179, VALUE_180, NULL, ottotto_step, TEXT_45},
    {VALUE_181, VALUE_182, VALUE_183, VALUE_184, NULL, item_box_air_step,
     TEXT_46},
    {VALUE_185, VALUE_186, VALUE_187, VALUE_188, NULL, level_item_step,
     TEXT_47},
    {VALUE_189, VALUE_190, VALUE_191, VALUE_192, NULL, big_bed_step, TEXT_48},
    {VALUE_193, VALUE_194, VALUE_195, VALUE_196, NULL, sand_entry_step,
     TEXT_49},
    {VALUE_197, VALUE_198, VALUE_199, VALUE_200, NULL, egg_stand_step, TEXT_50},
    {VALUE_201, VALUE_202, VALUE_203, VALUE_204, NULL, mystic_ruins_palm_step,
     TEXT_51},
    {VALUE_205, VALUE_206, VALUE_207, VALUE_208, NULL, mystic_ruins_tree_step,
     TEXT_52},
    {VALUE_209, VALUE_210, VALUE_211, VALUE_212, NULL,
     mystic_ruins_ambient_sound_step, TEXT_53},
    {VALUE_213, VALUE_214, VALUE_215, VALUE_216, NULL,
     mystic_ruins_final_way_step, TEXT_54},
    {VALUE_217, VALUE_218, VALUE_219, VALUE_220, NULL, grass_step, TEXT_55},
    {VALUE_221, VALUE_222, VALUE_223, VALUE_224, NULL, big_grass_step, TEXT_56},
    {VALUE_225, VALUE_226, VALUE_227, VALUE_228, NULL, windy_leaf_1_step,
     TEXT_57},
    {VALUE_229, VALUE_230, VALUE_231, VALUE_232, NULL, windy_leaf_2_step,
     TEXT_58},
    {VALUE_233, VALUE_234, VALUE_235, VALUE_236, NULL, windy_board_step,
     TEXT_59},
    {VALUE_237, VALUE_238, VALUE_239, VALUE_240, NULL, long_ladder_step,
     TEXT_60},
    {VALUE_241, VALUE_242, VALUE_243, VALUE_244, NULL, crowd_person_init,
     TEXT_61},
    {VALUE_245, VALUE_246, VALUE_247, VALUE_248, NULL, tikal_init, TEXT_62},
    {VALUE_249, VALUE_250, VALUE_251, VALUE_252, NULL, invisible_jump_step,
     TEXT_63},
    {VALUE_253, VALUE_254, VALUE_255, VALUE_256, NULL, kusa_step, TEXT_64},
    {VALUE_257, VALUE_258, VALUE_259, VALUE_260, NULL, tornado2_step, TEXT_65},
    {VALUE_261, VALUE_262, VALUE_263, VALUE_264, NULL, emblem_step, TEXT_66},
};
