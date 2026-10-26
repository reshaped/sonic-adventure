/* path task that runs players along a path from its first point */
#ifndef STG10_RUN_PATH_H
#define STG10_RUN_PATH_H

#include "system/task.h"

/* a path task's step for players 0 and 1, which nothing runs. no source
 * names it, and no word in the image or in 1ST_READ.BIN holds its address.
 * a player within 20 units of the path's first point runs along the path.
 * the path takes the player again 60 steps after the player leaves it. the
 * task ends itself once the stage or act changes */
void final_run_path_step(struct task *task);

#endif
