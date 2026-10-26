/* the cart course's stage task, and the kinds of object its acts place */
#ifndef MINICART_CART_H
#define MINICART_CART_H

#include "object/placement.h"
#include "system/task.h"

/* the course's acts */
#define CART_ACTS 6

/* the kinds of object the stage places, which cart_object_list holds with
 * their count */
extern struct object_kind cart_object_kinds[];

/* the stage task's first step, which 1ST_READ.BIN's stage_steps names. it
 * starts the music, readies the carts and starts the race. once the act menu
 * picks another act, the race starts again there */
void cart_step(struct task *task);

#endif
