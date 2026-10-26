/* the kinds of object the stage places: each one's step, and its name */
#include "b_chaos7/kinds.h"
#include "b_chaos7/axel.h"
#include "b_chaos7/breakable.h"
#include "b_chaos7/building.h"
#include "b_chaos7/fall.h"
#include "b_chaos7/props.h"
#include "b_chaos7/road.h"
#include "b_chaos7/tentacle.h"
#include "collision/invisible.h"
#include "object/dash_panel.h"
#include "object/dash_ring.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/line_ring.h"
#include "object/line_ring_v.h"
#include "object/panel.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/ring_group.h"
#include "object/spring.h"
#include "object/spring_h.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 13
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 12
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 6
#define TEXT_18_LENGTH 6
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 11
#define TEXT_21_LENGTH 9
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 8
#define TEXT_24_LENGTH 8
#define TEXT_25_LENGTH 8
#define TEXT_26_LENGTH 8
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 8
#define TEXT_29_LENGTH 8
#define TEXT_30_LENGTH 8
#define TEXT_31_LENGTH 8
#define TEXT_32_LENGTH 8
#define TEXT_33_LENGTH 6
#define TEXT_34_LENGTH 8
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 6
#define TEXT_37_LENGTH 9
#include "b_chaos7/kinds/text.inc"
#include "b_chaos7/kinds/values.inc"

/* VALUE_1 to VALUE_148 are the kinds' create, level, attr and range, numbers
 * the build takes from the disc */
struct object_kind chaos7_object_kinds[CHAOS7_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, ring_group_step, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, line_ring_v_init, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, line_ring_init, TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, spring_step, TEXT_5},
    {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, spring_b_step, TEXT_6},
    {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, spring_h_start, TEXT_7},
    {VALUE_29, VALUE_30, VALUE_31, VALUE_32, NULL, dash_panel_step, TEXT_8},
    {VALUE_33, VALUE_34, VALUE_35, VALUE_36, NULL, chaos7_axel_panel_step,
     TEXT_9},
    {VALUE_37, VALUE_38, VALUE_39, VALUE_40, NULL, panel_start, TEXT_10},
    {VALUE_41, VALUE_42, VALUE_43, VALUE_44, NULL, dash_ring_step, TEXT_11},
    {VALUE_45, VALUE_46, VALUE_47, VALUE_48, NULL, item_box_step, TEXT_12},
    {VALUE_49, VALUE_50, VALUE_51, VALUE_52, NULL, item_box_air_step, TEXT_13},
    {VALUE_53, VALUE_54, VALUE_55, VALUE_56, NULL, chaos7_red_car_step,
     TEXT_14},
    {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, chaos7_yellow_car_step,
     TEXT_15},
    {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, chaos7_blue_car_step,
     TEXT_16},
    {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, chaos7_cone_step, TEXT_17},
    {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, chaos7_tire_step, TEXT_18},
    {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, chaos7_gray_building_step,
     TEXT_19},
    {VALUE_77, VALUE_78, VALUE_79, VALUE_80, NULL, chaos7_brown_building_step,
     TEXT_20},
    {VALUE_81, VALUE_82, VALUE_83, VALUE_84, NULL, chaos7_red_building_step,
     TEXT_21},
    {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, chaos7_tentacle_step,
     TEXT_22},
    {VALUE_89, VALUE_90, VALUE_91, VALUE_92, NULL, chaos7_road_0_step, TEXT_23},
    {VALUE_93, VALUE_94, VALUE_95, VALUE_96, NULL, chaos7_road_1_step, TEXT_24},
    {VALUE_97, VALUE_98, VALUE_99, VALUE_100, NULL, chaos7_road_2_step,
     TEXT_25},
    {VALUE_101, VALUE_102, VALUE_103, VALUE_104, NULL, chaos7_road_3_step,
     TEXT_26},
    {VALUE_105, VALUE_106, VALUE_107, VALUE_108, NULL, chaos7_road_4_step,
     TEXT_27},
    {VALUE_109, VALUE_110, VALUE_111, VALUE_112, NULL, chaos7_signal_step,
     TEXT_28},
    {VALUE_113, VALUE_114, VALUE_115, VALUE_116, NULL, chaos7_fall_0_step,
     TEXT_29},
    {VALUE_117, VALUE_118, VALUE_119, VALUE_120, NULL, chaos7_wall_a_step,
     TEXT_30},
    {VALUE_121, VALUE_122, VALUE_123, VALUE_124, NULL, chaos7_wall_b_step,
     TEXT_31},
    {VALUE_125, VALUE_126, VALUE_127, VALUE_128, NULL, chaos7_wall_c_step,
     TEXT_32},
    {VALUE_129, VALUE_130, VALUE_131, VALUE_132, NULL, invisible_wall_step,
     TEXT_33},
    {VALUE_133, VALUE_134, VALUE_135, VALUE_136, NULL, invisible_sphere_step,
     TEXT_34},
    {VALUE_137, VALUE_138, VALUE_139, VALUE_140, NULL, invisible_cylinder_step,
     TEXT_35},
    {VALUE_141, VALUE_142, VALUE_143, VALUE_144, NULL, invisible_box_step,
     TEXT_36},
    {VALUE_145, VALUE_146, VALUE_147, VALUE_148, NULL, ottotto_step, TEXT_37},
};
