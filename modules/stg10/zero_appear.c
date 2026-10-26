/* C APP ZERO flags, which the robo walls and the robo shutter wait on */
#include <sg_xpt.h>

#include "stg10/zero_appear.h"

static Bool zero_appear_flags[ZERO_APPEAR_FLAGS];

void zero_appear_reset(void)
{
    int i;

    for (i = 0; i < ZERO_APPEAR_FLAGS; i++) {
        zero_appear_flags[i] = FALSE;
    }
}

Bool zero_appear_is_on(int id)
{
    return zero_appear_flags[id];
}

void zero_appear_set(int id)
{
    zero_appear_flags[id] = TRUE;
}
