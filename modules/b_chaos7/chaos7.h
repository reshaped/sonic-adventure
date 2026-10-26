/* the Perfect Chaos fight's stage and sky tasks */
#ifndef B_CHAOS7_CHAOS7_H
#define B_CHAOS7_CHAOS7_H

#include "system/task.h"

/* the stage task and sky task, which 1ST_READ.BIN creates from its stage_steps
 * and sky_steps */
void chaos7_step(struct task *task);
void chaos7_sky_step(struct task *task);

/* start the fight's second part. it stops the music, reads act 1's light
 * sources and palettes, and loads CHAOS7_0BREATH2ND into chaos7_breath_texlist.
 * a task it creates asks for the second part's music on its 61st step.
 * chaos7_part_2_end stops the music and reads act 0's lights back */
void chaos7_part_2_start(void);
void chaos7_part_2_end(void);
/* stop the stage's music. chaos7_restart stops it too, destroys the stage
 * task's children, and starts act 0 again from its opening. in another act,
 * it starts nothing. nothing in this image calls chaos7_restart */
void chaos7_music_stop(void);
void chaos7_restart(void);

#endif
