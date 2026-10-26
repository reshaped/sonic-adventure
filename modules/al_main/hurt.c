/* a Chao that's hit */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/entry.h"
#include "al_main/hurt.h"
#include "al_main/icon.h"
#include "al_main/status.h"
#include "collision/collision.h"
#include "player/lockon.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

/* after a hit, alife_take_hit's first HIT_WAIT calls are its cooldown.
 * hit_wait counts the calls since the hit, and is kept to HIT_WAIT_MAX */
#define HIT_WAIT 60
#define HIT_WAIT_MAX 10000

/* a hit takes HIT_HEALTH from hp, down to 0, and adds HIT_FEELING to a
 * feeling, up to FEELING_MAX */
#define HIT_HEALTH 200
#define HIT_FEELING 100
#define FEELING_MAX 200

#define KNOCK_SPEED 1.2F

#define MOTION_FALL 17
#define MOTION_LIE 19
#define MOTION_GET_UP 2
#define ICON_STEPS 180
#define SE_CRY_BOLD 0x385
#define SE_CRY 0x38f
#define SE_THUD 0x35d

/* the spin's turn each step, as the Chao falls and as it settles */
#define HURT_SPIN 0x1000
/* the Chao gets up once its work's timer, in steps from the hit, passes
 * this */
#define HURT_STEPS 200

/* the Chao's work->submode in MODE_HURT */
enum hurt_submode { HURT_START, HURT_FALL, HURT_LIE, HURT_GET_UP };

Bool alife_take_hit(struct task_work *work, struct alife_chao *chao)
{
    struct collision_work *collision = work->collision;
    struct alife_entry *entry = chao->task->extra;
    NJS_POINT3 *from;
    NJS_VECTOR speed;
    Uint16 feeling;
    Angle ang;

    if (chao->hit_wait++ < HIT_WAIT) {
        /* @bug uninitialized: returns no value during the cooldown. R0 keeps
         * the 122 that MOV #122,R0 at 0x0c913f9a loads to read hit_wait, which
         * alife_interrupt takes as TRUE, so it tries nothing after this */
        return; /* NOLINT(clang-diagnostic-return-mismatch) */
    } else {
        if (chao->hit_wait > HIT_WAIT_MAX) {
            chao->hit_wait = HIT_WAIT_MAX;
        }
        if ((work->flags & COLLISION_DAMAGED) || (entry->flags & ENTRY_HIT)) {
            from = &collision->hit->owner->work->pos;
            chao->unk_112 |= 1;
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_HURT;
            work->submode = HURT_START;
            work->timer = 0;
            chao->mode_timer = 0;
            status_hurt(work, chao, 10);
            chao->hit_wait = 0;
            lockon_release(chao->task);
            if (chao->record->hp >= HIT_HEALTH) {
                chao->record->hp -= HIT_HEALTH;
            } else {
                chao->record->hp = 0;
            }
            if (chao->record->agres > 0) {
                if (chao->record->kind > 0) {
                    feeling = chao->record->sorrow + HIT_FEELING;
                    if (feeling > FEELING_MAX) {
                        feeling = FEELING_MAX;
                    }
                    chao->record->sorrow = feeling;
                } else {
                    feeling = chao->record->anger + HIT_FEELING;
                    if (feeling > FEELING_MAX) {
                        feeling = FEELING_MAX;
                    }
                    chao->record->anger = feeling;
                }
            } else {
                feeling = chao->record->fear + HIT_FEELING;
                if (feeling > FEELING_MAX) {
                    feeling = FEELING_MAX;
                }
                chao->record->fear = feeling;
            }
            speed.x = work->pos.x - from->x;
            speed.y = 0;
            speed.z = work->pos.z - from->z;
            /* @bug logic: the test is of |x * z|, not of the length, so
             * a hit from along either axis takes the else's knock, not the
             * one away from the hitter (FMUL, FABS and FCMP/GT at 0x0c914124
             * to 0x0c91412a) */
            if (_builtin_fabsf(speed.x * speed.z) > 0.1F) {
                njUnitVector(&speed);
                speed.x *= KNOCK_SPEED;
                speed.y = KNOCK_SPEED;
                speed.z *= KNOCK_SPEED;
            } else {
                ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
                speed.x = njCos(ang) * KNOCK_SPEED;
                /* @bug logic: KNOCK_SPEED goes to speed.x, over its
                 * cosine, where the other branch gives it to speed.y (FMOV.S
                 * FR15,@R15 at 0x0c914168). the Chao is knocked toward +x and
                 * a random way on z, and not up */
                speed.x = KNOCK_SPEED;
                speed.z = njSin(ang) * KNOCK_SPEED;
            }
            chao->speed = speed;
            if (entry->held != NULL) {
                struct alife_entry *held = entry->held;

                if (held->held == entry) {
                    held->held = NULL;
                }
                entry->held = NULL;
            }
            return TRUE;
        }
        return FALSE;
    }
}

void alife_hurt(struct task_work *work, struct alife_chao *chao)
{
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case HURT_START:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = MOTION_FALL;
        chao->icon.kind = ICON_HURT;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = ICON_STEPS;
        if (chao->record->agres > 0) {
            sound_play(SE_CRY_BOLD, 0, 0, 0);
        } else {
            sound_play(SE_CRY, 0, 0, 0);
        }
        work->submode++;
        break;
    case HURT_FALL:
        if (work->flags & CHAO_ON_GROUND) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = MOTION_LIE;
            work->submode++;
            sound_play(SE_THUD, 0, 0, 0);
        }
        chao->spin += HURT_SPIN;
        break;
    case HURT_LIE:
        chao->spin = angle_approach(chao->spin, 0, HURT_SPIN);
        if (TASK_TIMER(work) > HURT_STEPS) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = MOTION_GET_UP;
            work->submode++;
        }
        break;
    case HURT_GET_UP:
        if (motion->flags & MOTION_ENDED) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
        }
        break;
    }
    if (work->flags & CHAO_ON_GROUND) {
        chao->accel.x = -chao->speed.x * 0.3F;
        chao->accel.z = -chao->speed.z * 0.3F;
    }
}
