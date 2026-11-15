/* the port's stand-in for Flycast's scheduler: the host steps the AICA
 * itself, by arm::run, so nothing is scheduled */
#pragma once
#include "types.h"
typedef int sh4_sched_callback(int tag, int cycles, int jitter, void *arg);
static inline int sh4_sched_register(int, sh4_sched_callback *, void * = nullptr) { return 0; }
static inline void sh4_sched_unregister(int) {}
static inline void sh4_sched_request(int, int) {}
