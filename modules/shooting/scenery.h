/* the sky chase's scenery: the land far under the tornado, and its cloud */
#ifndef SHOOTING_SCENERY_H
#define SHOOTING_SCENERY_H

#include "system/task.h"

/* the land's step; it makes the cloud */
void shooting_scenery_step(struct task *task);
/* a cloud far under the land, which nothing makes */
void shooting_low_cloud_step(struct task *task);

#endif
