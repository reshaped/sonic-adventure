/* the lens flare: a task that draws a light's flare as the camera sees it */
#ifndef EFFECT_FLARE_H
#define EFFECT_FLARE_H
#include <ninja.h>

#include "system/task_desc.h"

struct task;

/* a flare's descriptor: no record, and a step that draws lens_draw's flare
 * at work->pos on a frame's first step. its init copies arg, a point, into
 * work->pos unless arg is null */
extern const struct task_desc flare_desc;

/* a flare task at level 4 at *pos, which it copies. returns the task, or
 * null when task_create returns null. its one caller, Chaos 4's intro,
 * leaves the result unchecked */
struct task *flare_create(const NJS_POINT3 *pos);
/* make task a flare at its own work->pos. its draw and destructor become
 * null. returns task */
struct task *flare_setup(struct task *task);

#endif
