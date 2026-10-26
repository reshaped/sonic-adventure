/* the kinds of object the Egg Carrier outside places: each one's step and its
 * name */
#include "adv0100/kinds.h"
#include "adv0100/beach_chair.h"
#include "adv0100/boats.h"
#include "adv0100/book.h"
#include "adv0100/cannon.h"
#include "adv0100/crate.h"
#include "adv0100/door.h"
#include "adv0100/egg_cap.h"
#include "adv0100/egg_lift.h"
#include "adv0100/egg_seat.h"
#include "adv0100/eggman_bed.h"
#include "adv0100/elevator.h"
#include "adv0100/gate.h"
#include "adv0100/gunsight.h"
#include "adv0100/heli.h"
#include "adv0100/lightning.h"
#include "adv0100/living_light.h"
#include "adv0100/mast.h"
#include "adv0100/monorail.h"
#include "adv0100/move_seat.h"
#include "adv0100/palm.h"
#include "adv0100/palm_base.h"
#include "adv0100/parasol.h"
#include "adv0100/pier.h"
#include "adv0100/pool_door.h"
#include "adv0100/pool_water.h"
#include "adv0100/propeller.h"
#include "adv0100/saturn.h"
#include "adv0100/scene_change.h"
#include "adv0100/search_light.h"
#include "adv0100/side_lift.h"
#include "adv0100/sky_deck.h"
#include "adv0100/slot.h"
#include "adv0100/station.h"
#include "adv0100/tornado.h"
#include "adv0100/washer.h"
#include "collision/invisible.h"
#include "object/dash_ring.h"
#include "object/hint.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/line_ring.h"
#include "object/line_ring_v.h"
#include "object/placement.h"
#include "object/ring.h"
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
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 6
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 11
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 9
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 6
#define TEXT_24_LENGTH 8
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 6
#define TEXT_27_LENGTH 7
#define TEXT_28_LENGTH 12
#define TEXT_29_LENGTH 9
#define TEXT_30_LENGTH 8
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 7
#define TEXT_33_LENGTH 8
#define TEXT_34_LENGTH 6
#define TEXT_35_LENGTH 13
#define TEXT_36_LENGTH 10
#define TEXT_37_LENGTH 10
#define TEXT_38_LENGTH 11
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 9
#define TEXT_42_LENGTH 11
#define TEXT_43_LENGTH 10
#define TEXT_44_LENGTH 8
#define TEXT_45_LENGTH 9
#define TEXT_46_LENGTH 6
#define TEXT_47_LENGTH 6
#define TEXT_48_LENGTH 7
#define TEXT_49_LENGTH 7
#define TEXT_50_LENGTH 7
#define TEXT_51_LENGTH 6
#define TEXT_52_LENGTH 12
#define TEXT_53_LENGTH 10
#define TEXT_54_LENGTH 7
#define TEXT_55_LENGTH 7
#define TEXT_56_LENGTH 6
#define TEXT_57_LENGTH 8
#include "adv0100/kinds/text.inc"
#include "adv0100/kinds/values.inc"

/* each kind's create flags, level, attr and range are the disc's, as the
 * generated values.inc names them */
