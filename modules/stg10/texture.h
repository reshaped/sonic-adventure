/* Final Egg's animated texture, and O Texture, a square that shows it */
#ifndef STG10_TEXTURE_H
#define STG10_TEXTURE_H
#include <ninja.h>

#include "system/task.h"

/* one 64 by 64 texture. a final_texture_start task loads it on its first
 * step and releases it as it ends */
extern NJS_TEXLIST final_texture_texlist;

/* start a task that keeps the texture. Final Egg starts one as each of its
 * acts starts. the task sleeps after its first step. awake, it paints the
 * texture anew on each frame's first step. it kills itself once the stage or
 * act changes, which a sleeping task never tests. so when the act changes in
 * place, a task asleep lives on beside the next act's task. a retry ends
 * every task */
void final_texture_start(void);
/* take and give back a hold on the texture. O Texture and O TATEKAN hold it
 * while they exist. a hold taken while there is none wakes the latest
 * final_texture_start task to run its first step. giving back the last hold
 * puts that task to sleep. once any such task has ended, a hold wakes none
 * until another runs its first step */
void final_texture_hold(void);
void final_texture_release(void);

/* O Texture: the object list's step */
void final_texture_panel_step(struct task *task);

#endif
