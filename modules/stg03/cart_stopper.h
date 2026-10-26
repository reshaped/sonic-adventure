/* the cart stopper, which the set editor names O CART STOPPER */
#ifndef STG03_CART_STOPPER_H
#define STG03_CART_STOPPER_H

#include "system/task.h"

/* the step twinkle_object_kinds names for O CART STOPPER, which gives task
 * the stopper's record, step, draw and destructor. task needs a work and a
 * force. the record replaces task->motion without freeing what it held. a
 * stopper that starts while the set editor holds it gets no collision
 * record. its later steps then read through null */
void cart_stopper_start(struct task *task);

#endif
