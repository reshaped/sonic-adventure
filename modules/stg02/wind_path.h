/* Windy Valley path tasks: the wind path that carries a player along it, and an
 * unused spin path */
#ifndef STG02_WIND_PATH_H
#define STG02_WIND_PATH_H

#include "system/task.h"

/* the step the stage's wind paths name. task needs TASK_WORK's record, with a
 * struct path_vars in its vars, as path_create_tasks makes it. a player within
 * 50 of the path's first point runs along the path. the path takes a player no
 * sooner than 60 steps after it last let that player go. every sixth step sends
 * a leaf along the path. the task kills itself once the stage or act changes */
void windy_wind_path_step(struct task *task);
/* windy_wind_path_step's twin, which no path names. a player within 20 of the
 * first point runs the path curled up. the step plays no sound and sends no
 * leaves. player 0's work must exist. the step looks for player 1 past player
 * 0's work. it reads the 64 bytes after that work instead */
void windy_spin_path_step(struct task *task);

#endif
