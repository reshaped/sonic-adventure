/* the kinds of object the race places: each one's task, and its name */
#include "al_race/kinds.h"
#include "al_race/bow_chao.h"
#include "al_race/bugle.h"
#include "al_race/butterfly.h"
#include "al_race/cheer_chao.h"
#include "al_race/coconut.h"
#include "al_race/cracker.h"
#include "al_race/entry_button.h"
#include "al_race/flag.h"
#include "al_race/flower.h"
#include "al_race/palm.h"
#include "al_race/props.h"
#include "al_race/scoop.h"
#include "al_race/watering_can.h"
#include "al_race/zone.h"
#include "object/hint.h"
#include "object/placement.h"
#include "object/ring.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 4
#define TEXT_4_LENGTH 4
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 4
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 13
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 5
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 5
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 9
#include "al_race/kinds/text.inc"
#include "al_race/kinds/values.inc"

/* each kind's create flags, level, attr and range are the disc's, which the
 * generated values.inc gives as its VALUE_ macros */
struct object_kind race_object_kinds[RACE_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, entry_button_init, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, zone_init, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, race_palm_init, TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, coconut_init, TEXT_5},
    {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, flag_init, TEXT_6},
    {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, white_flower_init, TEXT_7},
    {VALUE_29, VALUE_30, VALUE_31, VALUE_32, NULL, purple_flower_init, TEXT_8},
    {VALUE_33, VALUE_34, VALUE_35, VALUE_36, NULL, start_mark_init, TEXT_9},
    {VALUE_37, VALUE_38, VALUE_39, VALUE_40, NULL, watering_can_init, TEXT_10},
    {VALUE_41, VALUE_42, VALUE_43, VALUE_44, NULL, bugle_init, TEXT_11},
    {VALUE_45, VALUE_46, VALUE_47, VALUE_48, NULL, scoop_init, TEXT_12},
    {VALUE_49, VALUE_50, VALUE_51, VALUE_52, NULL, small_screen_init, TEXT_13},
    {VALUE_53, VALUE_54, VALUE_55, VALUE_56, NULL, islet_init, TEXT_14},
    {VALUE_57, VALUE_58, VALUE_59, VALUE_60, NULL, hint_init, TEXT_15},
    {VALUE_61, VALUE_62, VALUE_63, VALUE_64, NULL, cheer_chao_init, TEXT_16},
    {VALUE_65, VALUE_66, VALUE_67, VALUE_68, NULL, bow_chao_init, TEXT_17},
    {VALUE_69, VALUE_70, VALUE_71, VALUE_72, NULL, cracker_init, TEXT_18},
    {VALUE_73, VALUE_74, VALUE_75, VALUE_76, NULL, butterfly_init, TEXT_19},
};
