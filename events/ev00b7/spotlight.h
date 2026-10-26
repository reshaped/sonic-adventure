/* EV00B7.PRS's spotlights, which sway until they aim at a point */
#ifndef EV00B7_SPOTLIGHT_H
#define EV00B7_SPOTLIGHT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

extern NJS_TEXLIST ev00b7_spotlight_texlist;

/* make a spotlight at the point, swaying until it is aimed, or return null
 * if task_create fails */
struct task *ev00b7_spotlight_create(Float x, Float y, Float z);
/* turn the spotlight to the point once, and hold it there. task must not
 * be null */
void ev00b7_spotlight_aim(struct task *task, Float x, Float y, Float z);
/* kill the spotlight, if there is one. the caller's pointer to it stays
 * as it was */
void ev00b7_spotlight_kill(struct task *task);

#endif
