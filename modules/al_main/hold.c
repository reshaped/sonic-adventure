/* a Chao that picks a fruit up and eats it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/entry.h"
#include "al_main/face.h"
#include "al_main/hold.h"
#include "al_main/icon.h"
#include "al_main/physics.h"
#include "al_main/status.h"
#include "object/common.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

struct hold_vars {
    Uint8 unk_00[4];
    Sint32 unk_04; /* 60 to 239 at random, set at the pick-up and never read */
};

#define VARS(work) ((struct hold_vars *)&(work)->vars)

#define MOTION_GO 27
#define MOTION_PICK_UP 57
#define MOTION_PUT_DOWN 61
#define ICON_STEPS 40
#define SE_TAKE 0x268
#define SE_HOLD 0x37e

/* the Chao picks the fruit up only while it faces it within this */
#define HOLD_FACING 0x1000
/* these count steps of the work's timer, which alife_try_hold starts at 0
 * and nothing restarts. a Chao that faces the fruit past HOLD_REACH_STEPS
 * gives up, and one that holds it past HOLD_STEPS lets go. the put-down ends
 * at its motion's end or once the timer passes PUT_DOWN_STEPS. a put-down
 * that starts past PUT_DOWN_STEPS so ends in its first step */
#define HOLD_REACH_STEPS 120
#define HOLD_STEPS 1200
#define PUT_DOWN_STEPS 200

/* the Chao bites the fruit each time its work's tick passes BITE_TICKS, and
 * then starts the tick over. chao_step counts tick up too, so a bite comes
 * every 91 steps */
#define BITE_TICKS 180

/* letting go of the fruit adds HOLD_PLEASURE to the record's pleasure, up to
 * PLEASURE_MAX, whether or not the Chao has eaten it */
#define HOLD_PLEASURE 100
#define PLEASURE_MAX 200

/* the Chao's work->submode in MODE_HOLD */
enum hold_submode { HOLD_START, HOLD_GO, HOLD_HOLD, HOLD_PUT_DOWN };

/* where the fruit is held, in front of the Chao */
static const NJS_POINT3 hold_offset = {2.0F, 1.2F, 0.0F};
/* 12 bytes the same as hold_offset's. no word in AL_MAIN, AL_RACE, the
 * gardens or 1ST_READ.BIN names them. their first word starts off 8, at
 * 0x0cb201cc, so it is this object's. the last 8 start on 8, so they could
 * also start another object, as splits.txt's note on hold.c's constants
 * says. SHC keeps a static that nothing reads, so this one holds them in
 * place */
static const NJS_POINT3 dat_0cb201cc = {
#include "al_main/hold/dat_0cb201cc.inc"
};

Bool alife_try_hold(struct task_work *work, struct alife_chao *chao)
{
    struct alife_entry *entry = chao->task->extra;
    struct alife_entry **hit;
    struct alife_entry **end;
    struct task *task;
    Sint8 count;

    if (work->mode != MODE_HOLD && (entry->flags & ENTRY_TOUCHING)) {
        count = entry->hit_count;
        end = entry->hits + count;
        for (hit = entry->hits; hit < end; hit++) {
            if ((*hit)->kind == ALIFE_FRUIT) {
                task = (*hit)->task;
                if (task != NULL && task->work != NULL) {
                    /* @hack redundant: the same take in both arms of a test of
                     * CARRY_HELD. SHC keeps the test, a TST at 0x0c914938,
                     * and both arms; without it, the object ends 32 bytes
                     * early and 88 of 88 words differ */
                    if (task->work->flags & CARRY_HELD) {
                        if ((*hit)->held == NULL) {
                            entry->held = *hit;
                            work->mode = MODE_HOLD;
                            work->submode = HOLD_START;
                            work->timer = 0;
                            sound_play(SE_TAKE, 0, 0, 0);
                            return TRUE;
                        }
                    } else {
                        if ((*hit)->held == NULL) {
                            entry->held = *hit;
                            work->mode = MODE_HOLD;
                            work->submode = HOLD_START;
                            work->timer = 0;
                            sound_play(SE_TAKE, 0, 0, 0);
                            return TRUE;
                        }
                    }
                }
            }
        }
    }
    return FALSE;
}

