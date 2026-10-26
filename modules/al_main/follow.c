/* a Chao that follows player 0 */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/alife.h"
#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/face.h"
#include "al_main/follow.h"
#include "al_main/physics.h"
#include "al_main/status.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

#define SE_CHATTER 0x37f
#define SE_CHATTER_2 0x380

/* fun_0c9136e0 starts only with at least this like for player 0's
 * character */
#define FOND_LIKE 50

void fun_0c913280(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * keeps in the frame at sp (0x0c913296); without it, 151 of 207 words
     * differ */
    struct chao_motion *motion = &chao->motion;
    Angle ang;
    Float sine;
    Float cosine;

    chao->target = player_work[0]->pos;
    chao->turn_speed = 0x400;
    alife_turn_to_target(work, chao);
    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 8)) {
            return;
        }
        chao->limit = (Uint8)(rand() * (200.0F * RAND_SCALE)) + 180;
        work->submode++;
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 142;
        /* fall through */
    case 1:
        if (chao->sound_timer++ > 30) {
            chao->sound_timer = 0;
            if (rand() * RAND_SCALE > 0.5F) {
                sound_play_at(SE_CHATTER, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            } else {
                sound_play_at(SE_CHATTER_2, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
        if (TASK_TIMER(work) > chao->limit && (motion->flags & MOTION_DONE)) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        if (TASK_TIMER(work) > 120 &&
            alife_target_distance2(work, chao) > 100.0F) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
    if (alife_target_distance2(work, chao) > 3.0F) {
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x = (cosine * 0.023F) - (chao->speed.x * 0.1F);
        chao->accel.z = (-sine * 0.023F) - (chao->speed.z * 0.1F);
    }
    alife_face_look(chao, 0, 0x1000, 2);
    alife_face_set_mouth(chao, 2, 2);
}

void fun_0c9134c0(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * keeps in the frame at sp (0x0c9134d6); without it, 149 of 191 words
     * differ */
    struct chao_motion *motion = &chao->motion;
    Angle ang;
    Float sine;
    Float cosine;

    chao->target = player_work[0]->pos;
    chao->turn_speed = 0x400;
    alife_turn_to_target(work, chao);
    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 11)) {
            return;
        }
        chao->limit = (Uint8)(rand() * (200.0F * RAND_SCALE)) + 180;
        work->submode++;
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 127;
        /* fall through */
    case 1:
        if (chao->sound_timer++ > 20) {
            chao->sound_timer = 0;
            if (rand() * RAND_SCALE > 0.5F) {
                sound_play_at(SE_CHATTER, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            } else {
                sound_play_at(SE_CHATTER_2, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
        if (TASK_TIMER(work) > chao->limit && (motion->flags & MOTION_DONE)) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        if (TASK_TIMER(work) > 120 &&
            alife_target_distance2(work, chao) > 100.0F) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
    if (alife_target_distance2(work, chao) > 3.0F) {
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x = (cosine * 0.023F) - (chao->speed.x * 0.1F);
        chao->accel.z = (-sine * 0.023F) - (chao->speed.z * 0.1F);
    }
}

void fun_0c9136e0(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * keeps in the frame at sp (0x0c9136f6); without it, 159 of 165 words
     * differ */
    struct chao_motion *motion = &chao->motion;
    Angle ang;
    Float sine;
    Float cosine;

    chao->target = player_work[0]->pos;
    chao->turn_speed = 0x400;
    alife_turn_to_target(work, chao);
    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 1)) {
            return;
        }
        if (chao->record->likes[alife_character].like < FOND_LIKE) {
            return;
        }
        chao->limit = (Uint8)(rand() * (200.0F * RAND_SCALE)) + 280;
        work->submode++;
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 132;
        /* fall through */
    case 1:
        if (chao->sound_timer++ > 30) {
            chao->sound_timer = 0;
        }
        if (TASK_TIMER(work) > chao->limit && (motion->flags & MOTION_DONE)) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        /* @hack spelling: the two tests nested, which keeps the walk's
         * 0.023 in FR6 and 0.1 in FR4 (0x0c913834, 0x0c913838); joined with
         * &&, the two trade, and 5 of 165 words differ */
        if (TASK_TIMER(work) > 120) {
            if (alife_target_distance2(work, chao) > 64.0F) {
                work->mode = MODE_IDLE;
                work->submode = 0;
                work->timer = 0;
            }
        }
        break;
    }
    if (alife_target_distance2(work, chao) > 3.0F) {
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x = (cosine * 0.023F) - (chao->speed.x * 0.1F);
        chao->accel.z = (-sine * 0.023F) - (chao->speed.z * 0.1F);
    }
}
