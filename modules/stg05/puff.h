/* Red Mountain's puffs: smoke and flame sprites in four groups. each group
 * is a task that steps its puffs and draws them */
#ifndef STG05_PUFF_H
#define STG05_PUFF_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* a puff, as its task's work */
struct mountain_puff {
    Float rate; /* the animation's frames a step, which only its step reads */
    Uint8 unk_04[4];
    Float frame;
    Angle ang;
    Float scale;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
};

/* mountain_puff_create's groups, which differ in texture and tint */
#define PUFF_SMOKE 0
#define PUFF_DARK_SMOKE 1
#define PUFF_FLAME 2 /* its color adds to what's behind it */
#define PUFF_VIOLET_SMOKE 3

/* create the four groups' tasks, and set up their sprites. it writes through
 * null when no task can be made. nothing calls it. no word in the stage or an
 * event names it. no word 1ST_READ.BIN reads while the stage is loaded names it
 * either */
void mountain_puff_init(void);
/* create a puff's task, which runs step, as a child of the task of group.
 * mountain_puff_init must have made that task, or the new task is linked
 * through null. returns null when no task can be made. the task's work is a
 * struct mountain_puff from heap_alloc, not cleared. the work is null when no
 * block fits, and the group's draw then reads through it. the caller sets the
 * frame, ang, scale and pos that the group draws, and whatever step reads */
struct task *mountain_puff_create(int group, void (*step)(struct task *task));
/* the animation frames of the group whose puffs are stepping, for their
 * steps to read */
int mountain_puff_frames(void);

#endif