void alife_hold(struct task_work *work, struct alife_chao *chao)
{
    struct chao_motion *motion = &chao->motion;
    struct alife_entry *entry = chao->task->extra;
    struct alife_entry **hit;
    struct alife_entry **end;
    NJS_POINT3 offset;
    NJS_POINT3 pos;
    Angle ang;
    Sint8 count;
    Sint8 kind;
    Uint16 pleasure;

    switch (work->submode) {
    case HOLD_START:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = MOTION_GO;
        chao->icon.kind = ICON_GLAD;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = ICON_STEPS;
        chao->turn_speed = 0x1000;
        entry->flags &= ~ENTRY_LET_GO;
        work->submode++;
        break;
    case HOLD_GO:
        if (entry->held != NULL) {
            if (entry->held->task != NULL) {
                chao->target = entry->held->task->work->pos;
            } else {
                entry->held = NULL;
                work->mode = MODE_IDLE;
                work->submode = 0;
                work->timer = 0;
            }
        } else {
            entry->held = NULL;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        if (alife_turn_to_target(work, chao) < HOLD_FACING) {
            if (entry->flags & ENTRY_TOUCHING) {
                count = entry->hit_count;
                end = entry->hits + count;
                for (hit = entry->hits; hit < end; hit++) {
                    if (*hit == entry->held && (*hit)->held == NULL) {
                        chao->motion.flags |= MOTION_NEW;
                        chao->motion.id = MOTION_PICK_UP;
                        work->submode++;
                        entry->held->flags |= ENTRY_HELD;
                        entry->held->held = entry;
                        VARS(work)->unk_04 =
                            (Uint8)(rand() * (180.0F * RAND_SCALE)) + 60;
                        sound_play_at(SE_HOLD, 0, 0, -20, work->pos.x,
                                      work->pos.y, work->pos.z);
                        break;
                    }
                }
                if (TASK_TIMER(work) > HOLD_REACH_STEPS) {
                    entry->held = NULL;
                    work->mode = MODE_IDLE;
                    work->submode = 0;
                    work->timer = 0;
                }
            } else {
                entry->held = NULL;
                work->mode = MODE_IDLE;
                work->submode = 0;
                work->timer = 0;
            }
        }
        break;
    case HOLD_HOLD:
        if (entry->held == NULL) {
            entry->held = NULL;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        offset = hold_offset;
        if (rand() * RAND_SCALE < 0.002F) {
            sound_play_at(SE_HOLD, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcPoint(NULL, &offset, &pos);
        njPopMatrix(1);
        /* @bug memory: the reset above leaves the step to go on, and with held
         * NULL, this store reads the fruit's task through it (MOV.L
         * @(32,R3),R2 at 0x0c914c32) */
        entry->held->task->work->pos = pos;
        if ((entry->flags & ENTRY_LET_GO) || TASK_TIMER(work) > HOLD_STEPS) {
            kind = entry->held->task->work->tick;
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = MOTION_PUT_DOWN;
            work->submode++;
            entry->held->flags &= ~ENTRY_HELD;
            entry->held = NULL;
            pleasure = chao->record->pleasure + HOLD_PLEASURE;
            if (pleasure > PLEASURE_MAX) {
                pleasure = PLEASURE_MAX;
            }
            chao->record->pleasure = pleasure;
            status_eat_fruit(work, chao, kind);
            entry->flags &= ~ENTRY_LET_GO;
            status_raise_like(work, chao, 3);
        } else if (work->tick++ > BITE_TICKS) {
            work->tick = 0;
            if (entry->held != NULL) {
                entry->held->flags |= ENTRY_LET_GO;
            } else {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = MOTION_PUT_DOWN;
                work->submode++;
            }
        }
        break;
    case HOLD_PUT_DOWN:
        if ((motion->flags & MOTION_ENDED) ||
            TASK_TIMER(work) > PUT_DOWN_STEPS) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
    alife_face_set_eyes(chao, 2, 2);
    chao->speed.x *= 0.5F;
    chao->speed.z *= 0.5F;
}
