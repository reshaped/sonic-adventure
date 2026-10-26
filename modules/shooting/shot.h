/* the tornado's shots: bullets, and missiles at its locks' targets */
#ifndef SHOOTING_SHOT_H
#define SHOOTING_SHOT_H
#include <sg_xpt.h>

#include "system/task.h"

/* a shot's work->vars, and a missile trail ray's */
struct shot_vars {
    Sint8 unk_00; /* set to 4, never read */
    Sint8 lock; /* a missile's index in lock_targets, or -1 for a bullet */
    Uint8 unk_02[2];
    Float unk_04; /* set to 0, never read */
    Float speed; /* a shot's speed, or a trail ray's alpha */
};

#define SHOT_VARS(work) ((struct shot_vars *)&(work)->vars)

/* the step of a shot, which must be a child of the tornado's task. its
 * creator sets lock before the shot's first step. for a missile, it also
 * sets extra to the task the missile homes on */
void shooting_shot_step(struct task *task);
/* make a ray of the trail of task, a missile, as a child of task. only
 * shooting_shot_step calls it */
void missile_trail(struct task *task);

#endif
