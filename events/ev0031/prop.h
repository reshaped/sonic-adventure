/* EV0031.PRS's prop: a model of the event's own, standing still */
#ifndef EV0031_PROP_H
#define EV0031_PROP_H

#include "system/task.h"

/* create the prop, with no event record, for the caller to place. its step
 * draws it at work->pos and work->ang with texture_set_regular's texlist. it
 * has no draw, so task_draw_all passes it by. null when task_create fails */
struct task *ev0031_prop_create(void);

#endif
