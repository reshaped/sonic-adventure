/* a Chao that makes gestures, again and again */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/face.h"
#include "al_main/gesture.h"
#include "al_main/status.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

#define SE_GESTURE 0x346
#define SE_CHATTER 0x37f
#define SE_CHATTER_2 0x380
#define SE_VOICE 0x352
#define SE_VOICE_2 0x353
#define SE_VOICE_3 0x354

void fun_0c914380(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * forms in R4 at the entry (0x0c914390, 0x0c914396); without it, 123 of
     * 200 words differ */
    struct chao_motion *motion = &chao->motion;
    int gesture;

    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 0)) {
            return;
        }
        chao->limit = (Uint8)(rand() * (20.0F * RAND_SCALE)) + 8;
        work->submode++;
        /* fall through */
    case 1:
        gesture = (int)(rand() * (4.0F * RAND_SCALE));
        switch (gesture) {
        case 0:
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 32;
            sound_play_at(SE_GESTURE, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            break;
        case 1:
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 33;
            sound_play_at(SE_GESTURE, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            break;
        case 2:
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 34;
            sound_play_at(SE_GESTURE, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            break;
        case 3:
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 35;
            sound_play_at(SE_GESTURE, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            break;
        }
        work->submode++;
        break;
    case 2:
        if (motion->flags & MOTION_ENDED) {
            chao->limit--;
            if (chao->limit > 0) {
                work->submode--;
            } else {
                work->mode = MODE_IDLE;
                work->submode = 0;
                work->timer = 0;
            }
        }
        break;
    }
    if (chao->record->curio > 0) {
        alife_face_set_eyes(chao, 2, 2);
    } else {
        alife_face_set_eyes(chao, 0, 2);
    }
    chao->accel.x = -chao->speed.x * 0.2F;
    chao->accel.z = -chao->speed.z * 0.2F;
}

void fun_0c9145a0(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * forms in R4 at the entry (0x0c9145b2, 0x0c9145b8); without it, 93 of
     * 294 words differ */
    struct chao_motion *motion = &chao->motion;
    /* @hack spelling: sound_play_at called through a pointer of its own, which
     * SHC loads into R12 at the entry (0x0c9145bc); without it, 111 of 294
     * words differ */
    int (*play)(int se, int id, int priority, int volume, Float x, Float y,
                Float z) = sound_play_at;
    int gesture;
    /* @bug uninitialized: no path sets chance. its tests read FR15, which
     * holds case 1's 0.3 (FCMP/GT FR15,FR15 at 0x0c914762 and 0x0c9147c2),
     * so gestures 2 and 3 always play SE_VOICE_2 */
#ifdef GCC
    /* keeps the value the tag names: FR15's 0.3 */
    Float chance = 0.3F;
#else
    Float chance;
#endif

    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 4)) {
            return;
        }
        chao->limit = (Uint8)(rand() * (10.0F * RAND_SCALE)) + 8;
        work->submode++;
        if (chao->record->curio > 0) {
            alife_face_set_eyes(chao, 2, 2000);
        } else {
            alife_face_set_eyes(chao, 0, 2000);
        }
        /* fall through */
    case 1:
        if (rand() * RAND_SCALE > 0.3F) {
            if (rand() * RAND_SCALE > 0.5F) {
                gesture = 0;
            } else {
                gesture = 1;
            }
        } else {
            if (rand() * RAND_SCALE > 0.5F) {
                gesture = 2;
            } else {
                gesture = 3;
            }
        }
        switch (gesture) {
        case 0:
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 92;
            play(SE_CHATTER, 0, 0, -20, work->pos.x, work->pos.y, work->pos.z);
            break;
        case 1:
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 94;
            play(SE_CHATTER_2, 0, 0, -20, work->pos.x, work->pos.y,
                 work->pos.z);
            break;
        case 2:
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 96;
            if (chance < 0.3F) { /* NOLINT(clang-diagnostic-uninitialized) */
                play(SE_VOICE, 0, 0, -20, work->pos.x, work->pos.y,
                     work->pos.z);
            } else if (chance < 0.6F) {
                play(SE_VOICE_2, 0, 0, -20, work->pos.x, work->pos.y,
                     work->pos.z);
            } else {
                play(SE_VOICE_3, 0, 0, -20, work->pos.x, work->pos.y,
                     work->pos.z);
            }
            break;
        case 3:
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 97;
            if (chance < 0.3F) {
                play(SE_VOICE, 0, 0, -20, work->pos.x, work->pos.y,
                     work->pos.z);
            } else if (chance < 0.6F) {
                play(SE_VOICE_2, 0, 0, -20, work->pos.x, work->pos.y,
                     work->pos.z);
            } else {
                play(SE_VOICE_3, 0, 0, -20, work->pos.x, work->pos.y,
                     work->pos.z);
            }
            break;
        }
        work->submode++;
        break;
    case 2:
        if (motion->flags & MOTION_ENDED) {
            chao->limit--;
            if (chao->limit > 0) {
                work->submode--;
            } else {
                work->mode = MODE_IDLE;
                work->submode = 0;
                work->timer = 0;
            }
        }
        break;
    }
    chao->accel.x = -chao->speed.x * 0.2F;
    chao->accel.z = -chao->speed.z * 0.2F;
}
