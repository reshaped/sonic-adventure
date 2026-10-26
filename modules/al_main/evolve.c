/* MODE_GROW_UP: the cocoon in which a child Chao takes its adult type */
#include <ninja.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/life.h"
#include "al_main/shape.h"
#include "al_main/status.h"
#include "collision/collision.h"
#include "object/common.h"
#include "player/lockon.h"
#include "system/task.h"

void alife_evolve(struct task_work *work, struct alife_chao *chao)
{
    lockon_release(chao->task);
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 7;
        work->submode++;
        break;
    case 1:
        if (TASK_TIMER(work) > 300) {
            work->submode++;
            work->flags &= ~(CARRY_HELD | CARRY_HOLDABLE);
            work->collision->shapes[0].flags |= SHAPE_OFF;
            work->collision->shapes[1].flags |= SHAPE_OFF;
            work->collision->shapes[2].flags &= ~SHAPE_OFF;
            work->collision->shapes[0].levels =
                (work->collision->shapes[0].levels & ~SHAPE_GUARD) | GUARD(3);
            work->collision->shapes[1].levels =
                (work->collision->shapes[1].levels & ~SHAPE_GUARD) | GUARD(3);
            chao->flags |= CHAO_IN_COCOON;
            chao->cocoon_scale = 0.0F;
            chao->cocoon_alpha = 0.2F;
        }
        break;
    case 2:
        /* @hack spelling: the sum written out, which loads the scale once,
         * into FR6, and forms 1 less the scale by an FSUB from a copy of
         * FR5's 1 (0x0c914ed0 to 0x0c914ed6); without it, the scale loads
         * twice, work and chao trade R13 and R14, and 116 of 284 words
         * differ */
        chao->cocoon_scale =
            chao->cocoon_scale + ((1.0F - chao->cocoon_scale) * 0.2F);
        if (chao->cocoon_scale > 0.95F) {
            chao->cocoon_scale = 1.0F;
            work->submode++;
        }
        break;
    case 3:
        chao->cocoon_alpha += 1.0F / 1800.0F;
        if (chao->cocoon_alpha > 1.0F) {
            chao->cocoon_alpha = 1.0F;
            work->submode++;
            status_evolve(work, chao);
            work->timer = 0;
        }
        break;
    case 4:
        if (TASK_TIMER(work) > 300) {
            work->submode++;
            alife_shape_free(work, chao);
            alife_shape_init(work, chao);
            alife_shape_blend(work, chao);
            alife_shape_set_parts(work, chao);
            alife_shape_set_wins(work, chao);
        }
        break;
    case 5:
        chao->cocoon_alpha -= 1.0F / 3600.0F;
        if (chao->cocoon_alpha < 0.2F) {
            chao->cocoon_alpha = 0.2F;
            work->submode++;
        }
        break;
    case 6:
        chao->cocoon_scale -= 0.1F;
        if (chao->cocoon_scale < 0.0F) {
            chao->cocoon_scale = 0.0F;
            work->flags |= CARRY_HOLDABLE;
            work->collision->shapes[0].flags &= ~SHAPE_OFF;
            work->collision->shapes[1].flags &= ~SHAPE_OFF;
            work->collision->shapes[2].flags |= SHAPE_OFF;
            /* @hack spelling: both ands written out, which keep work in R13
             * and chao in R14 (0x0c914dc2, 0x0c914dc6); without them, the two
             * trade, and 150 of 284 words differ */
            work->collision->shapes[0].levels =
                work->collision->shapes[0].levels & ~SHAPE_GUARD;
            work->collision->shapes[1].levels =
                work->collision->shapes[1].levels & ~SHAPE_GUARD;
            /* @hack spelling: the and written out, which keeps work in R13
             * and chao in R14 (0x0c914dc2, 0x0c914dc6); without it, the two
             * trade, and 83 of 284 words differ */
            chao->flags = chao->flags & ~CHAO_IN_COCOON;
            work->submode++;
        }
        break;
    case 7:
        chao->handling = HANDLING_FREE;
        work->mode = MODE_IDLE;
        work->submode = 0;
        work->timer = 0;
        break;
    }
    if (work->flags & CHAO_ON_GROUND) {
        chao->accel.x = -(chao->speed.x * 0.3F);
        chao->accel.z = -(chao->speed.z * 0.3F);
    }
}
