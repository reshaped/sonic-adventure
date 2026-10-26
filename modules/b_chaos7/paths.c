/* the stage's paths */
#include "b_chaos7/paths.h"
#include "b_chaos7/run.h"
#include "path/path.h"

/* path_create_tasks stops at the null */
#pragma section HEAD
struct path *chaos7_paths[4] = {&run_2_path, &run_0_path, &run_1_path, NULL};
#pragma section
