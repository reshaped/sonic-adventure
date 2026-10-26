/* a Chao that plays the trumpet, plays the drum, or sings */
#include <ninja.h>
#include <stdlib.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/empty.h"
#include "al_main/face.h"
#include "al_main/models.h"
#include "al_main/music.h"
#include "al_main/physics.h"
#include "al_main/status.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

/* each copied to a local that nothing reads */
static const NJS_POINT3 trumpet_offset = {0.0F, 0.0F, 0.0F};
static const NJS_POINT3 drum_offset = {0.0F, 0.0F, 0.0F};

#define SE_TRUMPET 0x36d
#define SE_TRUMPET_2 0x39b
#define SE_DRUM 0x28c
#define SE_SING 0x35c
#define SE_SING_2 0x361

void alife_trumpet(struct task_work *work, struct alife_chao *chao)
{
    struct chao_motion *motion = &chao->motion;
    struct chao_motion_entry *entry;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 offset;
    NJS_ACTION action;
    NJS_MATRIX matrix;
    Angle ang;

    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 13)) {
            return;
        }
        chao->limit = (int)(rand() * (800.0F * RAND_SCALE)) + 500;
        work->submode++;
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 41;
        fun_0c90ca24(work, chao, 7);
        /* fall through */
    case 1:
        if (chao->sound_timer++ > 40 && rand() * RAND_SCALE < 0.2F) {
            if (rand() * RAND_SCALE > 0.5F) {
                sound_play_at(SE_TRUMPET, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            } else {
                sound_play_at(SE_TRUMPET_2, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            alife_face_set_eyes(chao, 2, 20);
            chao->sound_timer = 0;
        }
        /* @hack redundant: motion set again to the address it holds, which
         * reloads &chao->motion into R12 (0x0c917428, 0x0c917430); without
         * it, 209 of 252 words differ */
        motion = &chao->motion;
        entry = &motion->table[motion->current];
        /* @hack redundant: offset, set from trumpet_offset and never read,
         * which the image copies into the frame (0x0c91742c to 0x0c917454);
         * without it, 126 of 252 words differ */
        offset = trumpet_offset;
        action.object = chao->shape.model;
        action.motion = entry->motion;
        pose_action(&_unit_matrix_, &action, motion->frame, 26, &matrix);
        njPushMatrix(NULL);
        njSetTexture(&alife_chao_texlist);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njRotateY(NULL, 0x4000);
        njMultiMatrix(NULL, &matrix);
        njTranslate(NULL, 0.0F, -0.2F, -0.5F);
        draw_model(&alife_trumpet_model);
        njPopMatrix(1);
        if (TASK_TIMER(work) > chao->limit) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 6;
            work->submode++;
        }
        break;
    case 2:
        if (motion->flags & MOTION_DONE) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            fun_0c90ca28(work, chao);
        }
        break;
    }
    alife_face_set_mouth(chao, 0, 2);
    alife_drag(work, chao, 0.3F);
}

void alife_drum(struct task_work *work, struct alife_chao *chao)
{
    struct chao_motion *motion;
    struct chao_motion_entry *entry;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 offset;
    NJS_ACTION action;
    NJS_MATRIX matrix;
    Angle ang;

    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 9)) {
            return;
        }
        chao->limit = (int)(rand() * (200.0F * RAND_SCALE)) + 200;
        work->submode++;
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 138;
        fun_0c90ca20(work, chao, 4);
        fun_0c90ca24(work, chao, 6);
        /* fall through */
    case 1:
        if (chao->sound_timer++ > 60) {
            work->submode++;
        }
        break;
    case 2:
        if (chao->sound_timer++ > 40 && rand() * RAND_SCALE < 0.2F) {
            sound_play_at(SE_DRUM, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            alife_face_set_eyes(chao, 2, 20);
            chao->sound_timer = 0;
        }
        motion = &chao->motion;
        entry = &motion->table[motion->current];
        /* @hack redundant: offset, set from drum_offset and never read,
         * which the image copies into the frame (0x0c9176d6 to 0x0c9176fe);
         * without it, 152 of 269 words differ */
        offset = drum_offset;
        action.object = chao->shape.model;
        action.motion = entry->motion;
        pose_action(&_unit_matrix_, &action, motion->frame, 4, &matrix);
        njSetTexture(&alife_chao_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njPushMatrix(NULL);
        njRotateY(NULL, 0x4000);
        njMultiMatrix(NULL, &matrix);
        njTranslate(NULL, 0.0F, 0.0F, -0.5F);
        draw_model(&dat_0ca4cc08);
        njPopMatrix(1);
        njTranslate(NULL, 1.5F, 1.5F, 0.3F);
        njRotateY(NULL, 0x4000);
        draw_model(&alife_drum_model);
        njPopMatrix(1);
        if (TASK_TIMER(work) > chao->limit) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            fun_0c90ca28(work, chao);
        }
        break;
    }
    if (chao->record->kind > 0) {
        alife_face_set_eyes(chao, 2, 2);
        alife_face_set_mouth(chao, 1, 2);
    } else {
        alife_face_set_mouth(chao, 2, 2);
    }
    alife_drag(work, chao, 0.3F);
}

void alife_sing(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 5)) {
            return;
        }
        chao->limit = (int)(rand() * (800.0F * RAND_SCALE)) + 200;
        work->submode++;
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 85;
        sound_play_at(SE_SING, 0, 0, -20, work->pos.x, work->pos.y,
                      work->pos.z);
        chao->sound_timer = 0;
        /* fall through */
    case 1:
        if (chao->sound_timer++ > 180 && rand() * RAND_SCALE < 0.02F) {
            if (rand() * RAND_SCALE > 0.5F) {
                sound_play_at(SE_SING, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            } else {
                sound_play_at(SE_SING_2, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            chao->sound_timer = 0;
        }
        if (TASK_TIMER(work) > chao->limit) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
    alife_face_set_eyes(chao, 2, 1);
    work->tick++;
    if (work->tick < 40) {
        alife_face_set_mouth(chao, 3, 2);
    } else if (work->tick < 80) {
        alife_face_set_mouth(chao, 0, 2);
    } else {
        work->tick = 0;
    }
    alife_drag(work, chao, 0.3F);
}
