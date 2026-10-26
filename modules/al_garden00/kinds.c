/* the kinds of object the garden places: each one's step, and its name in
 * the set editor */
#include "al_garden00/kinds.h"
#include "al_garden00/door.h"
#include "al_garden00/elevator.h"
#include "al_garden00/fountain.h"
#include "al_main/area.h"
#include "al_main/cam_colli.h"
#include "al_main/tree.h"
#include "al_main/vms.h"
#include "al_main/warp.h"
#include "collision/invisible.h"
#include "object/hint.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/tikal.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 4
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 7
#define TEXT_17_LENGTH 6
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 6
#define TEXT_21_LENGTH 8
#include "al_garden00/kinds/text.inc"
#include "al_garden00/kinds/values.inc"

/* each kind's create flags, level, attr and range are values the build reads
 * from the disc. every kind's param is null */
struct object_kind
    station_square_garden_object_kinds[STATION_SQUARE_GARDEN_KINDS] = {
        {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
        {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, garden_tree_init, TEXT_2},
        {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, garden_flower_step,
         TEXT_3},
        {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, vms_init, TEXT_4},
        {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, invisible_cylinder_step,
         TEXT_5},
        {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, invisible_box_step,
         TEXT_6},
        {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, invisible_sphere_step,
         TEXT_7},
        {VALUE_29, VALUE_30, VALUE_31, VALUE_32, NULL, warp_egg_carrier_init,
         TEXT_8},
        {VALUE_33, VALUE_34, VALUE_35, VALUE_36, NULL, warp_mystic_ruins_init,
         TEXT_9},
        {VALUE_37, VALUE_38, VALUE_39, VALUE_40, NULL, elevator_init, TEXT_10},
        {VALUE_41, VALUE_42, VALUE_43, VALUE_44, NULL, fountain_init, TEXT_11},
        {VALUE_45, VALUE_46, VALUE_47, VALUE_48, NULL, garden_door_init,
         TEXT_12},
        {VALUE_49, VALUE_50, VALUE_51, VALUE_52, NULL, tikal_init, TEXT_13},
        {VALUE_53, VALUE_54, VALUE_55, VALUE_56, NULL, garden_ground_init,
         TEXT_14},
        {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, garden_water_init,
         TEXT_15},
        {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, garden_road_init,
         TEXT_16},
        {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, garden_fly_init,
         TEXT_17},
        {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, garden_cliff_init,
         TEXT_18},
        {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, tikal_init, TEXT_13},
        {VALUE_77, VALUE_78, VALUE_79, VALUE_80, NULL, cam_colli_step, TEXT_19},
        {VALUE_81, VALUE_82, VALUE_83, VALUE_84, NULL, invisible_wall_step,
         TEXT_20},
        {VALUE_85, VALUE_86, VALUE_87, VALUE_88, NULL, hint_init, TEXT_21},
};
