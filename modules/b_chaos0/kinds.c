/* the kinds of object the stage places: each one's step, and its name in
 * the set editor */
#include "b_chaos0/kinds.h"
#include "b_chaos0/clock.h"
#include "b_chaos0/grass.h"
#include "b_chaos0/helicopter.h"
#include "b_chaos0/patrol_car.h"
#include "b_chaos0/pole.h"
#include "object/placement.h"
#include "object/ring.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 6
#define TEXT_9_LENGTH 8
#include "b_chaos0/kinds/text.inc"
#include "b_chaos0/kinds/values.inc"

/* each kind's create flags, level, attr and range come from the disc,
 * through values.inc. every kind's param, which its task's work->vars.p[0]
 * would hold, is NULL */
struct object_kind chaos0_object_kinds[CHAOS0_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, chaos0_grass_0_step, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, chaos0_grass_1_step, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, chaos0_grass_2_step, TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, chaos0_grass_3_step, TEXT_5},
    {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, chaos0_helicopter_step,
     TEXT_6},
    {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, chaos0_patrol_car_step,
     TEXT_7},
    {VALUE_29, VALUE_30, VALUE_31, VALUE_32, NULL, chaos0_pole_step, TEXT_8},
    {VALUE_33, VALUE_34, VALUE_35, VALUE_36, NULL, chaos0_clock_step, TEXT_9},
};
