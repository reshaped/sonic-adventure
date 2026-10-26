/* E ACLIFT, a lift that raises an aircraft through doors in the deck */
#ifndef STG06_ACLIFT_H
#define STG06_ACLIFT_H

#include "system/task.h"

/* the step a lift's task starts with. the object list places lifts, and
 * rane.c's deck sections make lifts as children. scl.z picks the way the
 * aircraft leaves. above 0 it turns right, above -10 left, and otherwise it
 * rises straight ahead. a turning aircraft starts out heading the lift's ang.y
 * less 0x4000. one that rises heads ang.y plus 0x4000. a lift that is a child
 * goes with its parent, and any other once out of range */
void aclift_step(struct task *task);

#endif
