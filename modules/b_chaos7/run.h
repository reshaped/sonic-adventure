/* Perfect Chaos's run paths, which carry player 0 up to ram the boss */
#ifndef B_CHAOS7_RUN_H
#define B_CHAOS7_RUN_H

#include "path/path.h"
#include "system/task.h"

/* the three runs' paths, which chaos7_paths names */
extern struct path run_0_path;
extern struct path run_1_path;
extern struct path run_2_path;

/* each run's step, which its path names for its task. run n can take player 0
 * only while chaos7_run_open(n) is TRUE */
void chaos7_run_0_step(struct task *task);
void chaos7_run_1_step(struct task *task);
void chaos7_run_2_step(struct task *task);

#endif
