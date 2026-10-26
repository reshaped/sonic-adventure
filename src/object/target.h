/* targets: things E-102 shoots down, which the stages count by id */
#ifndef OBJECT_TARGET_H
#define OBJECT_TARGET_H
#include <sg_xpt.h>

#include "system/task.h"

/* the start of a target. a stage may start no more than 64 targets: the
 * 65th writes past the list target_cleared reads */
void target_start(struct task *task);
/* TRUE when a target with the id has started since target_reset and every
 * such target is shot down, else FALSE */
Bool target_cleared(int id);
/* forget every target, for a new stage */
void target_reset(void);

#endif
