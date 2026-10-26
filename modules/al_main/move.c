/* how a Chao makes for its target: on foot, flying, swimming or climbing,
 * and the behaviors that send it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/entry.h"
#include "al_main/face.h"
#include "al_main/move.h"
#include "al_main/physics.h"
#include "al_main/status.h"
#include "chao/chao.h"
#include "effect/splash.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

#define SE_HATCH 0x265
#define SE_FLY 0x267
#define SE_PAIR 0x268
#define SE_SPLASH 0x27c
#define SE_CALL 0x390

Bool fun_0c915b00(struct task_work *work, struct alife_chao *chao)
{
    struct alife_entry *entry = chao->task->extra;

    if (work->mode != MODE_GO && work->mode != MODE_GO_TO_PLAYER &&
        work->mode != MODE_GO_2 && work->mode != 10 && work->mode != 31 &&
        work->pos.y + 1.0F < chao->terrain.water_level) {
        chao->unk_112 |= 1;
        chao->target.x = work->pos.x + (njCos(work->ang.y) * 40.0F);
        chao->target.z = work->pos.z - (njSin(work->ang.y) * 40.0F);
        chao->target.y = work->pos.y;
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        chao->last_mode = work->mode;
        work->mode = MODE_GO;
        work->submode = 0;
        work->timer = 0;
        chao->mode_timer = 0;
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

/* not static, though nothing calls it: SHC drops a static function nothing in
 * its file calls */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
int fun_0c915be0(void)
{
    return 1;
}

/* MOVE_WALK. the walk's motion is 51 for a Chao with hp below 200 or life below
 * 20. otherwise it is 46 for a growth below 0.3 with a run below 300, and 158
 * for a run below 3500. past that, it is 49 or 50 at even odds. the Chao speeds
 * up along its facing by its power and run. its run also sets its top speed on
 * x and z. a Chao with a run below 1000 may stop once its timer passes twice
 * its run plus 300, unless its motion is 46 or 47. from then on, each step
 * stops it at a chance of 1000 - run in a million */
static void move_walk(struct task_work *work, struct alife_chao *chao)
{
    Uint16 run = chao->record->skills.run;
    NJS_VECTOR speed;
    Float limit;
    Float magnitude;

    chao->flags |= 0x80;
    chao->turn_speed = 0x200;
    alife_turn_to_target(work, chao);
    switch (work->submode) {
    case 0:
        if (chao->record->growth < 0.3F && run < 300) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 46;
        } else if (run < 3500) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 158;
        } else if (rand() * RAND_SCALE > 0.5F) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 49;
        } else {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 50;
        }
        if (chao->record->hp < 200 || chao->record->life < 20) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 51;
        }
        work->submode++;
        /* fall through */
    case 1: {
        Angle ang = work->ang.y;
        Float sine = njSin(ang);
        Float cosine = njCos(ang);
        Float accel =
            (chao->record->skills.power + run + 10000) * (0.23F / 30000.0F);

        chao->accel.x =
            (accel * cosine) - (chao->speed.x * _builtin_fabsf(sine) * 0.03F);
        /* @hack spelling: the absolute cosine in a statement of its own, which
         * takes FR5 (FABS FR5 at 0x0c915d62); inline in the product, it takes
         * FR4, and 7 of 279 words differ */
        cosine = _builtin_fabsf(cosine);
        chao->accel.z = (-sine * accel) - (chao->speed.z * cosine * 0.03F);
    } break;
    }
    limit = (chao->record->skills.run + 4000) * 0.000006F;
    speed.x = chao->speed.x;
    speed.y = 0.0F;
    speed.z = chao->speed.z;
    magnitude = njScalor(&speed);
    if (magnitude > limit) {
        njUnitVector(&speed);
        speed.x *= limit;
        speed.y *= limit;
        speed.z *= limit;
        chao->speed.x = speed.x;
        chao->speed.z = speed.z;
    }
    chao->terrain.query_height = 4.5F;
    if (run < 1000 && chao->motion.current != 46 &&
        chao->motion.current != 47) {
        if (TASK_TIMER(work) > (run * 2) + 300) {
            if (rand() * RAND_SCALE < (1000 - run) * 0.000001F) {
                work->timer = 0;
                chao->move = MOVE_STOP;
                work->submode = 0;
                chao->mode_timer = 0;
            }
        }
    }
}

