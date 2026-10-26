/* behaviors of motions and voices, most of which a Chao's moods choose */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/face.h"
#include "al_main/icon.h"
#include "al_main/mood.h"
#include "al_main/physics.h"
#include "al_main/status.h"
#include "chao/chao.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

void fun_0c911680(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * forms in R4 at the entry (0x0c91168c) and reads in case 2's test
     * (0x0c9117a4); without it, 150 of 159 words differ */
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case 0:
        chao->limit = (int)(rand() * (200.0F * RAND_SCALE)) + 200;
        work->submode++;
        alife_face_set_eyes(chao, 2, 2);
        /* fall through */
    case 1:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 11;
        chao->icon.kind = ICON_SYMBOL_5;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 60;
        work->submode++;
        if (rand() * RAND_SCALE > 0.5F) {
            sound_play_at(0x346, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            alife_face_set_mouth(chao, 1, 60);
        } else {
            sound_play_at(0x384, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            alife_face_set_mouth(chao, 3, 60);
        }
        break;
    case 2:
        if (motion->flags & MOTION_DONE) {
            if (TASK_TIMER(work) > chao->limit) {
                work->mode = MODE_IDLE;
                work->submode = 0;
                work->timer = 0;
            } else if (chao->sound_timer++ > 80) {
                work->submode--;
                sound_play_at(0x384, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
                chao->sound_timer = 0;
            }
        }
        break;
    }
    alife_drag(work, chao, 0.3F);
}

void alife_hungry(struct task_work *work, struct alife_chao *chao)
{
    Float choice;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 62;
        alife_face_set_eyes(chao, 1, 2);
        alife_face_set_mouth(chao, 6, 2);
        work->submode++;
        choice = rand() * RAND_SCALE;
        if (choice < 0.3F) {
            sound_play_at(0x360, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        } else if (choice < 0.6F) {
            sound_play_at(0x365, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        } else if (choice < 0.9F) {
            sound_play_at(0x38f, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        chao->limit = (int)(rand() * (400.0F * RAND_SCALE)) + 400;
        break;
    case 1:
        alife_face_set_eyes(chao, 1, 2);
        alife_face_set_mouth(chao, 6, 2);
        if (TASK_TIMER(work) > chao->limit) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        if (chao->sound_timer++ > 120 && rand() * RAND_SCALE < 0.005F) {
            choice = rand() * RAND_SCALE;
            if (choice < 0.15F) {
                sound_play_at(0x360, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            } else if (choice < 0.35F) {
                sound_play_at(0x365, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            } else if (choice < 0.5F) {
                sound_play_at(0x38f, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            } else {
                sound_play_at(0x358, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            chao->sound_timer = 0;
        }
        break;
    }
    alife_drag(work, chao, 0.3F);
}

void fun_0c911ac0(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * forms in R4 at the entry (0x0c911aca) and reads in case 3's test
     * (0x0c911bc6); without it, 74 of 139 words differ */
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case 0:
        chao->limit = (int)(rand() * (200.0F * RAND_SCALE)) + 200;
        work->submode++;
        /* fall through */
    case 1:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 75;
        chao->icon.kind = ICON_SYMBOL_5;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 180;
        work->submode++;
        break;
    case 2:
        if (chao->sound_timer++ > 60) {
            if (rand() * RAND_SCALE > 0.5F) {
                sound_play_at(0x346, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            } else {
                sound_play_at(0x347, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            work->submode++;
            chao->sound_timer = 0;
        }
        break;
    case 3:
        if (motion->flags & MOTION_DONE) {
            if (TASK_TIMER(work) > chao->limit) {
                work->mode = MODE_IDLE;
                work->submode = 0;
                work->timer = 0;
            } else {
                work->submode = 0;
            }
        }
        break;
    }
    alife_drag(work, chao, 0.3F);
}

/* @hack redundant: register puts chao in R14 and work in R13 (0x0c911c3a,
 * 0x0c911c3e); without it, the two swap and 28 of 119 words differ */
void fun_0c911c38(struct task_work *work, register struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->limit = (int)(rand() * (200.0F * RAND_SCALE)) + 200;
        work->submode++;
        /* fall through */
    case 1:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 64;
        chao->icon.kind = ICON_SYMBOL_5;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 90;
        work->submode++;
        break;
    case 2:
        if (rand() * RAND_SCALE < 0.003F) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 66;
        }
        if (rand() * RAND_SCALE < 0.01F) {
            sound_play_at(0x367, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        if (TASK_TIMER(work) > chao->limit) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
    alife_drag(work, chao, 0.3F);
}

void fun_0c911d5e(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * forms in R4 at the entry (0x0c911d6a) and reads in case 1's test
     * (0x0c911dde); without it, 44 of 72 words differ */
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 6)) {
            return;
        }
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 68;
        work->submode++;
        sound_play_at(0x36e, 0, 0, -20, work->pos.x, work->pos.y, work->pos.z);
        break;
    case 1:
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

void fun_0c911e14(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 115;
        work->submode++;
        chao->icon.kind = ICON_SYMBOL_3;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 120;
        chao->limit = (int)(rand() * (100.0F * RAND_SCALE)) + 120;
        if (chao->record->curio < 0) {
            sound_play_at(0x348, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        } else {
            sound_play_at(0x355, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
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
    alife_drag(work, chao, 0.3F);
}

void fun_0c911f40(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 84;
        work->submode++;
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
        if (rand() * RAND_SCALE < 0.01F) {
            sound_play_at(0x367, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    }
    alife_face_set_mouth(chao, 0, 2);
    alife_drag(work, chao, 0.3F);
}

void fun_0c912040(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * forms in R4 at the entry (0x0c91204a) and reads in case 1's test
     * (0x0c912094); without it, 87 of 90 words differ */
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 63;
        chao->limit = (int)((rand() * (2.0F * RAND_SCALE)) + 1.0F);
        chao->sound_timer = 0;
        work->submode++;
        break;
    case 1:
        if (motion->flags & MOTION_DONE) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
        }
        if (chao->sound_timer++ == 7) {
            sound_play_at(0x37a, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    }
    alife_face_set_mouth(chao, 0, 2);
    alife_drag(work, chao, 0.3F);
}

/* @hack redundant: register puts chao in R14 and work in R13 (0x0c912122,
 * 0x0c912126); without it, the two swap and 33 of 98 words differ */
void fun_0c912120(struct task_work *work, register struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->icon.kind = ICON_HURT;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 60;
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 70;
        work->submode++;
        chao->limit = 180;
        if (chao->record->agres < 0) {
            sound_play_at(0x38c, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        } else {
            sound_play_at(0x39c, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
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
    alife_face_set_mouth(chao, 2, 2);
    alife_drag(work, chao, 0.3F);
}

void fun_0c91221a(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 82;
        work->submode++;
        chao->limit = (int)(rand() * (600.0F * RAND_SCALE)) + 600;
        if (rand() * RAND_SCALE > 0.5F) {
            sound_play_at(0x35f, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        } else {
            sound_play_at(0x360, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        chao->sound_timer = 0;
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
        if (chao->sound_timer++ > 300) {
            chao->sound_timer = 0;
            /* the product is 0 only for a rand() of 0, which alone picks
             * 0x360 */
            if (rand() * RAND_SCALE != 0.0F) {
                sound_play_at(0x35f, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            } else {
                sound_play_at(0x360, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
        break;
    }
    /* the second call replaces the first's mouth 1, so the Chao shows
     * mouth 5 */
    alife_face_set_mouth(chao, 1, 2);
    alife_face_set_mouth(chao, 5, 2);
    alife_drag(work, chao, 0.3F);
}

void alife_sleepy(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * forms in R4 at the entry (0x0c9123cc) and reads in case 2's test
     * (0x0c912480); without it, 103 of 108 words differ */
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 71;
        work->submode++;
        chao->limit = 250;
        /* the product is 0 only for a rand() of 0, which alone skips the
         * voice */
        if (rand() * RAND_SCALE != 0.0F) {
            sound_play_at(0x36c, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        chao->sound_timer = 0;
        break;
    case 1:
        if (TASK_TIMER(work) > chao->limit) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 71;
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
    alife_face_look(chao, 0x2000, 0, 2);
    alife_face_set_mouth(chao, 3, 2);
    alife_drag(work, chao, 0.3F);
}

void fun_0c9124c4(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * forms in R4 at the entry (0x0c9124d2) and reads in case 1's test
     * (0x0c91258e); without it, 106 of 114 words differ */
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 7)) {
            return;
        }
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 153;
        work->submode++;
        sound_play_at(0x36e, 0, 0, -20, work->pos.x, work->pos.y, work->pos.z);
        chao->unk_0c4 = 0;
        break;
    case 1:
        if (TASK_TIMER(work) == 20) {
            chao->speed.y = 1.0F;
        }
        if (motion->flags & MOTION_DONE) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
            chao->unk_0c4 = 0;
            chao->unk_0b0 = 0.0F;
            chao->unk_0b4 = 0.0F;
            chao->unk_0b8 = 0.0F;
        }
        break;
    }
    /* fly / 2000 divides whole numbers, so a fly below 2000 adds nothing */
    /* NOLINTBEGIN(bugprone-integer-division) */
    chao->unk_0b4 =
        ((Float)(chao->record->skills.fly / 2000) * njCos(chao->unk_0c4)) +
        1.0F;
    /* NOLINTEND(bugprone-integer-division) */
    alife_drag(work, chao, 0.3F);
}

void fun_0c912600(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        if (!status_require_minimal(chao, 3)) {
            return;
        }
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 155;
        work->submode++;
        sound_play_at(0x36e, 0, 0, -20, work->pos.x, work->pos.y, work->pos.z);
        chao->limit = (int)(rand() * (1800.0F * RAND_SCALE)) + 300;
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
    alife_drag(work, chao, 0.3F);
}
