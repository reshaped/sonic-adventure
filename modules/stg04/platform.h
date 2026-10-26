/* O STP4S platform that sinks under a rider and rises back, O STP4T one
 * that stands, and O FLYST one that flies to and fro */
#ifndef STG04_PLATFORM_H
#define STG04_PLATFORM_H

#include "system/task.h"

/* the object list's steps for O STP4S, O STP4T and O FLYST */
void platform_sink_step(struct task *task);
void platform_step(struct task *task);
void platform_fly_step(struct task *task);

#endif
