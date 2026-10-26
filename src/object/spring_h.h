/* spring h: a spring board, as long as its scl.x says. a player who touches
 * its side is thrown up from 8 in front of the board */
#ifndef OBJECT_SPRING_H_H
#define OBJECT_SPRING_H_H

#include "system/task.h"

/* the start of a spring board, as the object lists place it */
void spring_h_start(struct task *task);

#endif