struct object_kind egg_carrier_object_kinds[EGG_CARRIER_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, spring_step, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, spring_b_step, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, switch_step, TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, dash_ring_step, TEXT_5},
    {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, item_box_step, TEXT_6},
    {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, savepoint_step, TEXT_7},
    {VALUE_29, VALUE_30, VALUE_31, VALUE_32, NULL, invisible_wall_step, TEXT_8},
    {VALUE_33, VALUE_34, VALUE_35, VALUE_36, NULL,
     egg_carrier_scene_change_step, TEXT_9},
    {VALUE_37, VALUE_38, VALUE_39, VALUE_40, NULL, egg_carrier_heli_step,
     TEXT_10},
    {VALUE_41, VALUE_42, VALUE_43, VALUE_44, NULL, egg_carrier_crate_step,
     TEXT_11},
    {VALUE_45, VALUE_46, VALUE_47, VALUE_48, NULL,
     egg_carrier_search_light_step, TEXT_12},
    {VALUE_49, VALUE_50, VALUE_51, VALUE_52, NULL, egg_carrier_door_step,
     TEXT_13},
    {VALUE_53, VALUE_54, VALUE_55, VALUE_56, NULL, egg_carrier_doors_step,
     TEXT_14},
    {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, egg_carrier_cannon_step,
     TEXT_15},
    {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, egg_carrier_side_lift_step,
     TEXT_16},
    {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, egg_carrier_tornado_step,
     TEXT_17},
    {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, egg_carrier_gate_step,
     TEXT_18},
    {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, egg_carrier_lightning_step,
     TEXT_19},
    {VALUE_77, VALUE_78, VALUE_79, VALUE_80, NULL,
     egg_carrier_outside_egg_lift_step, TEXT_20},
    {VALUE_81, VALUE_82, VALUE_83, VALUE_84, NULL, egg_carrier_station_step,
     TEXT_21},
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, egg_carrier_monorail_step,
     TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, hint_init, TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, invisible_sphere_step,
     TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, invisible_cylinder_step,
     TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, invisible_box_step,
     TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, invisible_wall_step,
     TEXT_8},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, ottotto_step, TEXT_27},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, item_box_air_step,
     TEXT_28},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL,
     egg_carrier_egg_seat_step, TEXT_29},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, egg_carrier_egg_cap_step,
     TEXT_30},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL,
     egg_carrier_gunsight_step, TEXT_31},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, egg_carrier_saturn_step,
     TEXT_32},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, egg_carrier_washer_step,
     TEXT_33},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL, egg_carrier_slot_step,
     TEXT_34},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL,
     egg_carrier_living_light_step, TEXT_35},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL,
     egg_carrier_elevator_step, TEXT_36},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL,
     egg_carrier_move_seat_step, TEXT_37},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL,
     egg_carrier_eggman_bed_step, TEXT_38},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL,
     egg_carrier_pool_door_step, TEXT_39},
    {VALUE_161, VALUE_162, VALUE_163, VALUE_164, NULL,
     egg_carrier_propeller_step, TEXT_40},
    {VALUE_165, VALUE_166, VALUE_167, VALUE_168, NULL, egg_carrier_door_step,
     TEXT_13},
    {VALUE_169, VALUE_170, VALUE_171, VALUE_172, NULL,
     egg_carrier_sky_deck_step, TEXT_41},
    {VALUE_173, VALUE_174, VALUE_175, VALUE_176, NULL,
     egg_carrier_pool_water_step, TEXT_42},
    {VALUE_177, VALUE_178, VALUE_179, VALUE_180, NULL,
     egg_carrier_palm_base_step, TEXT_43},
    {VALUE_181, VALUE_182, VALUE_183, VALUE_184, NULL,
     egg_carrier_beach_chair_step, TEXT_44},
    {VALUE_185, VALUE_186, VALUE_187, VALUE_188, NULL, egg_carrier_parasol_step,
     TEXT_45},
    {VALUE_189, VALUE_190, VALUE_191, VALUE_192, NULL, egg_carrier_palm_step,
     TEXT_46},
    {VALUE_193, VALUE_194, VALUE_195, VALUE_196, NULL, egg_carrier_pier_step,
     TEXT_47},
    {VALUE_197, VALUE_198, VALUE_199, VALUE_200, NULL, egg_carrier_book1_step,
     TEXT_48},
    {VALUE_201, VALUE_202, VALUE_203, VALUE_204, NULL, egg_carrier_book2_step,
     TEXT_49},
    {VALUE_205, VALUE_206, VALUE_207, VALUE_208, NULL, egg_carrier_book3_step,
     TEXT_50},
    {VALUE_209, VALUE_210, VALUE_211, VALUE_212, NULL, egg_carrier_mast_step,
     TEXT_51},
    {VALUE_213, VALUE_214, VALUE_215, VALUE_216, NULL, line_ring_v_init,
     TEXT_52},
    {VALUE_217, VALUE_218, VALUE_219, VALUE_220, NULL, line_ring_init, TEXT_53},
    {VALUE_221, VALUE_222, VALUE_223, VALUE_224, NULL, tikal_init, TEXT_54},
    {VALUE_225, VALUE_226, VALUE_227, VALUE_228, NULL, egg_carrier_raft_step,
     TEXT_55},
    {VALUE_229, VALUE_230, VALUE_231, VALUE_232, NULL, egg_carrier_boat_step,
     TEXT_56},
    {VALUE_233, VALUE_234, VALUE_235, VALUE_236, NULL, emblem_step, TEXT_57},
};
