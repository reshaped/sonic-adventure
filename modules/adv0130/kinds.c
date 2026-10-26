/* the kinds of object the Egg Carrier's inside stage places: each one's step,
 * and its name */
#include "adv0130/kinds.h"
#include "adv0100/station.h"
#include "adv0130/batten.h"
#include "adv0130/blift.h"
#include "adv0130/booster.h"
#include "adv0130/bridge.h"
#include "adv0130/cleaner.h"
#include "adv0130/container.h"
#include "adv0130/egg_door.h"
#include "adv0130/egg_lift.h"
#include "adv0130/egg_warp.h"
#include "adv0130/hall_duct.h"
#include "adv0130/hammer_switch.h"
#include "adv0130/jdoor.h"
#include "adv0130/kurotama.h"
#include "adv0130/mole.h"
#include "adv0130/monorail.h"
#include "adv0130/reservoir.h"
#include "adv0130/rotary.h"
#include "adv0130/scene_change.h"
#include "adv0130/tarai.h"
#include "adv0130/w_robo.h"
#include "collision/invisible.h"
#include "object/hint.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/level_item.h"
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
#define TEXT_6_LENGTH 12
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 10
#define TEXT_21_LENGTH 15
#define TEXT_22_LENGTH 7
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 7
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 6
#define TEXT_29_LENGTH 8
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 6
#define TEXT_32_LENGTH 7
#define TEXT_33_LENGTH 12
#define TEXT_34_LENGTH 9
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 7
#define TEXT_38_LENGTH 8
#include "adv0130/kinds/text.inc"
#include "adv0130/kinds/values.inc"

/* each kind's create flags, level, attr and range are the disc's, as the
 * generated values.inc names them */
struct object_kind egg_carrier_inside_object_kinds[EGG_CARRIER_INSIDE_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, spring_step, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, spring_b_step, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, switch_step, TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, item_box_step, TEXT_5},
    {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, savepoint_step, TEXT_6},
    {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, invisible_wall_step, TEXT_7},
    {VALUE_29, VALUE_30, VALUE_31, VALUE_32, NULL,
     egg_carrier_inside_scene_change_step, TEXT_8},
    {VALUE_33, VALUE_34, VALUE_35, VALUE_36, NULL, egg_carrier_cleaner_step,
     TEXT_9},
    {VALUE_37, VALUE_38, VALUE_39, VALUE_40, NULL, egg_carrier_tarai_step,
     TEXT_10},
    {VALUE_41, VALUE_42, VALUE_43, VALUE_44, NULL, egg_carrier_egg_door_step,
     TEXT_11},
    {VALUE_45, VALUE_46, VALUE_47, VALUE_48, NULL, egg_carrier_batten_step,
     TEXT_12},
    {VALUE_49, VALUE_50, VALUE_51, VALUE_52, NULL, egg_carrier_w_robo_step,
     TEXT_13},
    {VALUE_53, VALUE_54, VALUE_55, VALUE_56, NULL, egg_carrier_hall_duct_step,
     TEXT_14},
    {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, egg_carrier_bridge_step,
     TEXT_15},
    {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, egg_carrier_station_step,
     TEXT_16},
    {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, egg_carrier_rotary_step,
     TEXT_17},
    {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, egg_carrier_monorail_step,
     TEXT_18},
    {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, egg_carrier_egg_lift_step,
     TEXT_19},
    {VALUE_77, VALUE_78, VALUE_79, VALUE_80, NULL,
     egg_carrier_hammer_switch_step, TEXT_20},
    {VALUE_81, VALUE_82, VALUE_83, VALUE_84, NULL, mole_step, TEXT_21},
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, egg_carrier_jdoor_step,
     TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, egg_carrier_jswitch_step,
     TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, egg_carrier_reservoir_step,
     TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, egg_carrier_egg_door_step,
     TEXT_11},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, egg_carrier_blift_step,
     TEXT_25},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL,
     egg_carrier_container_step, TEXT_26},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, egg_carrier_booster_step,
     TEXT_27},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, hint_init, TEXT_28},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, invisible_sphere_step,
     TEXT_29},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, invisible_cylinder_step,
     TEXT_30},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, invisible_box_step,
     TEXT_31},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, invisible_wall_step,
     TEXT_7},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, ottotto_step, TEXT_32},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL, item_box_air_step,
     TEXT_33},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL,
     egg_carrier_egg_warp_step, TEXT_34},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL,
     egg_carrier_kurotama_step, TEXT_35},
    {VALUE_149, VALUE_150, VALUE_151, VALUE_152, NULL, level_item_step,
     TEXT_36},
    {VALUE_153, VALUE_154, VALUE_155, VALUE_156, NULL, tikal_init, TEXT_37},
    {VALUE_157, VALUE_158, VALUE_159, VALUE_160, NULL, emblem_step, TEXT_38},
};
