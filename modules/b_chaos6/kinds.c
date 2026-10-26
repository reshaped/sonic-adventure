/* the kinds of object the stage places: each one's step, and its name */
#include "b_chaos6/kinds.h"
#include "b_chaos6/smoke.h"
#include "collision/invisible.h"
#include "object/placement.h"
#include "object/ring.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 9
#include "b_chaos6/kinds/text.inc"
#include "b_chaos6/kinds/values.inc"

/* each kind's create flags, level, attr and range come from the disc, as
 * the VALUE_ macros of the generated values.inc */
struct object_kind chaos6_object_kinds[CHAOS6_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
    {VALUE_5, VALUE_6, VALUE_7, VALUE_8, NULL, chaos6_smoke_step, TEXT_2},
    {VALUE_9, VALUE_10, VALUE_11, VALUE_12, NULL, invisible_wall_step, TEXT_3},
    {VALUE_13, VALUE_14, VALUE_15, VALUE_16, NULL, invisible_sphere_step,
     TEXT_4},
    {VALUE_17, VALUE_18, VALUE_19, VALUE_20, NULL, invisible_cylinder_step,
     TEXT_5},
    {VALUE_21, VALUE_22, VALUE_23, VALUE_24, NULL, invisible_box_step, TEXT_6},
    {VALUE_25, VALUE_26, VALUE_27, VALUE_28, NULL, ottotto_step, TEXT_7},
};
