/* HIGH RAFT A and HIGH RAFT C platforms, O TUNAGI solid joining block, and
 * O HELIP L heliport light that is only drawn */
#ifndef STG04_RAFT_H
#define STG04_RAFT_H

#include "system/task.h"

/* the object list's steps */
void highway_raft_a_step(struct task *task);
void highway_raft_c_step(struct task *task);
void highway_raft_joint_step(struct task *task);
void heliport_light_step(struct task *task);

#endif