/* MOVE_STOP plays motion 1, then motion 2 once its timer passes 240. it
 * sends the Chao back to idle when motion 2 stops at its end. at timer 25
 * it shows alife_newborn's face, with that behavior's sound and a jolt.
 * each step, its accel is -0.2 times its speed on x and z */
static void move_stop(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the flags of case 2 tested through a pointer taken at
     * entry, which puts &chao->motion in R4 before the switch (0x0c915ee8,
     * 0x0c915eee); without it, 74 of 156 words differ */
    struct chao_motion *motion = &chao->motion;

    chao->flags |= 0x80;
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 1;
        work->submode++;
        break;
    case 1:
        if (TASK_TIMER(work) == 25) {
            Sint8 agres = chao->record->agres;

            alife_face_set_eyes(chao, 1, 200);
            if (agres > 50) {
                alife_face_set_mouth(chao, 3, 200);
            } else if (agres > 0) {
                alife_face_set_mouth(chao, 2, 200);
            } else if (agres > -50) {
                alife_face_set_mouth(chao, 5, 200);
            } else {
                alife_face_set_mouth(chao, 6, 200);
            }
            sound_play(SE_HATCH, 0, 0, 0);
            chao->scale_speed = 0.16F;
        }
        if (TASK_TIMER(work) > 240) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 2;
            alife_face_set_eyes(chao, 0, 0);
            chao->shape.objects[NODE_EYE_0]->model->mats[0].attr_texId = 2;
            chao->shape.objects[NODE_EYE_1]->model->mats[0].attr_texId = 2;
            work->submode++;
        }
        break;
    case 2:
        if (motion->flags & MOTION_ENDED) {
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
    chao->accel.x = -(chao->speed.x * 0.2F);
    chao->accel.z = -(chao->speed.z * 0.2F);
}

/* MOVE_FLY, for fly / 2 + 11 steps, then MOVE_DESCEND. the Chao bobs as it
 * flies. while it is more than 5 across from its target and mode_timer is
 * under 600, it climbs or sinks toward 15 above the target. otherwise it
 * makes for 0.5 below the target, a third as fast. its fly raises both
 * rates and its top speed */
