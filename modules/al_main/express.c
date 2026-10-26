/* a Chao's mark for its favorite character, and its behaviors of voices and
 * a prop */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/alife.h"
#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/express.h"
#include "al_main/face.h"
#include "al_main/models.h"
#include "al_main/physics.h"
#include "al_main/status.h"
#include "chao/chao.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define SE_CHATTER 0x37f
#define SE_CHATTER_2 0x380
#define SE_VOICE 0x377

struct mark_vars {
    Uint8 unk_00[8];
    /* the mark's work, which lasts as long as the Chao's task: the mark is
     * its child */
    struct task_work *mark;
};
#define VARS(work) ((struct mark_vars *)&(work)->vars)

/* the point fun_0c912daa copies into offset, which nothing reads */
static const NJS_POINT3 prop_offset = {0.0F, 0.0F, 0.0F};

/* the mark: alife_favorite_sprite's frame work->submode, laid on the
 * ground at the angles of work, with work->scl.z as its alpha */
static void favorite_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;

    if (!frame_step) {
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + 0.01F, work->pos.z);

        TURN_VIA(njRotateZ, work->ang.z, angle);
        TURN_VIA(njRotateX, work->ang.x, angle);
        TURN_VIA(njRotateY, work->ang.y, angle);
        njRotateX(NULL, -0x4000);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        draw_set_material(work->scl.z, 1.0F, 1.0F, 1.0F);
        alife_favorite_sprite.p.x = 0.0F;
        alife_favorite_sprite.p.y = 0.0F;
        alife_favorite_sprite.sx = 0.08F;
        alife_favorite_sprite.sy = 0.08F;
        alife_favorite_sprite.ang = 0;
        njFogDisable();
        njDrawSprite3D(&alife_favorite_sprite, work->submode,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njFogEnable();
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

void alife_mark_favorite(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0: {
        struct task *child;
        struct task_work *sprite;
        NJS_POINT3 pos;
        struct angles ang;
        Float height;
        Uint16 affinity;
        int i;
        int selected;
        struct chao_like *relationships;

        if (!status_require_minimal(chao, 14)) {
            return;
        }
        if ((chao->record->curio + 110) * (1.0F / 210.0F) <
            rand() * RAND_SCALE) {
            return;
        }
        if (alife_mark_count > 100) {
            return;
        }
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 101;
        work->submode++;
        alife_mark_count++;
        chao->limit = 500 + (int)(rand() * (300.0F * RAND_SCALE));
        child = task_create_child(TASK_WORK, favorite_draw, chao->task);

        VARS(work)->mark = child->work;
        sprite = child->work;
        sprite->pos.x = work->pos.x + (2.0F * njCos(work->ang.y));
        sprite->pos.z = work->pos.z - (2.0F * njSin(work->ang.y));
        pos.x = sprite->pos.x;
        pos.y = work->pos.y + 3.0F;
        pos.z = sprite->pos.z;
        height = ground_below(pos.x, pos.y, pos.z, &ang) +
                 (rand() * (0.01F * RAND_SCALE));
        sprite->ang.x = ang.x;
        sprite->ang.y = work->ang.y - 0x4000;
        sprite->ang.z = ang.z;
        sprite->pos.y = height;
        sprite->scl.z = 0.0F;
        relationships = chao->record->likes;
        affinity = 0;
        selected = 6;
        for (i = 0; i < 6; i++) {
            if (relationships[i].like > affinity) {
                /* the like is above affinity, which starts at 0, so it is
                 * positive */
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                affinity = relationships[i].like;
                selected = i;
            }
        }

        /* @hack redundant: a never-run test of relationships, which keeps the
         * likes pointer in R5 and the best score in R6 through the selection
         * loop (0x0c91296a, 0x0c912942); without it, the two trade and 33 of
         * 352 words differ */
        if (0) {
            if (relationships) {
            }
        }
        sprite->submode = selected;
    } break;
    case 1:
        if (TASK_TIMER(work) > chao->limit) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
        }
        if (chao->sound_timer++ > 150 && rand() * RAND_SCALE < 0.01F) {
            sound_play_at(SE_VOICE, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            chao->sound_timer = 0;
        }
        if (work->tick > 60) {
            VARS(work)->mark->scl.z += 0.1F;
            work->tick = 0;
        }
        break;
    }
    if (chao->record->curio > 0) {
        alife_face_set_eyes(chao, 2, 2);
        alife_face_set_mouth(chao, 1, 2);
    } else {
        alife_face_set_eyes(chao, 0, 2);
        alife_face_set_mouth(chao, 0, 2);
    }
    alife_drag(work, chao, 0.3F);
}

