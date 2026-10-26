/* the containers the object list names O CON and O D CON. O CON slides as
 * gravity tilts, and O D CON drifts away once the collapse reaches it */
#ifndef STG06_CONTAINER_H
#define STG06_CONTAINER_H

#include "system/task.h"

/* the steps the object list names for O CON and O D CON */
void sky_deck_container_step(struct task *task);
void sky_deck_container_d_step(struct task *task);

#endif
