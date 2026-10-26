/* O CONTAINER shipping container, placed separately for Big and for the other
 * characters */
#ifndef ADV00_CONTAINER_H
#define ADV00_CONTAINER_H

#include "system/task.h"

/* the step the object list names for O CONTAINER. a placement's scale x
 * says whom its container is for: 1 for Big and 0 for the others. outside the
 * set editor, a container that isn't for player 0's character kills its
 * placement */
void station_square_container_init(struct task *task);

#endif