void fun_0c912ba0(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 10)) {
            return;
        }
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 90;
        work->submode++;
        sound_play_at(0x36e, 0, 0, -20, work->pos.x, work->pos.y, work->pos.z);
        chao->limit = (int)(rand() * (300.0F * RAND_SCALE)) + 300;
        break;
    case 1:
        if (TASK_TIMER(work) > chao->limit) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
        }
        break;
    }
    alife_face_look(chao, 0x4000, 0, 2);
    alife_drag(work, chao, 0.3F);
}

void fun_0c912ca8(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * forms in R4 at the entry (0x0c912cb4, 0x0c912cba); without it, 76 of
     * 107 words differ */
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 5;
        work->submode++;
        sound_play_at(0x388, 0, 0, -20, work->pos.x, work->pos.y, work->pos.z);
        chao->limit = (int)(rand() * (600.0F * RAND_SCALE)) + 600;
        break;
    case 1:
        if (TASK_TIMER(work) > chao->limit) {
            sound_play_at(0x388, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 5;
            work->submode++;
        }
        break;
    case 2:
        if (motion->flags & MOTION_DONE) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
        }
        break;
    }
    alife_drag(work, chao, 0.3F);
}

void fun_0c912daa(struct task_work *work, struct alife_chao *chao)
{
    struct chao_motion *motion;
    struct chao_motion_entry *entry;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 offset;
    NJS_ACTION action;
    NJS_MATRIX matrix;
    Angle angle;

    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 12)) {
            return;
        }
        if ((chao->record->curio + 110) * (1.0F / 210.0F) <
            rand() * RAND_SCALE) {
            return;
        }
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 76;
        work->submode++;
        chao->limit = (int)(rand() * (300.0F * RAND_SCALE)) + 500;
        break;
    case 1:
        if (chao->limit-- < 1) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
        }
        if (TASK_TIMER(work) > 60 && rand() * RAND_SCALE < 0.01F) {
            work->timer = 0;
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 77;
            sound_play_at(0x371, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        if (chao->sound_timer++ > 150 && rand() * RAND_SCALE < 0.01F) {
            sound_play_at(SE_VOICE, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            chao->sound_timer = 0;
        }
        motion = &chao->motion;
        entry = &motion->table[motion->current];
        /* @hack redundant: offset set and never read, a copy of
         * prop_offset that the image makes at sp+76 (0x0c912f5e to
         * 0x0c912f86); without it, 132 of 289 words differ */
        offset = prop_offset;
        action.object = chao->shape.model;
        action.motion = entry->motion;
        pose_action(&_unit_matrix_, &action, motion->frame, 7, &matrix);
        njPushMatrix(NULL);
        njSetTexture(&alife_chao_texlist);
        njTranslateV(NULL, &work->pos);

        TURN_VIA(njRotateY, work->ang.y, angle);
        njRotateY(NULL, 0x4000);
        njMultiMatrix(NULL, &matrix);
        njTranslate(NULL, 0.0F, 0.5F, -0.5F);
        draw_model(&dat_0ca50650);
        njPopMatrix(1);
        break;
    }
    if (chao->record->curio > 0) {
        alife_face_set_eyes(chao, 2, 2);
        alife_face_set_mouth(chao, 1, 2);
    } else {
        alife_face_set_eyes(chao, 0, 2);
        alife_face_set_mouth(chao, 0, 2);
    }
    alife_drag(work, chao, 0.3F);
}

void fun_0c9130c6(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * keeps in the frame at sp (0x0c9130d8); without it, the function runs
     * 34 bytes short, and 134 of 135 words differ */
    struct chao_motion *motion = &chao->motion;

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
        break;
    }
    alife_face_look(chao, 0, 0x1000, 2);
    alife_face_set_mouth(chao, 2, 2);
}
