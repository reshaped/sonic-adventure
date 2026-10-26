/* MODE_LIFE_OVER: the cocoon that ends a Chao's life, and may leave an egg
 * of its next generation */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "al_main/face.h"
#include "al_main/life.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "object/common.h"
#include "player/lockon.h"
#include "system/task.h"

struct reborn_vars {
    Uint8 unk_00[8];
    /* the task of the egg the Chao leaves, or NULL. the task is the egg's
     * own, and must last until the cocoon is gone and lets the egg go */
    struct task *egg;
};
#define VARS(work) ((struct reborn_vars *)&(work)->vars)

void alife_reincarnate(struct task_work *work, struct alife_chao *chao)
{
    lockon_release(chao->task);
    alife_face_set_eyes(chao, 5, 1);
    alife_face_set_mouth(chao, 5, 1);
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 7;
        work->submode++;

        VARS(work)->egg = NULL;
        break;
    case 1:
        if (TASK_TIMER(work) > 60) {
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
            chao->record->reincarnations++;
        }
        break;
    case 2:
        /* @hack spelling: the sum written out, which loads the scale once,
         * into FR6, and subtracts it by an FSUB from a copy of FR5's 1
         * (0x0c9151c2 to 0x0c9151c8); without it, the scale loads twice, 1
         * less the scale takes an FNEG and an FADD, and 46 of 327 words
         * differ */
        chao->cocoon_scale =
            chao->cocoon_scale + ((1.0F - chao->cocoon_scale) * 0.2F);
        if (chao->cocoon_scale > 0.99F) {
            chao->cocoon_scale = 1.0F;
            work->submode++;
        }
        break;
    case 3:
        chao->cocoon_alpha += 1.0F / 1200.0F;
        if (chao->cocoon_alpha > 1.0F) {
            chao->cocoon_alpha = 1.0F;
            work->submode++;
            work->timer = 0;
        }
        break;
    case 4:
        if (TASK_TIMER(work) > 600) {
            work->submode++;
            if (chao->record->user_like > 60 &&
                chao->record->type != CHAO_CHAOS && chao->record->hp) {
                struct chao *slot = chao_find_empty();
                VARS(work)->egg = NULL;
                if (slot) {
                    struct chao *record;
                    struct task *egg;
                    chao_reborn(chao->record);
                    record = chao->record;
                    {
                        /* @hack redundant: a record never used, whose 512
                         * bytes give the image's frame (MOV.W #-512 and ADD
                         * R0,R15 at 0x0c91508c, 0x0c91508e); without it, 79 of
                         * 327 words differ */
                        /* NOLINTNEXTLINE(clang-diagnostic-unused-variable) */
                        struct chao unused;
                        *slot = *record;
                    }
                    chao->record = slot;
                    chao->record->type = CHAO_NOTHING;
                    record->type = CHAO_EGG;
                    egg = alife_chao_create(record);
                    if (egg) {
                        struct alife_chao *egg_chao =
                            (struct alife_chao *)egg->motion;
                        struct task_work *egg_work = egg->work;
                        egg_chao->handling = HANDLING_REBORN;
                        egg_work->submode = 0;
                        egg_chao->flags |= 0x40;
                        VARS(work)->egg = egg;
                        egg_work->pos = work->pos;
                        record->pos = work->pos;
                    }
                }
            }
            chao->flags &= ~CHAO_SHAPE_SHOWN;
        }
        break;
    case 5:
        chao->cocoon_alpha -= 1.0F / 1200.0F;
        if (chao->cocoon_alpha < 0.2F) {
            chao->cocoon_alpha = 0.2F;
            work->submode++;
        }
        break;
    case 6:
        chao->cocoon_scale -= 0.1F;
        if (chao->cocoon_scale < 0.0F) {
            chao->cocoon_scale = 0.0F;
            if (VARS(work)->egg) {
                struct task_work *egg_work = VARS(work)->egg->work;
                struct alife_chao *egg_chao =
                    (struct alife_chao *)VARS(work)->egg->motion;
                egg_work->submode++;
                egg_chao->flags &= ~0x40;
            }
            alife_chao_remove(chao->task);
        }
        break;
    }
    if (work->flags & CHAO_ON_GROUND) {
        chao->accel.x = -(chao->speed.x * 0.3F);
        chao->accel.z = -(chao->speed.z * 0.3F);
    }
}
