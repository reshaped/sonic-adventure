/* the kinds of object the stage places: each one's task, and its name */
#include "b_robo/kinds.h"
#include "collision/invisible.h"
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
#include "object/toge_bubble.h"

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
#include "b_robo/kinds/text.inc"
#include "b_robo/kinds/values.inc"

/* each kind's create flags, level, attr and range come from the disc,
 * through values.inc */
struct object_kind eggmanrobo_object_kinds[EGGMANROBO_KINDS] = {
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
};
