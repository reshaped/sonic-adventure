/* the shards a broken platform or cylinder throws apart */
#ifndef STG06_SHARDS_H
#define STG06_SHARDS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* make a task that throws count shards apart from pos. shard i flies at a
 * random velocity, each component of which lies within speed * 2^i of 0.
 * each step, a shard's velocity gains gravity along y. each shard spins about x
 * and about z. each spin is a random rate of up to 5 degrees a step. the task
 * ends once every shard is more than 300 from pos. it ends too once
 * placement_try_leave_by finds neither the camera nor player 0 within about
 * 1500 of pos. return the task, or NULL when an allocation or task_create
 * fails, which leaks what was allocated before it. unused is never read */
struct task *sky_deck_shards_create(NJS_POINT3 *pos, Float speed, Float gravity,
                                    int count, int unused);

#endif
