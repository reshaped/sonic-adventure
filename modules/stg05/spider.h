/* Mtn SpiderA, a spider that climbs and drops on a thread, and Mtn SpiderB,
 * one that walks the surface under it and turns back where it ends */
#ifndef STG05_SPIDER_H
#define STG05_SPIDER_H

#include "system/task.h"

/* the steps mountain_object_list names for Mtn SpiderA and Mtn SpiderB.
 * spider A's task needs a work and an extra, and spider B's a work and a
 * motion. each draws at the scale in scl.x, or at 0.06 for 0.
 *
 * spider A hangs from the surface up to 400 above its set entry's position,
 * where there is one. it climbs no higher than it starts, and descends no
 * lower than its scale z. with scale z 0, it picks a bottom between itself
 * and the floor up to 1000 below it. with no floor there, the bottom comes
 * from a floor height no path sets. unless the set editor holds the spider,
 * its first step allocates its thread from the heap. that step writes
 * through null when the heap can't hold the thread */
void mountain_spider_a_step(struct task *task);
void mountain_spider_b_step(struct task *task);

#endif
