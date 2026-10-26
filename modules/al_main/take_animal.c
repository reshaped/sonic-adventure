/* a Chao that takes an animal from a player, and what the animal gives it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/entry.h"
#include "al_main/icon.h"
#include "al_main/physics.h"
#include "al_main/sparkle.h"
#include "al_main/status.h"
#include "al_main/take_animal.h"
#include "chao/chao.h"
#include "object/common.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

/* three stretches of one motion. the Chao plays them as it goes for the
 * animal, in a loop while it holds the animal up, and as it lets go */
#define MOTION_GO 27
#define MOTION_HOLD 28
#define MOTION_LET_GO 29
/* the motion the Chao plays once it has let go, before mode 13 */
#define MOTION_AFTER 30
#define ICON_STEPS 40 /* how long the Chao's icon shows */
/* how far the Chao turns toward the animal each step */
#define TURN_SPEED 0x1000
/* the work's timer, from the mode's start, past which a Chao going for the
 * animal gives up once it faces it. a Chao that holds the animal lets it go
 * then */
#define TAKE_STEPS 120
/* the timer's steps at which sparkles start over the Chao: a spiral 1 above
 * it, a spiral 3 above it, then glints that gather */
#define LOW_SPIRAL_STEP 20
#define HIGH_SPIRAL_STEP 50
#define GATHER_STEP 80
/* the two voices, one at random, that play as the Chao goes for the
 * animal */
#define SE_GO_VOICE_A 0x363
#define SE_GO_VOICE_B 0x364
/* played as the Chao takes the animal, and again as glints gather */
#define SE_TAKE 0x25f
#define SE_AFTER 0x349 /* the Chao's voice as its let-go motion ends */

/* the work's submode */
enum take_mode { TAKE_START, TAKE_GO, TAKE_HOLD, TAKE_LET_GO, TAKE_AFTER };

Bool alife_try_take_animal(struct task_work *work, struct alife_chao *chao)
{
    struct alife_entry *entry = chao->task->extra;
    struct alife_entry **hit;
    struct alife_entry **end;
    struct task *task;
    Sint8 count;
    /* @hack spelling: the draw kept in a local, so that SHC copies it to FR4
     * before the multiply, FMOV FR3,FR4 at 0x0c91395a; tested in place, 22
     * of 114 words differ */
    Float draw;

    if (work->mode != MODE_TAKE_ANIMAL && (entry->flags & ENTRY_TOUCHING)) {
        count = entry->hit_count;
        end = entry->hits + count;
        for (hit = entry->hits; hit < end; hit++) {
            if ((*hit)->kind == ALIFE_ANIMAL) {
                task = (*hit)->task;
                if (task && task->work) {
                    if (task->work->flags & CARRY_HELD) {
                        if (!(*hit)->held) {
                            entry->held = *hit;
                            work->mode = MODE_TAKE_ANIMAL;
                            work->submode = TAKE_START;
                            work->timer = 0;
                            draw = rand() * RAND_SCALE;
                            if (draw < 0.5F) {
                                sound_play(SE_GO_VOICE_A, 0, 0, 0);
                            } else {
                                sound_play(SE_GO_VOICE_B, 0, 0, 0);
                            }
                            status_raise_like(work, chao, 2);
                            return TRUE;
                        }
                    }
                }
            }
        }
    }
    return FALSE;
}

