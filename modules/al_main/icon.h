/* a Chao's icon: the ball over its head, and a symbol that springs up from
 * it */
#ifndef AL_MAIN_ICON_H
#define AL_MAIN_ICON_H
#include <ninja.h>

#include "al_main/chao.h"
#include "system/task.h"

/* the icon's kind, which picks how alife_icon_step moves it. ICON_BALL is
 * the ball alone. kinds 1 to 3, ICON_GLAD_1, ICON_GLAD and ICON_GLAD_3, show
 * one symbol, ICON_GLAD's. ICON_SYMBOL_3 to ICON_SYMBOL_5 show textures 3 to 5
 * of alife_icon_texlist, and ICON_HURT shows its own. ICON_HOLD and
 * ICON_HOLD_2 leave the icon as it stands */
enum icon_kind {
    ICON_BALL = 0,
    ICON_GLAD_1 = 1,
    ICON_GLAD = 2,
    ICON_GLAD_3 = 3,
    ICON_SYMBOL_3 = 4,
    ICON_SYMBOL_4 = 5,
    ICON_SYMBOL_5 = 6,
    ICON_HURT = 7,
    ICON_HOLD = 8,
    ICON_HOLD_2 = 9
};

extern NJS_TEXLIST alife_icon_texlist; /* the gardens load it from AL_ICON */

/* set the icon's kind to ICON_BALL, the ball alone, and its unk_06 to
 * 0xffff */
void alife_icon_clear(struct task_work *work, struct alife_chao *chao);
/* move the Chao's icon as its kind says, and draw it. a Chaos Chao's draws
 * as a ring of light. each call counts 1 up in both parts' frames */
void alife_icon_step(struct task_work *work, struct alife_chao *chao);
/* draw the Chao's icon and count 1 up as alife_icon_step does, without
 * moving it */
void alife_icon_draw(struct task_work *work, struct alife_chao *chao);

#endif
