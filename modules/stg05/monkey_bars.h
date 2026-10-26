/* o_untei1, o_untei2, o_untei3 and o_untei6: monkey bars, each with a path
 * that players hang from */
#ifndef STG05_MONKEY_BARS_H
#define STG05_MONKEY_BARS_H

#include "system/task.h"

/* the steps mountain_object_list names for o_untei1, o_untei2, o_untei3 and
 * o_untei6. unless the set editor holds the bars, their first step sets up the
 * path. that step reads through null when no task can be made to run the path.
 * it writes through null when the heap can't hold the path */
void monkey_bars_1_step(struct task *task);
void monkey_bars_2_step(struct task *task);
void monkey_bars_3_step(struct task *task);
void monkey_bars_6_step(struct task *task);

#endif
