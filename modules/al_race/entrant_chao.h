/* the task of the Chao that takes an entrant's place */
#ifndef AL_RACE_ENTRANT_CHAO_H
#define AL_RACE_ENTRANT_CHAO_H

struct alife_chao;
struct task;

/* the task's start, step and destructor. the task's work keeps its entrant
 * in its vars, which the start needs. the task's motion is the Chao. the
 * start sets the Chao up, gives it to the entrant and returns it. while
 * edit_mode is set, the step sets the Chao's scale to 1 and does nothing
 * more. otherwise it takes the entrant's command or runs the Chao's mode,
 * draws the Chao while ENTRANT_SHOW is set, and steps the task's children.
 * the destructor frees the entrant's place and runs fun_0c9002ac's cleanup */
struct alife_chao *entrant_chao_init(struct task *task);
void entrant_chao_step(struct task *task);
void entrant_chao_destroy(struct task *task);

#endif
