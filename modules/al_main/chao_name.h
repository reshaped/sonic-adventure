/* a Chao's name, in letters above its head */
#ifndef AL_MAIN_CHAO_NAME_H
#define AL_MAIN_CHAO_NAME_H
#include <ninja.h>

#include "al_main/chao.h"
#include "chao/chao.h"
#include "system/task.h"

/* draw chao's name on a window, facing the screen, centered on pos in the
 * current matrix's frame. its letters are scale wide, in color, which must
 * not be NULL. the name ends at its first zero. the draw cuts a name that
 * isn't empty to seven letters: it zeroes chao->name[CHAO_NAME_SIZE - 1] */
void chao_name_draw_at(struct chao *chao, NJS_POINT3 *pos, Float scale,
                       NJS_COLOR *color);
/* chao_name_step fades the name and draws it above the Chao while its alpha
 * is past 10, at a size that grows with the alpha. while CHAO_SHOW_NAME is
 * set, the name fades in for its first 150 frames, then out. while the flag
 * is clear, it fades out. chao_name_draw draws the name as it stands,
 * without the fade */
void chao_name_step(struct task_work *work, struct alife_chao *chao);
void chao_name_draw(struct task_work *work, struct alife_chao *chao);

#endif