static void move_fly(struct task_work *work, struct alife_chao *chao)
{
    Uint16 fly = chao->record->skills.fly;
    Float magnitude;
    Float limit;

    chao->turn_speed = 0x200;
    alife_turn_to_target(work, chao);
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 14;
        chao->terrain.query_height = 4.5F;
        work->submode++;
        chao->limit = (fly / 2) + 10;
        /* fall through */
    case 1: {
        Angle ang = work->ang.y;
        Float sine = njSin(ang);
        Float cosine = njCos(ang);
        Float dx;
        Float dz;
        Float distance2;
        Float lift;

        chao->accel.x =
            (cosine * 0.023F) - (chao->speed.x * _builtin_fabsf(sine) * 0.03F);
        chao->accel.z =
            (-sine * 0.023F) - (chao->speed.z * _builtin_fabsf(cosine) * 0.03F);
        chao->accel.y = 0.05F + (njSin(chao->bob_phase) * 0.01F);
        dx = chao->target.x - work->pos.x;
        dz = chao->target.z - work->pos.z;
        distance2 = (dx * dx) + (dz * dz);
        lift = 0.5F + (fly * 0.00010001F);
        if (distance2 > 25.0F && (Uint16)chao->mode_timer < 600) {
            if (chao->target.y + 15.0F >
                work->pos.y + (njSin(chao->bob_phase) * 2.0F)) {
                work->pos.y += lift * 0.15F;
            } else {
                /* @hack redundant: a never-run store of 0.0 to lift, which
                 * keeps SHC from taking lift * 0.15 once, in FR14, for both
                 * arms: the descent loads 0.15 and multiplies at 0x0c916220 to
                 * 0x0c916228, as the image does; without it, 54 of 266 words
                 * differ */
                if (0) {
                    lift = 0.0F;
                }
                work->pos.y -= lift * 0.15F;
            }
        } else {
            if (chao->target.y - 0.5F >
                work->pos.y + (njSin(chao->bob_phase) * 2.0F)) {
                work->pos.y += lift * 0.05F;
            } else {
                /* @hack spelling: lift scaled in a statement of its own, by
                 * FMUL FR14,FR15 at 0x0c916280 ahead of the subtraction.
                 * nothing reads lift after it; as work->pos.y -= lift * 0.05F,
                 * as the other branch writes it, 179 of 266 words differ */
                lift *= 0.05F;
                work->pos.y -= lift;
            }
        }
        chao->bob_phase += 0x300;
        if (chao->limit-- == 0) {
            chao->move = MOVE_DESCEND;
            work->submode = 0;
            chao->mode_timer = 0;
        }
    } break;
    }
    sound_play_at_for(SE_FLY, (int)chao, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 4);
    magnitude = njScalor(&chao->speed);
    limit = (chao->record->skills.fly + 8000) * 0.00005F;
    if (magnitude > limit) {
        njUnitVector(&chao->speed);
        chao->speed.x *= limit;
        chao->speed.y *= limit;
        chao->speed.z *= limit;
    }
}

/* MOVE_SWIM. the stroke is motion 36 for a swim below 200, and 16 for one below
 * 500. past that, a random draw picks motion 99. its chance is 1 in 10, 3 in 10
 * from a swim of 2500, and 7 in 10 from 5000. otherwise the stroke is 15.
 * motion 99 also needs bit 12 of the record's minimal set, which
 * status_require_minimal tests. where the bit is clear, the stroke is 15, and
 * status_require_minimal sends the Chao back to idle and raises its tedious. a
 * Chao with a swim below 200 also shows eyes 1.
 *
 * the water pushes the Chao toward 1.6 under its surface. each step, a Chao
 * more than 0.6 under the surface gains 0.05 of accel.y, which cancels
 * alife_apply_speed's pull of 0.05. it also gains 0.001 for each unit it lies
 * deeper than 1.6, and loses as much for each unit short of that. a Chao less
 * deep gains 0.046, so it sinks.
 *
 * within 3 under the surface, once its timer passes 30, the Chao makes a ripple
 * at 7 in 100 each step. within 13 of its target, it goes to a place. its swim
 * raises its top speed, and the stroke plays at 4 times its speed */