void alife_take_animal(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the motion's flags read through a pointer of their own,
     * which SHC works out in R4 at the entry, ADD R12,R4 at 0x0c913a06, for the
     * last two cases' tests, as at 0x0c913f12; through chao->motion, 119 of 566
     * words and 10 of the function's 155 words of data differ */
    struct chao_motion *motion = &chao->motion;
    struct alife_entry *entry = chao->task->extra;
    switch (work->submode) {
    case TAKE_START:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = MOTION_GO;
        chao->icon.kind = ICON_GLAD;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = ICON_STEPS;
        chao->turn_speed = TURN_SPEED;
        work->submode++;
        break;
    case TAKE_GO:
        if (entry->held) {
            if (entry->held->task) {
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
        /* the turn left before the step: one short of TURN_SPEED is one the
         * step finishes, which leaves the Chao facing the animal */
        if (alife_turn_to_target(work, chao) < TURN_SPEED) {
            if (entry->flags & ENTRY_TOUCHING) {
                struct alife_entry **hit;
                struct alife_entry **end;
                Sint8 count = entry->hit_count;
                end = entry->hits + count;
                for (hit = entry->hits; hit < end; hit++) {
                    if (*hit == entry->held && !(*hit)->held) {
                        chao->motion.flags |= MOTION_NEW;
                        chao->motion.id = MOTION_HOLD;
                        work->submode++;
                        entry->held->flags |= ENTRY_HELD;
                        entry->held->held = entry;
                        sound_play(SE_TAKE, 0, 0, 0);
                        break;
                    }
                }
                if (TASK_TIMER(work) > TAKE_STEPS) {
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
    case TAKE_HOLD:
        if (!entry->held) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }

        /* @bug memory: with held null, the reset above still goes on to set
         * held's ENTRY_HELD, at 0x0c913bbe */
        entry->held->flags |= ENTRY_HELD;
        {
            /* where the Chao holds the animal, in the Chao's frame, and then
             * where each sparkle starts */
            NJS_POINT3 offset = {2.0F, 2.0F, 0.0F};
            NJS_POINT3 position;
            Angle angle;
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);

            TURN_VIA(njRotateY, work->ang.y, angle);
            njCalcPoint(NULL, &offset, &position);
            njPopMatrix(1);
            if (entry->held->task) {
                entry->held->task->work->pos = position;
            }
            if (TASK_TIMER(work) == LOW_SPIRAL_STEP) {
                offset.x = work->pos.x;
                offset.y = work->pos.y + 1.0F;
                offset.z = work->pos.z;
                alife_sparkle_spiral_create(&offset);
            }
            if (TASK_TIMER(work) == HIGH_SPIRAL_STEP) {
                offset.x = work->pos.x + 0.5F;
                offset.y = work->pos.y + 3.0F;
                offset.z = work->pos.z;
                alife_sparkle_spiral_create(&offset);
            }
            if (TASK_TIMER(work) == GATHER_STEP) {
                offset.x = work->pos.x;
                offset.y = work->pos.y + 3.0F;
                offset.z = work->pos.z;
                alife_sparkle_gather_create(&offset);
                sound_play(SE_TAKE, 0, 0, 0);
            }
        }
        if (TASK_TIMER(work) > TAKE_STEPS) {
            /* the push the animal takes as it is let go, in the Chao's
             * frame */
            NJS_VECTOR velocity = {0.3F, 0.7F, 0.0F};
            NJS_VECTOR rotated;
            NJS_POINT3 effect;
            Uint8 kind;
            Angle angle;
            effect.x = work->pos.x;
            effect.y = work->pos.y + 3.0F;
            effect.z = work->pos.z;
            alife_sparkle_burst_create(&effect);
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = MOTION_LET_GO;
            work->submode++;
            sound_play(SE_CONFIRM, 0, 0, 0);
            if (entry->held) {
                entry->held->flags &= ~ENTRY_HELD;
            }
            njPushMatrix(&_unit_matrix_);

            TURN_VIA(njRotateY, work->ang.y, angle);
            njCalcPoint(NULL, &velocity, &rotated);
            njPopMatrix(1);
            /* @bug memory: with held or its task null, the branches
             * at 0x0c913da0 and 0x0c913da8 skip kind's load, and the calls
             * below take as kind the low byte of what R9 still holds,
             * sound_play's address: 0x68. for a Chao of any type but
             * CHAO_CHAOS, status_give_parts then reads row 104 of
             * child_part_objects or adult_part_objects, which hold 15 rows */
            /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
            if (entry->held) {
                /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
                if (entry->held->task) {
                    entry->held->task->motion->speed = rotated;
                    kind = entry->held->task->work->tick;
                }
            }
            entry->held = NULL;

            if (chao->record->type != CHAO_CHAOS) {
                status_give_parts(work, chao, kind);
            }
            status_give_skills(work, chao, kind);
            status_lean(work, chao, kind);
            status_add_minimal(work, chao, kind);
            {
                NJS_POINT3 center;
                NJS_VECTOR velocity = {0.0F, 0.3F, 0.0F};
                /* @hack spelling: effect written through a pointer of its own,
                 * which SHC keeps in R14, set at 0x0c913e26, for the loop's
                 * stores, as at 0x0c913e56; through effect, 49 of 566 words and
                 * 9 of the function's 155 words of data differ */
                NJS_POINT3 *point;
                Angle phase = 0;
                int i;
                Float cosine;
                Float sine;
                center = work->pos;
                point = &effect;
                point->y = center.y + 2.0F;

                /* sixteen points round the Chao, 3 out and 2 up, and a speed
                 * for each. the loop works them out, and nothing reads
                 * them */
                for (i = 0; i < 16; i++) {
                    cosine = njCos(phase);
                    sine = njSin(phase);
                    point->x = center.x + (cosine * 3.0F);
                    point->z = center.z + (sine * 3.0F);
                    velocity.x = cosine * 0.05F;
                    velocity.z = sine * 0.05F;
                    phase += 0x1000;
                }
            }
        }
        break;
    case TAKE_LET_GO:
        if (motion->flags & MOTION_ENDED) {
            sound_play(SE_AFTER, 0, 0, 0);
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = MOTION_AFTER;
            work->submode++;
        }
        break;
    case TAKE_AFTER:
        if (motion->flags & MOTION_ENDED) {
            work->mode = 13;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
    chao->speed.x *= 0.5F;
    chao->speed.z *= 0.5F;
}
