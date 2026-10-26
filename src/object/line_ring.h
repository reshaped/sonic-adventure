/* the line ring, O LINE RING: once its switch is on, it drops rings one by
 * one along a straight line */
#ifndef OBJECT_LINE_RING_H
#define OBJECT_LINE_RING_H

#include "system/task.h"

/* the object lists' step for O LINE RING */
void line_ring_init(struct task *task);

#endif