static void move_swim(struct task_work *work, struct alife_chao *chao)
{
    Uint16 swim = chao->record->skills.swim;
    struct alife_chao *alias;
    Float level;
    Float magnitude;
    Float limit;
    NJS_POINT3 pos;
    NJS_VECTOR growth;

    /* @hack spelling: the level read through alias, a copy of chao, which SHC
     * keeps in R4 and passes alife_turn_to_target in R5 (0x0c916380 to
     * 0x0c91638c); without it, 382 of 390 words differ */
    alias = chao;
    level = alias->terrain.water_level;
    /* @hack redundant: a goto to the next statement, which SHC deletes but
     * which loads 0x100 into R2 and the call's address into R3 (0x0c91637c,
     * 0x0c916386); without it, the two trade and 6 of 390 words differ */
    goto turn;
turn:
    chao->turn_speed = 0x100;
    alife_turn_to_target(work, chao);
    switch (work->submode) {
    case 0:
        if (swim < 200) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 36;
        } else if (swim < 500) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 16;
        } else if (swim < 2500) {
            if (rand() * RAND_SCALE < 0.1F && status_require_minimal(chao, 2)) {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 99;
            } else {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 15;
            }
        } else if (swim < 5000) {
            if (rand() * RAND_SCALE < 0.3F && status_require_minimal(chao, 2)) {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 99;
            } else {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 15;
            }
        } else {
            if (rand() * RAND_SCALE < 0.7F && status_require_minimal(chao, 2)) {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 99;
            } else {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 15;
            }
        }
        work->submode++;
        /* fall through */
    case 1: {
        Angle ang = work->ang.y;
        Float sine = njSin(ang);
        Float cosine = njCos(ang);

        chao->accel.x =
            (cosine * 0.023F) - (chao->speed.x * _builtin_fabsf(sine) * 0.03F);
        chao->accel.z =
            (-sine * 0.023F) - (chao->speed.z * _builtin_fabsf(cosine) * 0.03F);
    } break;
    }
    {
        Float depth = level - work->pos.y - 1.6F;

        if (depth > -1.0F) {
            chao->accel.y += 0.05F + (depth * 0.001F);
        } else {
            chao->accel.y += 0.046F;
        }
        if (chao->speed.y > 1.0F) {
            chao->speed.y = 1.0F;
        } else if (chao->speed.y < -1.0F) {
            chao->speed.y = -1.0F;
        }
        if (work->pos.y < level && work->pos.y + 3.0F > level) {
            if (TASK_TIMER(work) > 30 && rand() * RAND_SCALE > 0.93F) {
                pos.x = work->pos.x - (rand() * (0.2F * RAND_SCALE)) + 0.1F;
                pos.y = level;
                pos.z = work->pos.z - (rand() * (0.2F * RAND_SCALE)) + 0.1F;
                growth.x = 0.02F;
                growth.y = 0.0F;
                growth.z = 0.6F;
                ripple_create(&pos, &growth, 0.5F);
            }
        }
        if (alife_target_distance2(work, chao) < 169.0F) {
            alife_go_to_place(work, chao);
        }
        magnitude = njScalor(&chao->speed);
        limit = (chao->record->skills.swim + 8000) * 0.00001F;
        if (magnitude > limit) {
            njUnitVector(&chao->speed);
            chao->speed.x *= limit;
            chao->speed.y *= limit;
            /* @hack spelling: z's product written out, which loads motions
             * 99 and 15 into R9 and R10 (0x0c916426, 0x0c91642c); as *=, the
             * two trade and 8 of 390 words differ */
            chao->speed.z = chao->speed.z * limit;
        }
        chao->motion.rate = njScalor(&chao->speed) * 4.0F;
        if (swim < 200) {
            alife_face_set_eyes(chao, 1, 2);
        }
    }
}

/* MOVE_DESCEND, once a flight is over. the Chao plays motion 37, adding
 * 0.04 to accel.y each step, until the motion reaches its end. it then plays
 * motion 40 once it is on the ground, and walks once that motion reaches
 * its end */
static void move_descend(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: the flags of cases 1 and 3 tested through a pointer taken
     * at entry, which puts &chao->motion in R6 before the switch
     * (0x0c9167c2, 0x0c9167c6); without it, 67 of 68 words differ */
    struct chao_motion *motion = &chao->motion;

    /* @hack spelling: a return to end the default and cases 1 to 3, where
     * nothing follows the switch, which gives the default and cases 1 and 2 an
     * RTS of their own (0x0c9167d8, 0x0c91680c, 0x0c91682a); as breaks, 3 of 68
     * words differ */
    switch (work->submode) {
    default:
        return;
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 37;
        work->submode++;
        /* fall through */
    case 1:
        if (motion->flags & MOTION_DONE) {
            work->submode++;
        }
        /* the image's word for this float, 0x3d23d709, is one less than
         * 0.04F's, 0x3d23d70a */
        chao->accel.y += 0.039999996F;
        return;
    case 2:
        if (work->flags & CHAO_ON_GROUND) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 40;
            work->submode++;
        }
        return;
    case 3:
        if (motion->flags & MOTION_DONE) {
            chao->move = MOVE_WALK;
            work->submode = 0;
            chao->mode_timer = 0;
        }
        return;
    }
}

