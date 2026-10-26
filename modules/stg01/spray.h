/* the spray O DOLPHIN throws up: drops that fall and shrink as they fly */
#ifndef STG01_SPRAY_H
#define STG01_SPRAY_H

#include <ninja.h>

#include "system/task.h"

/* the drops' texlist, which the stage's pvm list loads */
extern NJS_TEXLIST beach_spray_texlist;

/* a spray drop's step. its task needs a work. the creator sets its pos, its
 * speed in SPRAY_VARS, and scl.x. the drop draws at a scale of scl.x + 1.7,
 * and scl.x shrinks to 0 as it flies. scl.z below 0 makes the drop leave
 * mist, in puffs that scale with -scl.z. the drop kills its task when its
 * flight ends */
void beach_spray_step(struct task *task);

/* a drop's move a step, in work->vars. speed_y is its rise on its first step */
struct spray_vars {
    Float speed_x;
    Float speed_z;
    Float speed_y;
};

#define SPRAY_VARS(work) ((struct spray_vars *)&(work)->vars)

#endif
