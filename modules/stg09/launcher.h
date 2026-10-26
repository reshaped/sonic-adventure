/* O HOSIN, an unseen launcher that flings player 0 out along its aim */
#ifndef STG09_LAUNCHER_H
#define STG09_LAUNCHER_H

#include "system/task.h"

/* the object list's step. the launcher is a cylinder of radius scl.x + 5.
 * whatever touches it, it flings player 0 along its aim at 1.5 + scl.z, and
 * raises dust. the aim is (scl.y, 10, 0) in the launcher's model space,
 * which its angles turn. a fling leaves player 0 beyond its control for 60
 * steps */
void launcher_step(struct task *task);

#endif