/* MOVE_CLIMB. the Chao plays motion 55, moving 0.3 a step along its
 * facing. its speed keeps only its y part, scaled to a rate its power sets.
 * once a floor has cleared 0x80 in chao->flags, the Chao plays motion 56
 * and gains 1 of speed.y. it does the same the next step the flag is clear,
 * and walks once it is on the ground or its timer passes 30 */
static void move_climb(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 55;
        work->submode++;
        /* fall through */
    case 1: {
        Angle ang = work->ang.y;
        Float sine = njSin(ang);
        Float cosine = njCos(ang);
        Float limit;

        work->pos.x += cosine * 0.3F;
        work->pos.z += -sine * 0.3F;
        chao->speed.x = 0.0F;
        chao->speed.z = 0.0F;
        /* the length goes unused (0x0c91692e) */
        njScalor(&chao->speed);
        limit = (chao->record->skills.power + 10000) * 0.00001F;
        /* @hack redundant: a never-run pair of tests of limit, which put the
         * cosine in FR5 and 0.3 in FR4 (0x0c916904, 0x0c916906), as the image
         * has them; without it, the two trade and 11 of 294 words differ */
        if (0) {
            if (limit) {
            }
            if (limit) {
            }
        }
        njUnitVector(&chao->speed);
        chao->speed.x *= limit;
        chao->speed.y *= limit;
        chao->speed.z *= limit;
        if (!(chao->flags & 0x80)) {
            work->submode++;
            work->timer = 0;
            chao->terrain.query_height = 3.0F;
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 56;
            chao->speed.y += 1.0F;
        }
    } break;
    case 2:
        if (!(chao->flags & 0x80)) {
            chao->speed.y += 1.0F;
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 56;
            work->submode++;
        } else {
            work->timer = 0;
        }
        {
            Angle ang = work->ang.y;
            Float sine = njSin(ang);
            Float cosine = njCos(ang);
            Float magnitude;
            Float limit;

            chao->accel.x += (cosine * 0.002F) -
                             (chao->speed.x * _builtin_fabsf(sine) * 0.012F);
            chao->accel.z += (-sine * 0.002F) -
                             (chao->speed.z * _builtin_fabsf(cosine) * 0.012F);
            /* the image's word for this float, 0x3d6147ab, is three less
             * than 0.055F's, 0x3d6147ae */
            chao->accel.y += 0.05499999F;
            magnitude = njScalor(&chao->speed);
            limit = (chao->record->skills.power + 10000) * 0.00001F;
            if (magnitude > limit) {
                njUnitVector(&chao->speed);
                chao->speed.x *= limit;
                chao->speed.y *= limit;
                chao->speed.z *= limit;
            }
        }
        break;
    case 3:
        if (TASK_TIMER(work) > 30 || (work->flags & CHAO_ON_GROUND)) {
            chao->move = MOVE_WALK;
            work->submode = 0;
            chao->mode_timer = 0;
        } else {
            Angle ang = work->ang.y;
            Float sine = njSin(ang);
            Float cosine = njCos(ang);

            chao->accel.x += (cosine * 0.002F) -
                             (chao->speed.x * _builtin_fabsf(sine) * 0.012F);
            chao->accel.z += (-sine * 0.002F) -
                             (chao->speed.z * _builtin_fabsf(cosine) * 0.012F);
        }
        break;
    }
}

/* change the Chao's move as its terrain and target call for. a walker
 * with a fly over 400 flies to a target more than 10 above it. a walker
 * more than 1.5 under water swims, as does a stopped, flying or descending
 * Chao with TERRAIN_DEEP set, each with a splash. a flier on a floor walks.
 * a swimmer less than 1 under the surface goes to a place where it faces a
 * wall. one less than 0.7 under it walks on a floor. a climber whose target
 * lies behind it flies */
