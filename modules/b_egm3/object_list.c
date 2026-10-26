/* the kinds of object the stage places, a ring and four rockets, and their
 * names. the list holds them */
#include "b_egm3/object_list.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/rocket.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 9
#include "b_egm3/object_list/text.inc"
#include "b_egm3/object_list/values.inc"

#pragma section HEAD
/* each kind's create flags, level, attr and range come from the disc,
 * through values.inc. no kind here has a param */
static struct object_kind eggmobile3_object_kinds[5] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, rocket_h_step, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, rocket_hs_step, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, rocket_v_step, TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, rocket_vs_step, TEXT_5},
};
struct object_list eggmobile3_object_list = {
#include "b_egm3/object_list/eggmobile3_object_list.inc"
};
#pragma section
