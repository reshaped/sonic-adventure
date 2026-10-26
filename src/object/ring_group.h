/* the ring group, "O GRING": rings on a line or round a circle, each a
 * child task. a taken ring's bit in the placement's state keeps the group
 * from making it again */
#ifndef OBJECT_RING_GROUP_H
#define OBJECT_RING_GROUP_H

struct task;

/* the step of a group the stage places */
void ring_group_step(struct task *task);

#endif
