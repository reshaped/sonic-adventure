/* the collapse of the second act's land, which breaks away behind a front */
#ifndef STG06_COLLAPSE_H
#define STG06_COLLAPSE_H
#include <sg_xpt.h>

#include "system/task.h"

/* make the floor under the land's pieces solid, and unset the front. the
 * act's task calls it as the second act starts */
void collapse_reset(void);
/* TRUE in the second act for a point the front has passed. there, z is 0 or
 * more, and x and y are both at or below the front's. FALSE otherwise. the
 * land's pieces there fall a while after the front passes them */
Bool collapse_reached(Float x, Float y, Float z);
/* the collapse's step, which the second act's task starts as a child */
void collapse_step(struct task *task);

#endif