static void move_choose(struct task_work *work, struct alife_chao *chao)
{
    NJS_VECTOR facing;
    NJS_VECTOR way;

    switch (chao->move) {
    case MOVE_WALK:
        if (chao->target.y > work->pos.y + 10.0F &&
            chao->record->skills.fly > 400) {
            chao->move = MOVE_FLY;
            work->submode = 0;
            chao->mode_timer = 0;
        }
        if (chao->terrain.water_level > work->pos.y + 1.5F) {
            sound_play_at(SE_SPLASH, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            chao->move = MOVE_SWIM;
            work->submode = 0;
            chao->mode_timer = 0;
        }
        break;
    case MOVE_STOP:
        if (chao->terrain.flags & TERRAIN_DEEP) {
            sound_play_at(SE_SPLASH, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            chao->move = MOVE_SWIM;
            work->submode = 0;
            chao->mode_timer = 0;
        }
        break;
    case MOVE_FLY:
        if (chao->terrain.flags & TERRAIN_FLOOR) {
            chao->move = MOVE_WALK;
            work->submode = 0;
            chao->mode_timer = 0;
        }
        if (chao->terrain.flags & TERRAIN_DEEP) {
            sound_play_at(SE_SPLASH, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            chao->move = MOVE_SWIM;
            work->submode = 0;
            chao->mode_timer = 0;
        }
        break;
    case MOVE_3:
        break;
    case MOVE_SWIM:
        if ((chao->terrain.flags & TERRAIN_FACING_WALL) &&
            work->pos.y > chao->terrain.water_level - 1.0F) {
            alife_go_to_place(work, chao);
        }
        if (work->pos.y + 0.7F > chao->terrain.water_level &&
            (chao->terrain.flags & TERRAIN_FLOOR)) {
            chao->move = MOVE_WALK;
            work->submode = 0;
            chao->mode_timer = 0;
        }
        break;
    case MOVE_DESCEND:
        if (chao->terrain.flags & TERRAIN_DEEP) {
            sound_play_at(SE_SPLASH, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            chao->move = MOVE_SWIM;
            work->submode = 0;
            chao->mode_timer = 0;
        }
        break;
    case MOVE_CLIMB: {
        Angle ang = work->ang.y;

        facing.x = njCos(ang);
        facing.z = -njSin(ang);
        way.x = chao->target.x - work->pos.x;
        way.z = chao->target.z - work->pos.z;
        /* @bug uninitialized: facing.y and way.y are never set. njScalor and
         * njUnitVector read way's from sp+4 (0x0c916da2, 0x0c916dae), and
         * njInnerProduct reads it and facing's from sp+16 (0x0c916db8), as
         * earlier code left those words */
        if (njScalor(&way) != 0.0F) {
            njUnitVector(&way);
        }
        if (njInnerProduct(&way, &facing) < -0.8F) {
            chao->move = MOVE_FLY;
            work->submode = 0;
            chao->mode_timer = 0;
        }
    } break;
    }
}

/* ease the Chao's spin toward 0, by at most 0x800, and step its move.
 * query_height is 4.5 unless the move sets its own, and MOVE_3 does
 * nothing */
static void move_step(struct task_work *work, struct alife_chao *chao)
{
    chao->terrain.query_height = 4.5F;
    chao->spin = angle_approach(chao->spin, 0, 0x800);
    switch (chao->move) {
    case MOVE_WALK:
        move_walk(work, chao);
        break;
    case MOVE_STOP:
        move_stop(work, chao);
        break;
    case MOVE_FLY:
        move_fly(work, chao);
        break;
    case MOVE_3:
        break;
    case MOVE_SWIM:
        move_swim(work, chao);
        break;
    case MOVE_DESCEND:
        move_descend(work, chao);
        break;
    case MOVE_CLIMB:
        move_climb(work, chao);
        break;
    }
}

void alife_go_to_player(struct task_work *work, struct alife_chao *chao)
{
    chao->target = player_work[0]->pos;
    move_choose(work, chao);
    move_step(work, chao);
    alife_face_set_eyes(chao, 2, 2);
    if (alife_target_distance2(work, chao) < 169.0F ||
        (Uint16)chao->mode_timer++ > 1200) {
        work->mode = MODE_IDLE;
        work->submode = 0;
        work->timer = 0;
    }
}

void alife_go(struct task_work *work, struct alife_chao *chao)
{
    move_choose(work, chao);
    move_step(work, chao);
    if (alife_target_distance2(work, chao) < 100.0F ||
        (Uint16)chao->mode_timer++ > 2000) {
        work->mode = MODE_IDLE;
        work->submode = 0;
        work->timer = 0;
    }
}

void alife_join_court(struct task_work *work, struct alife_chao *chao)
{
    struct alife_entry *entry = chao->task->extra;
    struct alife_entry **hit;
    struct alife_entry **end;
    Sint8 count;

    move_choose(work, chao);
    move_step(work, chao);
    if (alife_target_distance2(work, chao) < 100.0F ||
        (Uint16)chao->mode_timer++ > 2000) {
        work->mode = MODE_IDLE;
        work->submode = 0;
        work->timer = 0;
    }
    if (entry->flags & ENTRY_TOUCHING) {
        count = entry->hit_count;
        end = entry->hits + count;
        for (hit = entry->hits; hit < end; hit++) {
            if ((*hit)->kind == ALIFE_HATCHED) {
                if ((*hit)->task->work->mode == MODE_COURT) {
                    if ((*hit)->held == NULL) {
                        entry->held = *hit;
                        (*hit)->held = entry;
                        work->mode = MODE_PARTNER;
                        work->submode = 0;
                        work->timer = 0;
                        sound_play(SE_PAIR, 0, 0, 0);
                    }
                }
            }
        }
    }
}

void fun_0c9170c0(struct task_work *work, struct alife_chao *chao)
{
    move_choose(work, chao);
    move_step(work, chao);
    if (alife_target_distance2(work, chao) < 100.0F ||
        (Uint16)chao->mode_timer++ > 2000) {
        work->mode = MODE_IDLE;
        work->submode = 0;
        work->timer = 0;
    }
}

void fun_0c917140(struct task_work *work, struct alife_chao *chao)
{
    NJS_POINT3 target;
    NJS_VECTOR twice;

    switch (work->submode) {
    case 0:
        sound_play_at(SE_CALL, 0, 0, -20, work->pos.x, work->pos.y,
                      work->pos.z);
        work->submode++;
        break;
    }
    target = player_work[0]->pos;
    twice.x = work->pos.x * 2.0F;
    twice.y = work->pos.y * 2.0F;
    twice.z = work->pos.z * 2.0F;
    /* @bug logic: aims the Chao at player 0's pos less twice its own, a point
     * that moves the other way when both move (JSR at 0x0c9171aa). this
     * point's negation, twice the Chao's pos less player 0's, lies as far past
     * the Chao as player 0 lies before it, and mode 10 is one of behavior.c's
     * afraid_modes. so njSubVector's operands look reversed, an inference from
     * the arithmetic and that list */
    njSubVector(&target, &twice);
    chao->target = target;
    move_choose(work, chao);
    move_step(work, chao);
    if (alife_target_distance2(work, chao) < 225.0F ||
        (Uint16)chao->mode_timer++ > 1200) {
        work->mode = MODE_IDLE;
        work->submode = 0;
        work->timer = 0;
    }
}

void fun_0c917240(struct task_work *work, struct alife_chao *chao)
{
    NJS_POINT3 target;
    NJS_VECTOR twice;

    target = player_work[0]->pos;
    twice.x = work->pos.x * 2.0F;
    twice.y = work->pos.y * 2.0F;
    twice.z = work->pos.z * 2.0F;
    /* @bug logic: aims the Chao at player 0's pos less twice its own, as
     * fun_0c917140 does, a point that moves the other way when both move. so
     * njSubVector's operands look reversed, an inference from the arithmetic
     * alone (JSR at 0x0c917286) */
    njSubVector(&target, &twice);
    chao->target = target;
    move_choose(work, chao);
    move_step(work, chao);
}
