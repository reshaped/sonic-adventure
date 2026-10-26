/* Red Mountain's stage task, whose work holds the lava's level, and its sky
 * task */
#ifndef STG05_MOUNTAIN_H
#define STG05_MOUNTAIN_H

#include "system/task.h"

/* the stage task's work, which its first step sets. the step that ends the
 * stage clears it to NULL. pos.y holds the lava's level. scl.x holds the
 * level the lava rises to, and scl.y its rise each step, which MagmaCnt
 * sets. the lava clears scl.y once it reaches scl.x */
extern struct task_work *mountain_work;

struct mountain_vars {
    /* how far the lava's level moved this step, up or down. a raft moves its
     * rider as far */
    Float lava_moved;
};

#define MOUNTAIN_VARS(work) ((struct mountain_vars *)&(work)->vars)

/* the steps 1ST_READ.BIN's stage_steps and sky_steps name for the stage. as
 * an act starts, the stage task reads through null when the music's task can't
 * be made. with Knuckles in act 2, it writes through null when every object of
 * the world's pool is lent */
void mountain_step(struct task *task);
void mountain_sky_step(struct task *task);

/* nothing calls these. no word in the stage or an event names them. no word
 * 1ST_READ.BIN reads while the stage is loaded names them either */
void knuckles_tutorial_create(struct task *task);
void fun_0c91335e(struct task *task);
void fun_0c9137a0(struct task *task);
void fun_0c913a20(int mode);
void fun_0c913bc6(struct task *task);
void fun_0c913c6e(struct task *task);

#endif
