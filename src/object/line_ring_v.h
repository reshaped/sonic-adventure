/* line ring v, O LINE RING V: once its switch is on, it drops rings one by
 * one. they rise along a quarter circle from the ground, then straight
 * up */
#ifndef OBJECT_LINE_RING_V_H
#define OBJECT_LINE_RING_V_H

#include "system/task.h"

/* the object lists' step for O LINE RING V, or O LINERING V in the fields */
void line_ring_v_init(struct task *task);

#endif
