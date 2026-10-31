/* act 1's mirror room: O RMIRROR, O_MRSNA, O BIGMR, O FOG_SW, O FOG_LI and
 * O HOGHASIRA, and the room's fog */
#ifndef STG07_MIRROR_H
#define STG07_MIRROR_H
#include <sg_xpt.h>

#include "system/task.h"

/* the fog's color. 1ST_READ.BIN's frame_vsync passes it to njSetFogColor
 * each frame of act 1 */
extern Uint32 ruin_fog_color;

/* the object list's steps */
void mirror_step(struct task *task);
void mirror_snake_step(struct task *task);
void big_mirror_step(struct task *task);
void fog_switch_step(struct task *task);
void fog_light_step(struct task *task);
void hoghasira_step(struct task *task);
/* act 1's fog task, which ruin_sky_draw creates. while player 0 is in the
 * mirror room, each step remakes ruin_fog black. its far depth steps toward
 * player 0's distance from the stand the light last reached. a held fog draws
 * in close instead. the first step away from the room makes the fog white out
 * to 1600. later steps there leave it. its destructor starts the mirrors'
 * shared state over */
void ruin_act1_fog_step(struct task *task);

#endif
