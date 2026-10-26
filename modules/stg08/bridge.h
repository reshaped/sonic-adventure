/* TURIBASI 1: a rope bridge that falls apart when an enemy strikes it. it
 * stands again about 200 steps later */
#ifndef STG08_BRIDGE_H
#define STG08_BRIDGE_H

#include "system/task.h"

/* the object list's step */
void icecap_bridge_step(struct task *task);
/* a longer bridge, which takes bridge_init's own step and never breaks.
 * nothing in the image calls it */
void icecap_bridge_2_step(struct task *task);

#endif
