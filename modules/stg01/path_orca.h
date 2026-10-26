/* O PO SUMMON's path orca, which leaps in and out of the sea along
 * path_orca_path */
#ifndef STG01_PATH_ORCA_H
#define STG01_PATH_ORCA_H

#include "system/task.h"

/* the step path_orca_summon creates the orca's task with. the task needs a
 * work, and ends once the orca passes the path's end. its destructor makes
 * the controller forget the orca, and stops pad 0's vibration */
void path_orca_init(struct task *task);

#endif
