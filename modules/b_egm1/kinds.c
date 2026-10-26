/* the stage places one kind of object, rings. the kind gives their step and
 * their name in the set editor */
#include "b_egm1/kinds.h"
#include "object/placement.h"
#include "object/ring.h"

#define TEXT_1_LENGTH 7
#include "b_egm1/kinds/text.inc"
#include "b_egm1/kinds/values.inc"

/* the kind's create flags, level, attr and range come from the disc,
 * through values.inc */
struct object_kind eggmobile1_object_kinds[EGGMOBILE1_KINDS] = {
    {VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
};
