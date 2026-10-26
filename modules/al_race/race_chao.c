/* a Chao in the race: its task, the gains its record takes, the modes it
 * moves in, how the ground, water and walls hold it, and its voice */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/chao.h"
#include "al_main/face.h"
#include "al_main/icon.h"
#include "al_main/motion.h"
#include "al_main/physics.h"
#include "al_main/shape.h"
#include "al_main/status.h"
#include "al_race/course.h"
#include "al_race/game.h"
#include "al_race/item.h"
#include "al_race/race_chao.h"
#include "al_race/racer.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "collision/polygon.h"
#include "debug/editor.h"
#include "draw/pose.h"
#include "effect/splash.h"
#include "sound/cue.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the sound of a Chao's first voice. the tables below number each voice
 * from it: voice n is sound FIRST_VOICE + n */
#define FIRST_VOICE 838
#define VOICES 87 /* how many voices a Chao has, from FIRST_VOICE on */

/* the modes' sounds, each heard only while the race follows the Chao's
 * lane */
#define SE_STEP 0x262 /* a step, each time the Chao has gone 4 more */
#define SE_REST 892 /* as a rest starts to slow */
#define SE_PAUSE_SLEEP 858 /* every 61 steps of MODE_PAUSE_SLEEP's pause */
#define SE_PAUSE_AGRES 890 /* once in MODE_PAUSE_AGRES's pause */
#define SE_SWING 0x356 /* the swing's second motion */
#define SE_DANCE 839 /* the first of the three a dance picks among */
#define SE_SPLASH 0x2cd

/* the voices race_chao_voice gives by mood, the lowest mood first.
 * race_chao_moods_1 is for requests 1 to 3, and race_chao_moods_4 for 4, 10
 * and 11 */
static Uint8 race_chao_moods_1[13] = {8,  0,  35, 14, 11, 12, 28,
                                      44, 80, 66, 65, 61, 62};
static Uint8 race_chao_moods_4[12] = {83, 36, 72, 73, 46, 47,
                                      48, 24, 32, 23, 31, 9};
/* the voices it picks among at random for request 17 */
static Uint8 race_chao_voices_17[7] = {22, 23, 26, 47, 56, 70, 82};

static struct collision_shape race_chao_shapes[1] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS,
     PUSH_ENEMIES | PUSH_OBJECTS,
     {0.0F, 2.8F, 0.0F},
     2.3F,
     2.8F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the motions a Chao picks among as it waits at the start */
static const Uint8 race_chao_idles[5] = {7, 8, 9, 5, 6};
/* how far the racer in first place must run, by the race's kind, before a
 * RACER_LATE_START racer leaves the start */
static const Float race_chao_late_start[5] = {160.0F, 670.0F, 2000.0F, 2550.0F,
                                              1300.0F};
static const struct sound_falloff race_chao_falloff = {0, -50, 20.0F, 1000.0F};

/* what each step takes from a racing Chao's speed along y */
#define CHAO_GRAVITY 0.05F
/* a wall whose normal dips more than this below level leans out, and sets
 * bit 0 of the racer's overhang */
#define OVERHANG_ANGLE NJM_DEG_ANG(15)
/* race_chao_water_level's level where it finds no water */
#define NO_WATER (-100000.0F)

static void race_chao_sound(struct task_work *work, int se);
static Float race_chao_take_skill_value(struct racer *racer, Float skill,
                                        Float low, Float high);
static void race_chao_ready(struct task_work *work, struct alife_chao *chao);
static void race_chao_pose(struct task_work *work, struct alife_chao *chao,
                           int motion);
static void race_chao_obey(struct task_work *work, struct alife_chao *chao);
static void race_chao_splash(struct task_work *work, struct alife_chao *chao);
static void race_chao_wake(struct task_work *work, struct alife_chao *chao);
static void race_chao_ripple(struct task_work *work, struct alife_chao *chao,
                             Float level);
static Float race_chao_water_level(struct task_work *work,
                                   struct alife_chao *chao);
static void race_chao_voice(struct task_work *work, struct alife_chao *chao);
static void race_chao_loop_sound(struct task_work *work,
                                 struct alife_chao *chao);

/* add the racer's gains to its Chao's record, each on two steps of every
 * 64 that the racer's gain_timer counts:
 *
 *   sleep_gain   to sleep, on steps 0 and 30
 *   unk_64_gain  to unk_64, on steps 10 and 40
 *   unk_65_gain  to unk_65, on steps 15 and 45
 *
 * a gain below 0 is raised to 0 first. an added gain is cleared, and a sum
 * over 200 is cut to 200 */
static void race_chao_add_gains(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    Uint16 tick;

    ++racer->gain_timer;
    if (racer->sleep_gain < 0) {
        racer->sleep_gain = 0;
    }
    if (racer->unk_64_gain < 0) {
        racer->unk_64_gain = 0;
    }
    if (racer->unk_65_gain < 0) {
        racer->unk_65_gain = 0;
    }
    if (racer->gain_timer == 0xffff) {
        racer->gain_timer = 0;
    }
    tick = racer->gain_timer & 63;
    /* @hack redundant: the tests of sleep, unk_64 and unk_65 below 0, which
     * never hold: each gain is raised to 0 first (0x0cb81810 to 0x0cb8182c),
     * and each field is unsigned. they keep the CMP/PZ at 0x0cb818ca,
     * 0x0cb8190e and 0x0cb8198a; without them, count unmeasured */
    switch (tick) {
    case 0:
    case 30:
        chao->record->sleep += racer->sleep_gain;
        racer->sleep_gain = 0;
        if (chao->record->sleep > 200) {
            chao->record->sleep = 200;
        } else if (chao->record->sleep < 0) {
            chao->record->sleep = 0;
        }
        break;
    case 10:
    case 40:
        chao->record->unk_64 += racer->unk_64_gain;
        racer->unk_64_gain = 0;
        if (chao->record->unk_64 > 200) {
            chao->record->unk_64 = 200;
        } else if (chao->record->unk_64 < 0) {
            chao->record->unk_64 = 0;
        }
        break;
    case 15:
    case 45:
        chao->record->unk_65 += racer->unk_65_gain;
        racer->unk_65_gain = 0;
        if (chao->record->unk_65 > 200) {
            chao->record->unk_65 = 200;
        } else if (chao->record->unk_65 < 0) {
            chao->record->unk_65 = 0;
        }
        break;
    }
}

/* set the racer's in_zone to 1 while what the Chao's collision hits is a
 * small zone, else to 0 */
static void race_chao_set_in_zone(struct task_work *work,
                                  struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;

    if (work->collision->flags & COLLISION_HIT) {
        if (zone_get_size(work->collision->hit) == ZONE_SMALL) {
            racer->in_zone = 1;
            return;
        }
    }
    racer->in_zone = 0;
}

/* add accel to speed, less CHAO_GRAVITY along y, then add speed to pos and
 * clear accel. while the racer is RACER_TURNING, the Chao keeps its speed along
 * x and z and moves along y alone */
static void race_chao_integrate(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;

    chao->speed.y += chao->accel.y - CHAO_GRAVITY;
    work->pos.y += chao->speed.y;
    chao->accel.y = 0.0F;
    if (racer->flags & RACER_TURNING) {
        chao->accel.x = 0.0F;
        chao->accel.z = 0.0F;
        return;
    }
    chao->speed.x += chao->accel.x;
    chao->speed.z += chao->accel.z;
    work->pos.x += chao->speed.x;
    work->pos.z += chao->speed.z;
    chao->accel.x = 0.0F;
    chao->accel.z = 0.0F;
}

/* run along the heading, with a motion by the Chao's run and fly skills and
 * its kind. its speed is at most race_chao_take_skill_value's for run, from
 * 0.25 to 0.5. after 90 steps the Chao weaves across its heading. on a rise,
 * the push along the heading loses up to half of itself, and less the higher
 * the power skill. after 300 steps the Chao may tumble, the likelier the nearer
 * it is to first place. after 60 steps, a record's sleep over 199 is cleared,
 * and the Chao takes MODE_PAUSE_SLEEP. a record's unk_65 over 199 is cleared
 * too. the Chao then takes MODE_PAUSE_AGRES for an agres above 0, or else
 * MODE_PAUSE_MILD */
static void race_chao_run_grown(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    Angle ang;
    Float sine;
    Float cosine;
    Float drag;
    Float rate;
    Float speed;
    Float top;
    int chance;

    switch (work->submode) {
    case 0:
        if (chao->record->skills.run > 3500) {
            if (chao->record->skills.fly > 3500) {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 10;
                racer->run_motion = 10;
            } else {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 49;
                racer->run_motion = 49;
            }
        } else if (chao->record->kind > 0) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 48;
            racer->run_motion = 48;
        } else {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 0;
            racer->run_motion = 0;
        }
        chao->terrain.query_height = 7.0F;
        ++work->submode;
        work->timer = 0;
        work->tick = 0;
        /* fall through */
    case 1:
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x +=
            (cosine * 0.023F) - (chao->speed.x * njAbs(sine) * 0.023F);
        chao->accel.z +=
            (-sine * 0.023F) - (chao->speed.z * njAbs(cosine) * 0.023F);
        if (TASK_TIMER(work) > 90.0F) {
            chao->accel.x += njSin(racer->phase) * sine * 0.01F;
            chao->accel.z += njSin(racer->phase) * cosine * 0.01F;
        }
        chance = (int)RANDOM(0.0F, 1000.0F);
        if (TASK_TIMER(work) > 300 && chance < 15 - (racer_place(racer) * 2)) {
            SET_MODE(work, MODE_KOKE);
            break;
        }
        if (TASK_TIMER(work) > 60) {
            if (chao->record->sleep > 199) {
                chao->record->sleep = 0;
                SET_MODE(work, MODE_PAUSE_SLEEP);
                break;
            }
            if (chao->record->unk_65 > 199) {
                chao->record->unk_65 = 0;
                /* @hack redundant: a test of agres that never runs, which
                 * gives the record R3 and agres R2 (0x0cb81ce8, 0x0cb81cec)
                 * and the mode R2 (0x0cb81cf4 to 0x0cb81cfa); without it, the
                 * two trade and 6 of 592 words differ */
                if (0) {
                    if (chao->record->agres > 0) {
                    }
                }
                work->mode = chao->record->agres > 0 ? MODE_PAUSE_AGRES
                                                     : MODE_PAUSE_MILD;
                work->submode = 0;
                work->timer = 0;
                /* @hack redundant: an empty test that never runs, which keeps
                 * the timer's reset in this tail's own BRA slot (0x0cb81cfe,
                 * 0x0cb81d00); without it, the tail branches to the sleep's
                 * at 0x0cb81ccc, 265 of 592 words differ and the function
                 * runs 32 bytes short */
                if (0) {
                }
                /* @hack spelling: a return where a break would do, which reads
                 * sleep_gain into R3 at 0x0cb81d44; as a break, R2, and 2 of
                 * 592 words differ */
                return;
            }
        }
        if (racer->sleep_gain < 1) {
            racer->sleep_gain = 1;
        }
        if (racer->unk_64_gain < 1) {
            racer->unk_64_gain = 1;
        }
        drag = racer->rise * 10.0F;
        if (drag > 0.5F) {
            drag = 0.5F;
        }
        drag -= chao->record->skills.power * 0.000025F;
        if (drag > 0.0F) {
            drag = 1.0F - drag;
            chao->accel.x *= drag;
            chao->accel.z *= drag;
            if (racer->unk_64_gain < 2) {
                racer->unk_64_gain = 2;
            }
            /* nothing reads this draw, but it moves rand's sequence on */
            rand();
        }
        if (work->tick > 120.0F) {
            if (racer->rise > 0.0F) {
                racer->voice = 5;
            } else {
                racer->voice = 1;
            }
            work->tick = (int)RANDOM(0.0F, 10.0F);
        }
        speed = njScalor(&chao->speed);
        top = race_chao_take_skill_value(racer, chao->record->skills.run, 0.25F,
                                         0.5F);
        if (speed > top) {
            speed = top;
            njUnitVector(&chao->speed);
            chao->speed.x *= speed;
            chao->speed.y *= speed;
            chao->speed.z *= speed;
        }
        switch (racer->run_motion) {
        case 0:
            rate = 0.5F;
            break;
        case 48:
            rate = 2.7F;
            break;
        case 10:
            rate = 2.0F;
            break;
        case 49:
            rate = 2.0F;
            break;
        default:
            rate = 1.0F;
            break;
        }
        chao->motion.rate = rate * speed;
        if (TASK_TIMER(work) > 60) {
            work->flags |= CHAO_MAY_SETTLE;
        }
        work->flags |= CHAO_MAY_LEAVE;
        break;
    }
}

/* push along the heading, with a motion by the Chao's power skill and its
 * kind. its speed is at most race_chao_take_skill_value's for power, from 0.1
 * to 0.25. SE_STEP plays each time the Chao has gone 4 more, and now and then
 * it strains, with a cry and ICON_HURT. after 300 steps it may tumble, the
 * likelier the nearer it is to first place */
static void race_chao_push(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    Angle ang;
    Float sine;
    Float cosine;
    Float speed;
    Float top;
    int chance;

    switch (work->submode) {
    case 0:
        if (chao->record->skills.power > 5000) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 160;
            racer->run_motion = 160;
        } else if (chao->record->kind > 0) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 128;
            racer->run_motion = 128;
        } else {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 159;
            racer->run_motion = 159;
        }
        chao->terrain.query_height = 7.0F;
        ++work->submode;
        work->timer = 0;
        work->tick = 0;
        racer->sound_distance = 0.0F;
        /* fall through */
    case 1:
        if (racer->unk_64_gain < 3) {
            racer->unk_64_gain = 3;
        }
        if (racer->sleep_gain < 1) {
            racer->sleep_gain = 1;
        }
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x +=
            (cosine * 0.023F) - (chao->speed.x * njAbs(sine) * 0.023F);
        chao->accel.z +=
            (-sine * 0.023F) - (chao->speed.z * njAbs(cosine) * 0.023F);
        chance = (int)RANDOM(0.0F, 1000.0F);
        if (TASK_TIMER(work) > 300 && chance < 15 - (racer_place(racer) * 2)) {
            SET_MODE(work, MODE_KOKE);
            /* @hack spelling: a return where a break would do, which loads
             * njScalor's address into R3 for the call at 0x0cb82104; as a
             * break, R2, and 2 of 404 words differ */
            return;
        }
        speed = njScalor(&chao->speed);
        top = race_chao_take_skill_value(racer, chao->record->skills.power,
                                         0.1F, 0.25F);
        if (speed > top) {
            speed = top;
            njUnitVector(&chao->speed);
            chao->speed.x *= speed;
            chao->speed.y *= speed;
            chao->speed.z *= speed;
        }
        chao->motion.rate = speed * 6.8F;
        racer->sound_distance += speed;
        if (racer->sound_distance > 4.0F) {
            racer->sound_distance = 0.0F;
            race_chao_sound(work, SE_STEP);
        }
        alife_face_set_eyes(chao, 1, 2);
        work->flags |= CHAO_MAY_SETTLE;
        work->flags |= CHAO_MAY_LEAVE;
        if (work->tick > 90.0F) {
            if (chao->record->skills.power > 5000) {
                racer->voice = 7;
            } else {
                racer->voice = 5;
            }
            work->tick = (int)RANDOM(0.0F, 10.0F);
            chao->icon.kind = ICON_HURT;
            chao->icon.symbol.state = 0;
            chao->icon.ball.state = 0;
            chao->icon.timer = 10;
        }
        break;
    }
}

/* run as a child, with motion 47 and its sound, leaning with the ground.
 * its speed is at most race_chao_take_skill_value's for run, from 0.1 to 0.38.
 * the Chao weaves, and a rise holds it back, as in race_chao_run_grown. after
 * 60 steps, a sleep over 199 is cleared, and the Chao pauses in mode 5 */
static void race_chao_run_child(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    Angle ang;
    Float sine;
    Float cosine;
    Float drag;
    Float speed;
    Float top;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 47;
        racer->run_motion = 47;
        chao->terrain.query_height = 7.0F;
        ++work->submode;
        work->timer = 0;
        work->tick = 0;
        /* fall through */
    case 1:
        work->flags |= CHAO_TILT;
        if (racer_is_followed(racer)) {
            sound_play_for(0x26d, (int)work, 1, 0, 2);
        }
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x +=
            (cosine * 0.023F) - (chao->speed.x * njAbs(sine) * 0.023F);
        chao->accel.z +=
            (-sine * 0.023F) - (chao->speed.z * njAbs(cosine) * 0.023F);
        if (TASK_TIMER(work) > 90.0F) {
            chao->accel.x += njSin(racer->phase) * sine * 0.01F;
            chao->accel.z += njSin(racer->phase) * cosine * 0.01F;
        }
        drag = racer->rise * 10.0F;
        if (drag > 0.5F) {
            drag = 0.5F;
        }
        drag -= chao->record->skills.power * 0.000025F;
        if (drag > 0.0F) {
            drag = 1.0F - drag;
            chao->accel.x *= drag;
            chao->accel.z *= drag;
        }
        speed = njScalor(&chao->speed);
        top = race_chao_take_skill_value(racer, chao->record->skills.run, 0.1F,
                                         0.38F);
        if (speed > top) {
            speed = top;
            njUnitVector(&chao->speed);
            chao->speed.x *= speed;
            chao->speed.y *= speed;
            chao->speed.z *= speed;
        }
        /* @hack spelling: the rate through top, which keeps the sine in FR15
         * and the cosine in FR14 (0x0cb822f6, 0x0cb82316); as
         * speed * 15.0F, the two trade and 87 of 414 words differ */
        top = speed * 15.0F;
        chao->motion.rate = top;
        if (racer->sleep_gain < 1) {
            racer->sleep_gain = 1;
        }
        if (TASK_TIMER(work) > 60) {
            if (chao->record->sleep > 199) {
                chao->record->sleep = 0;
                SET_MODE(work, MODE_PAUSE_SLEEP);
                break;
            }
            /* @hack redundant: the timer tested again inside the test that just
             * passed (0x0cb824a4, 0x0cb824d0); with one test, 66 of 290
             * words differ */
            if (TASK_TIMER(work) > 60) {
                work->flags |= CHAO_MAY_SETTLE;
            }
        }
        work->flags |= CHAO_MAY_LEAVE;
        if (work->tick > 60.0F * 2.1F) {
            racer->voice = 2;
            work->tick = (int)RANDOM(0.0F, 10.0F);
        }
        break;
    }
}

/* run a child as one, and any other Chao as grown. a RACER_LATE_START racer
 * runs as grown, child or not */
static void race_chao_run(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;

    if ((chao->record->type & 0x7f) == CHAO_CHILD &&
        !(racer->flags & RACER_LATE_START)) {
        race_chao_run_child(work, chao);
    } else {
        race_chao_run_grown(work, chao);
    }
}

/* walk along the heading. on a rise, the push along the heading loses up to
 * half of itself, and less the higher the power skill. SE_STEP plays each
 * time the Chao has gone 4 more. it may settle once the timer, which starts
 * at random from 0 to 59, passes 120 */
static void race_chao_walk(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    Angle ang;
    Float sine;
    Float cosine;
    Float drag;
    Float speed;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 0;
        chao->terrain.query_height = 7.0F;
        ++work->submode;
        work->timer = (int)RANDOM(0.0F, 60.0F);
        work->tick = 0;
        racer->sound_distance = 0.0F;
        /* fall through */
    case 1:
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x += (cosine * 0.023F) - (chao->speed.x * 0.07F);
        chao->accel.z += (-sine * 0.023F) - (chao->speed.z * 0.07F);
        drag = racer->rise * 10.0F;
        if (drag > 0.5F) {
            drag = 0.5F;
        }
        drag -= chao->record->skills.power * 0.000025F;
        if (drag > 0.0F) {
            drag = 1.0F - drag;
            chao->accel.x *= drag;
            chao->accel.z *= drag;
        }
        speed = njScalor(&chao->speed);
        chao->motion.rate = speed * 0.5F;
        racer->sound_distance += speed;
        if (racer->sound_distance > 4.0F) {
            racer->sound_distance = 0.0F;
            race_chao_sound(work, SE_STEP);
        }
        if (TASK_TIMER(work) > 120) {
            work->flags |= CHAO_MAY_SETTLE;
        }
        if (work->tick > 60.0F * 1.2F) {
            racer->voice = 1;
            work->tick = (int)RANDOM(0.0F, 10.0F);
        }
        break;
    }
    work->flags |= CHAO_MAY_LEAVE;
}

/* walk on along the heading for 60 steps, then play SE_REST, slow to a stop
 * and take motion 4 and eyes 4. it may settle once it starts to slow */
static void race_chao_rest(struct task_work *work, struct alife_chao *chao)
{
    Angle ang;
    Float sine;
    Float cosine;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 0;
        chao->terrain.query_height = 7.0F;
        ++work->submode;
        /* fall through */
    case 1:
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x += (cosine * 0.023F) - (chao->speed.x * 0.07F);
        chao->accel.z += (-sine * 0.023F) - (chao->speed.z * 0.07F);
        chao->motion.rate = njScalor(&chao->speed) * 0.5F;
        if (TASK_TIMER(work) > 60) {
            ++work->submode;
            race_chao_sound(work, SE_REST);
        }
        break;
    case 2:
        if (VECTOR_SQUARED(&chao->speed) > 0.1F * 0.1F) {
            chao->accel.x += -chao->speed.x * 0.125F;
            chao->accel.z += -chao->speed.z * 0.125F;
            chao->motion.rate = njScalor(&chao->speed) * 0.5F;
        } else {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 4;
            chao->motion.rate = 1.0F;
            chao->accel.x += -chao->speed.x;
            chao->accel.z += -chao->speed.z;
            ++work->submode;
        }
        break;
    case 3:
        alife_face_set_eyes(chao, 4, 2);
        break;
    }
    if (work->submode > 1) {
        work->flags |= CHAO_MAY_SETTLE;
    }
    work->flags |= CHAO_MAY_LEAVE;
}

/* stop with motion 20 and a cry. after 60 steps, play with the mode's item,
 * with its sounds while the race follows the lane:
 *
 *   MODE_PLAY_RAPPA   the bugle, motion 41
 *   MODE_PLAY_JYOURO  the watering can, motion 116
 *   MODE_PLAY_SCOOP   the scoop, motion 76
 *
 * after 600 steps of play, or at once on a cheer, COMMAND_RACE or
 * COMMAND_RETURN, run */
static void race_chao_play(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;

    chao->accel.x += -(chao->speed.x * 0.1F);
    chao->accel.z += -(chao->speed.z * 0.1F);
    racer->flags |= RACER_HELD;
    if ((racer->flags & RACER_CHEERED) ||
        racer_take_command(racer, COMMAND_RACE) ||
        racer_take_command(racer, COMMAND_RETURN)) {
        work->submode = 5;
    }
    racer_hold_path(racer);
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 20;
        chao->terrain.query_height = 7.0F;
        chao->motion.rate = 1.0F;
        work->submode = 1;
        work->tick = 0;
        work->timer = 0;
        racer->voice = 9;
        race_chao_sound(work, 616);
        /* fall through */
    case 1:
        chao->icon.kind = ICON_SYMBOL_5;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 5;
        if (TASK_TIMER(work) > 60) {
            switch (work->mode) {
            case MODE_PLAY_RAPPA:
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 41;
                race_chao_sound(work, 622);
                work->submode = 2;
                break;
            case MODE_PLAY_JYOURO:
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 116;
                work->submode = 3;
                break;
            /* @hack redundant: a case for MODE_PLAY_SCOOP beside the default,
             * whose test the switch makes (0x0cb82b1e); without it, 98 of 447
             * words differ */
            case MODE_PLAY_SCOOP:
            default:
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 76;
                work->submode = 4;
                break;
            }
            work->timer = 0;
        }
        break;
    case 2:
        alife_face_set_eyes(chao, 2, 2);
        if (racer_is_followed(racer)) {
            if ((TASK_TIMER(work) & 31) == 1) {
                sound_play(622, 0, 0, 0);
            }
            if ((TASK_TIMER(work) & 63) == 1) {
                chao->icon.kind = ICON_SYMBOL_5;
                chao->icon.symbol.state = 0;
                chao->icon.ball.state = 0;
                chao->icon.timer = 10;
            }
        }
        if (TASK_TIMER(work) > 600) {
            racer->voice = 2;
            work->submode = 5;
        }
        break;
    case 3:
        alife_face_set_eyes(chao, 2, 2);
        alife_face_set_mouth(chao, 3, 2);
        if (racer_is_followed(racer)) {
            sound_play_for(619, (int)racer, 1, 0, 2);
        }
        if (racer_is_followed(racer)) {
            if ((TASK_TIMER(work) & 63) == 1) {
                sound_play(619, 0, 0, 0);
                chao->icon.kind = ICON_SYMBOL_5;
                chao->icon.symbol.state = 0;
                chao->icon.ball.state = 0;
                chao->icon.timer = 10;
            }
        }
        if ((TASK_TIMER(work) & 127) == 1) {
            racer->voice = 2;
        }
        if (TASK_TIMER(work) > 600) {
            racer->voice = 2;
            work->submode = 5;
        }
        break;
    case 4:
        alife_face_set_eyes(chao, 2, 2);
        alife_face_set_mouth(chao, 1, 2);
        chao->icon.kind = ICON_SYMBOL_5;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 2;
        if (racer_is_followed(racer)) {
            if ((TASK_TIMER(work) & 31) == 1) {
                sound_play(633, 0, 0, 0);
            }
        }
        if ((TASK_TIMER(work) & 127) == 1) {
            racer->voice = 17;
        }
        if (TASK_TIMER(work) > 600) {
            work->submode = 5;
        }
        break;
    case 5:
        SET_MODE(work, MODE_RUN);
        break;
    }
}

/* tumble, braking the harder the more the Chao rises. after 60 steps it may
 * get up. going steeply down, it may stay down instead with motion 62 for
 * 92 to 121 steps more, the likelier the lower its agres. a cheer lets it
 * get up at once */
static void race_chao_koke(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    Float drag = -(racer->rise * 10.0F);
    int chance;

    if (drag > 1.0F) {
        drag = 1.0F;
    } else if (drag < -1.0F) {
        drag = -1.0F;
    }
    drag = 1.0F - drag;
    chao->accel.x += -(chao->speed.x * drag * 0.03F);
    chao->accel.z += -(chao->speed.z * drag * 0.03F);
    racer->flags |= RACER_HELD;
    if (racer->flags & RACER_CHEERED) {
        work->flags |= CHAO_MAY_SETTLE;
        work->flags |= CHAO_MAY_LEAVE;
    }
    switch (work->submode) {
    case 0:
        if ((chao->record->type & 0x7f) != CHAO_CHILD) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 1;
        } else {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 46;
        }
        chao->terrain.query_height = 7.0F;
        chao->motion.rate = njScalor(&chao->speed);
        ++work->submode;
        race_chao_sound(work, 613);
        chao->icon.kind = ICON_GLAD;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 60;
        /* fall through */
    case 1:
        if (racer->unk_65_gain < 3) {
            racer->unk_65_gain = 3;
        }
        if (TASK_TIMER(work) == 30) {
            racer->voice = 15;
        }
        if (TASK_TIMER(work) == 60) {
            chance = (int)RANDOM(0.0F, 150.0F);
            if (drag < 0.15F && chao->record->agres + chance < 100) {
                ++work->submode;
                work->timer = (int)RANDOM(0.0F, 30.0F);
                chao->icon.kind = ICON_HURT;
                chao->icon.symbol.state = 0;
                chao->icon.ball.state = 0;
                chao->icon.timer = 60;
            }
        } else if (TASK_TIMER(work) > 60) {
            work->flags |= CHAO_MAY_SETTLE;
            /* the push up cancels CHAO_GRAVITY and lifts the Chao 0.015
             * more. SHC folds the sum to 0x3d851eb7, one below 0.065F's
             * 0x3d851eb8 */
            chao->accel.y += CHAO_GRAVITY + 0.015F;
        }
        break;
    case 2:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 62;
        ++work->submode;
        /* fall through */
    case 3:
        if (racer->unk_65_gain > -3) {
            racer->unk_65_gain = -3;
        }
        if ((TASK_TIMER(work) & 63) == 1) {
            racer->voice = 13;
        }
        if (TASK_TIMER(work) > 120) {
            work->flags |= CHAO_MAY_SETTLE;
            chao->accel.y += CHAO_GRAVITY + 0.015F;
        }
        break;
    }
    work->flags |= CHAO_MAY_LEAVE;
}

/* pause: slow down for 60 steps at most, then stop and take motion, which
 * is 84, 78 or 63. any other is taken as 84. the pause then runs by its
 * motion, with its sounds while the race follows the lane:
 *
 *   84  the eyes turned by alife_face_look to 0 about x and z, for 180 of
 *       its frames, and voice 17 once. it may settle after 180 steps
 *   78  the eyes turned to 90 degrees about x for 300 of alife_face_look's
 *       frames, then eyes 4, a lean with the ground and SE_PAUSE_SLEEP every
 *       61 steps. it may settle after 300 steps
 *   63  SE_PAUSE_AGRES once. it may settle after 120 steps
 *
 * a cheer lets it settle at once */
static void race_chao_pause(struct task_work *work, struct alife_chao *chao,
                            int motion)
{
    struct racer *racer = RACER_VARS(work)->racer;
    int seconds;
    Float speed;

    racer->flags |= RACER_HELD;
    if (racer->flags & RACER_CHEERED) {
        work->flags |= CHAO_MAY_SETTLE;
        work->flags |= CHAO_MAY_LEAVE;
    }
    switch (work->submode) {
    case 0:
        chao->terrain.query_height = 7.0F;
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 0;
        ++work->submode;
        work->timer = 0;
        /* fall through */
    case 1:
        speed = njScalor(&chao->speed);
        chao->motion.rate = speed * 0.5F;
        if (VECTOR_SQUARED(&chao->speed) >= 0.15F * 0.15F &&
            TASK_TIMER(work) < 60) {
            chao->accel.x += -(chao->speed.x * 0.1F);
            chao->accel.z += -(chao->speed.z * 0.1F);
        } else {
            switch (motion) {
            case 84:
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = motion;
                racer->run_motion = motion;
                alife_face_look(chao, 0, 0, 180);
                break;
            case 78:
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = motion;
                racer->run_motion = motion;
                alife_face_look(chao, NJM_DEG_ANG(90), 0, 300);
                break;
            case 63:
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = motion;
                racer->run_motion = motion;
                break;
            default:
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 84;
                racer->run_motion = 84;
                alife_face_look(chao, 0, 0, 180);
                break;
            }
            ++work->submode;
            work->timer = 0;
            work->tick = 0;
        }
        break;
    case 2:
        chao->accel.x = -chao->speed.x;
        chao->accel.z = -chao->speed.z;
        chao->motion.rate = 1.0F;
        switch (racer->run_motion) {
        case 84:
            seconds = 3;
            break;
        case 78:
            work->flags |= CHAO_TILT;
            alife_face_set_eyes(chao, 4, 2);
            seconds = 5;
            break;
        case 63:
            seconds = 2;
            break;
        }
        /* @bug uninitialized: a run_motion other than 84, 78 or 63 leaves
         * seconds unset, and MUL.L at 0x0cb8325c reads the R4 that still
         * holds work (from 0x0cb83204). case 1 leaves run_motion 84, 78 or
         * 63, so the test never reads seconds unset */
        if (TASK_TIMER(work) > seconds * 60) {
            work->flags |= CHAO_MAY_SETTLE;
        }
        if (racer_is_followed(racer)) {
            switch (racer->run_motion) {
            case 84:
                if (work->tick == 1) {
                    racer->voice = 17;
                } else if (work->tick > 1) {
                    work->tick = 2;
                }
                break;
            case 78:
                if (work->tick > 60) {
                    sound_play(SE_PAUSE_SLEEP, 0, 0, 0);
                    work->tick = 0;
                }
                break;
            case 63:
                if (work->tick == 20) {
                    sound_play(SE_PAUSE_AGRES, 0, 0, 0);
                } else if (work->tick > 20) {
                    work->tick = 21;
                }
                break;
            }
        }
        break;
    }
    work->flags |= CHAO_MAY_LEAVE;
}

/* struggle with motion 36 and a cry, held near 2.5 under the water's
 * surface. the Chao throws its wake each step, and on about one step in 13
 * a ripple, with a sound. it may settle or leave after 72 steps, or at once
 * on a cheer */
static void race_chao_drown(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    Float level;
    Float depth;

    racer->flags |= RACER_HELD;
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 36;
        chao->terrain.query_height = 7.0F;
        work->timer = 0;
        ++work->submode;
        racer->voice = 14;
        chao->icon.kind = ICON_GLAD;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 60;
        /* fall through */
    case 1:
        chao->accel.x += -(chao->speed.x * 0.1F);
        chao->accel.z += -(chao->speed.z * 0.1F);
        if (racer->unk_65_gain < 4) {
            racer->unk_65_gain = 4;
        }
        level = race_chao_water_level(work, chao);
        depth = level - work->pos.y - 2.5F;
        if (depth > -3.0F) {
            if (depth > 0.0F) {
                chao->accel.y += CHAO_GRAVITY + (depth * 0.003F);
            } else {
                /* SHC folds the difference to 0x3d408311, one below 0.047F's
                 * 0x3d408312 */
                chao->accel.y += CHAO_GRAVITY - 0.003F;
            }
        }
        race_chao_wake(work, chao);
        if ((TASK_TIMER(work) & 1) && rand() * RAND_SCALE > 0.85F) {
            race_chao_ripple(work, chao, level);
            race_chao_sound(work, 624);
        }
        chao->motion.rate = 0.8F;
        break;
    /* @hack redundant: a default that does nothing, which loads
     * alife_face_set_eyes' address into R3 and alife_face_set_mouth's into
     * R2 (0x0cb834d0, 0x0cb834da); without it, the two trade and 4 of 277
     * words differ */
    default:
        break;
    }
    alife_face_set_eyes(chao, 1, 2);
    alife_face_set_mouth(chao, 5, 2);
    if (TASK_TIMER(work) > 1.2F * 60.0F || (racer->flags & RACER_CHEERED)) {
        work->flags |= CHAO_MAY_SETTLE;
        work->flags |= CHAO_MAY_LEAVE;
    }
}

/* walk on along the heading for seconds times 60 steps, then slow to a stop
 * and take motion. at 0 seconds, take it at once and stop. the Chao may
 * settle from the slowing on, and after 1200 steps it runs */
static void race_chao_eat(struct task_work *work, struct alife_chao *chao,
                          int motion, Float seconds)
{
    Angle ang;
    Float sine;
    Float cosine;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 0;
        chao->terrain.query_height = 7.0F;
        /* @hack spelling: submode stepped as submode + 1, which spills
         * motion to sp+4 and seconds to sp+0 (0x0cb83574, 0x0cb83576); as
         * ++, the two trade and 176 of 285 words differ */
        work->submode = work->submode + 1;
        /* fall through */
    case 1:
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x += (cosine * 0.023F) - (chao->speed.x * 0.07F);
        chao->accel.z += (-sine * 0.023F) - (chao->speed.z * 0.07F);
        if (seconds == 0.0F) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = motion;
            chao->motion.rate = 1.0F;
            work->submode = 3;
        } else if (TASK_TIMER(work) > seconds * 60.0F) {
            work->submode = 2;
            chao->motion.rate = njScalor(&chao->speed) * 0.5F;
        }
        break;
    case 2:
        if (0.1F * 0.1F < VECTOR_SQUARED(&chao->speed)) {
            chao->accel.x += -(chao->speed.x * 0.125F);
            chao->accel.z += -(chao->speed.z * 0.125F);
            chao->motion.rate = njScalor(&chao->speed) * 0.5F;
        } else {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = motion;
            chao->motion.rate = 1.0F;
            work->submode = 3;
        }
        break;
    case 3:
        chao->accel.x += -chao->speed.x;
        chao->accel.z += -chao->speed.z;
        break;
    }
    if (TASK_TIMER(work) > 1200) {
        SET_MODE(work, MODE_RUN);
        return;
    }
    if (work->submode >= 2) {
        work->flags |= CHAO_MAY_SETTLE;
    }
    work->flags |= CHAO_MAY_LEAVE;
}

/* stand still and take motion 52 twice: first with a cry, and 40 steps
 * later with SE_SWING. it may settle 50 steps after that */
static void race_chao_swing(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 52;
        chao->terrain.query_height = 7.0F;
        ++work->submode;
        work->timer = 0;
        chao->motion.rate = 1.0F;
        racer->voice = 8;
        chao->icon.kind = ICON_GLAD_3;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 40;
        alife_face_set_mouth(chao, 4, 2);
        /* fall through */
    case 1:
        if (TASK_TIMER(work) > 40) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 52;
            ++work->submode;
            work->timer = 0;
            race_chao_sound(work, SE_SWING);
        }
        break;
    case 2:
        if (TASK_TIMER(work) > 50) {
            work->flags |= CHAO_MAY_SETTLE;
        }
        break;
    }
    chao->accel.x += -chao->speed.x;
    chao->accel.z += -chao->speed.z;
    if (racer->unk_64_gain < 2) {
        racer->unk_64_gain = 2;
    }
    work->flags |= CHAO_MAY_LEAVE;
}

/* swim along the heading, held near 2.5 under the water's surface. its
 * speed is at most race_chao_take_skill_value's for swim, from 0.13 to 0.33.
 * once the Chao stands on ground less than 3.5 under the surface, it runs.
 * under 5000 swim skill it may drown, the likelier the lower the skill */
static void race_chao_swim(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    int chance;
    int unused; /* NOLINT(clang-diagnostic-unused-but-set-variable) */
    Angle ang;
    Float sine;
    Float cosine;
    Float level;
    Float depth;
    Float speed;
    Float top;

    switch (work->submode) {
    case 0:
        if ((chao->record->type & 0x7f) == CHAO_SWIM) {
            chance = (int)RANDOM(0.0F, 4.0F);
            /* @hack redundant: two tests of chance, stored and never read,
             * which keep 0.023 in FR5 and 0.012 in FR6, their pool words at
             * 0x0cb83ae0 and 0x0cb83adc (0x0cb83a46, 0x0cb83a4c); without
             * them, the two trade and 12 of 484 words differ */
            unused = chance ? work->mode : 0;
            unused = chance ? work->submode : 0;
            if (chance == 0) {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 100;
            } else {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 15;
            }
        } else if (chao->record->skills.swim > 2000) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 15;
        } else {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 16;
        }
        chao->terrain.query_height = 7.0F;
        work->tick = 0;
        work->timer = 0;
        ++work->submode;
        racer->sound_distance = 0.0F;
        /* fall through */
    case 1:
        if (racer->sleep_gain < 2) {
            racer->sleep_gain = 2;
        }
        if (chao->record->skills.swim < 2000) {
            if (racer->unk_65_gain < 2) {
                racer->unk_65_gain = 2;
            }
        }
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x +=
            (cosine * 0.023F) - (chao->speed.x * njAbs(sine) * 0.012F);
        chao->accel.z +=
            (-sine * 0.023F) - (chao->speed.z * njAbs(cosine) * 0.012F);
        level = race_chao_water_level(work, chao);
        depth = level - work->pos.y - 2.5F;
        if (depth > -3.0F) {
            if (depth > 0.0F) {
                chao->accel.y += CHAO_GRAVITY + (depth * 0.003F);
            } else {
                /* SHC folds the difference to 0x3d408311, one below 0.047F's
                 * 0x3d408312 */
                chao->accel.y += CHAO_GRAVITY - 0.003F;
            }
        }
        race_chao_wake(work, chao);
        if ((TASK_TIMER(work) & 1) && rand() * RAND_SCALE > 0.85F) {
            race_chao_ripple(work, chao, level);
        }
        /* @hack redundant: an else after the break, whose jump SHC deletes,
         * which loads the record into R3 ahead of 5000 (0x0cb83b56 to
         * 0x0cb83b5a); without it, 5000 takes R3 first, and 3 of 484 words
         * differ */
        if (depth < 1.0F && (work->flags & CHAO_ON_GROUND)) {
            SET_MODE(work, MODE_RUN);
            break;
        } else if (chao->record->skills.swim < 5000) {
            chance = (int)RANDOM(0.0F, 3000.0F);
            if (chance < 10.0F - (chao->record->skills.swim * 0.001F)) {
                SET_MODE(work, MODE_DROWN);
                break;
            }
        }
        if (TASK_TIMER(work) > 60) {
            work->flags |= CHAO_MAY_SETTLE;
            work->flags |= CHAO_MAY_LEAVE;
        }
        speed = njScalor(&chao->speed);
        chao->motion.rate = speed;
        /* nothing in this mode clears sound_distance, so once it passes 5.5
         * the sound plays every step */
        racer->sound_distance += speed;
        if (racer->sound_distance > 5.5F) {
            race_chao_sound(work, 0x259);
        }
        speed = njScalor(&chao->speed);
        top = race_chao_take_skill_value(racer, chao->record->skills.swim,
                                         0.13F, 0.33F);
        if (speed > top) {
            njUnitVector(&chao->speed);
            chao->speed.x *= top;
            chao->speed.y *= top;
            chao->speed.z *= top;
        }
        break;
    }
}

void race_chao_stop(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 11;
        chao->terrain.query_height = 7.0F;
        chao->motion.rate = 1.0F;
        ++work->submode;
        work->tick = 0;
        work->timer = 0;
        /* fall through */
    case 1:
        chao->accel.x += -(chao->speed.x * 0.1F);
        chao->accel.z += -(chao->speed.z * 0.1F);
        if (chao->speed.x < 0.01F) {
            chao->speed.x = 0.0F;
        }
        if (chao->speed.z < 0.01F) {
            chao->speed.z = 0.0F;
        }
        alife_face_set_eyes(chao, 2, 2);
        alife_face_set_mouth(chao, 1, 2);
        /* @hack spelling: 2 through a halfword local, which loads it into R3
         * after the state stores, whose offsets come by ADD (0x0cb83d86 to
         * 0x0cb83d90); stored directly, 2 loads first, one offset comes from
         * a pool word at 0x0cb83da6, and 7 of 140 words differ */
        {
            Sint16 timer = 2;

            chao->icon.kind = ICON_SYMBOL_4;
            chao->icon.symbol.state = 0;
            chao->icon.ball.state = 0;
            chao->icon.timer = timer;
        }
        break;
    }
}

/* stand as race_chao_stop does, with a cry every 76 to 85 steps. with
 * RACER_LATE_START, the Chao takes race_chao_ready's motion after 120 steps,
 * and holds still. it may settle at any step */
static void race_chao_goal(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;

    race_chao_stop(work, chao);
    if (work->tick > 1.4F * 60.0F) {
        racer->voice = 2;
        work->tick = (int)RANDOM(0.0F, 10.0F);
    }
    if ((racer->flags & RACER_LATE_START) && TASK_TIMER(work) > 120 &&
        work->submode < 5) {
        work->submode = 5;
    }
    switch (work->submode) {
    case 5:
        race_chao_ready(work, chao);
        work->submode = 6;
        /* fall through */
    case 6:
        chao->accel.x += -(chao->speed.x * 0.1F);
        chao->accel.z += -(chao->speed.z * 0.1F);
        if (chao->speed.x < 0.01F) {
            chao->speed.x = 0.0F;
        }
        if (chao->speed.z < 0.01F) {
            chao->speed.z = 0.0F;
        }
        break;
    }
    work->flags |= CHAO_MAY_SETTLE;
}

void fun_0cb83ec0(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: a return to end each case, where nothing follows the
     * switch, which ends case 0 with an RTS of its own (0x0cb83ef8); as breaks,
     * 10 of 36 words and 2 of 28 other words differ */
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 8;
        chao->terrain.query_height = 7.0F;
        chao->motion.rate = 0.5F;
        ++work->submode;
        return;
    case 1:
        alife_face_set_eyes(chao, 4, 2);
        return;
    /* @hack redundant: a default that only returns, which ends the default with
     * an RTS of its own (0x0cb83ece); without it, 2 of 36 words differ */
    default:
        return;
    }
}

/* take one of race_chao_idles at random, at half speed, and another 93 to
 * 122 steps later */
static void race_chao_idle(struct task_work *work, struct alife_chao *chao)
{
    Uint8 motion;

    switch (work->submode) {
    case 0:
        motion = (int)RANDOM(0.0F, 5.0F);
        motion = race_chao_idles[motion];
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = motion;
        chao->terrain.query_height = 7.0F;
        chao->motion.rate = 0.5F;
        alife_face_set_mouth(chao, 3, 2);
        ++work->submode;
        work->timer = (int)RANDOM(0.0F, 30.0F);
        break;
    case 1:
        if (TASK_TIMER(work) > 120.0F) {
            work->submode = 0;
        }
        break;
    }
}

/* take motion 155, the pose of a Chao ready to race. then move to the next
 * submode, with the timer at random from 0 to 29 */
static void race_chao_ready(struct task_work *work, struct alife_chao *chao)
{
    chao->motion.flags |= MOTION_NEW;
    chao->motion.id = 155;
    chao->terrain.query_height = 7.0F;
    chao->motion.rate = 0.8F;
    alife_face_set_mouth(chao, 3, 2);
    ++work->submode;
    work->timer = (int)RANDOM(0.0F, 30.0F);
}

/* idle at the start until COMMAND_RACE, then leave within 40 steps. while
 * the Chao idles, water above sends it to MODE_FLOAT. with RACER_LATE_START,
 * it waits in race_chao_ready's motion, and leaves once the racer in first
 * place has run race_chao_late_start's distance for the race's kind. it
 * leaves at once while that racer's distance is 0 or below */
static void race_chao_wait(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    int kind;
    int distance;

    switch (work->submode) {
    case 0:
    case 1:
        if (!(racer->flags & RACER_LATE_START)) {
            race_chao_idle(work, chao);
        } else if (!work->submode) {
            race_chao_ready(work, chao);
        }
        chao->accel.x += -(chao->speed.x * 0.05F);
        chao->accel.z += -(chao->speed.z * 0.05F);
        if (chao->terrain.flags & TERRAIN_WATER_ABOVE) {
            SET_MODE(work, MODE_FLOAT);
            return;
        }
        if (racer_take_command(racer, COMMAND_RACE)) {
            work->submode = 2;
            work->timer = (int)RANDOM(0.0F, 40.0F);
            /* @hack redundant: a break that repeats the case's own, whose jump
             * SHC deletes, which gives case 2's flags R3 (0x0cb84156 to
             * 0x0cb8415a); without it, R2, and 3 of 177 words differ */
            break;
        }
        break;
    case 2:
        if (!(racer->flags & RACER_LATE_START)) {
            if (TASK_TIMER(work) < 40) {
                break;
            }
        } else {
            distance = fun_0cb8b860(0, 1);
            if (distance > 0 && race_get_kind(&kind) &&
                distance < race_chao_late_start[kind]) {
                break;
            }
        }
        work->flags |= CHAO_MAY_LEAVE;
        break;
    }
    work->flags |= CHAO_MAY_SETTLE;
}

/* float at the start. below 0.5 over the water's surface the Chao is
 * lifted, and above it pushed down. with no water above or below, it
 * waits */
static void race_chao_float(struct task_work *work, struct alife_chao *chao)
{
    Float level;
    Float depth;

    /* @hack spelling: a return to end case 1, where nothing follows the switch,
     * which ends it with an RTS of its own (0x0cb84314); as a break, 1 of 118
     * words differ */
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 4;
        chao->terrain.query_height = 7.0F;
        chao->motion.rate = 0.5F;
        ++work->submode;
        /* fall through */
    case 1:
        chao->accel.x += -(chao->speed.x * 0.05F);
        chao->accel.y += -(chao->speed.y * 0.01F);
        chao->accel.z += -(chao->speed.z * 0.05F);
        if (chao->terrain.flags & TERRAIN_WATER_ABOVE) {
            level = chao->terrain.query.sides[SIDE_ABOVE].level;
        } else if (chao->terrain.flags & TERRAIN_WATER_BELOW) {
            level = chao->terrain.query.sides[SIDE_BELOW].level;
        } else {
            SET_MODE(work, MODE_WAIT);
            return;
        }
        depth = level - work->pos.y - 2.5F;
        if (depth > -3.0F) {
            if (depth > 0.0F) {
                chao->accel.y += CHAO_GRAVITY + (depth * 0.001F);
            } else {
                chao->accel.y += CHAO_GRAVITY - (depth * 0.001F);
            }
        } else {
            chao->accel.y += -CHAO_GRAVITY;
        }
        return;
    /* @hack redundant: a default that only returns, which ends it with an RTS
     * of its own (0x0cb841ee); without it, 1 of 118 words differ */
    default:
        return;
    }
}

/* leap out along the heading, then glide. its speed is at most
 * race_chao_take_skill_value's for fly, from 0.6 to 1. the fly skill holds the
 * glide up. after 300 steps the Chao runs */
static void race_chao_fly(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    Angle ang;
    Float sine;
    Float cosine;
    Float speed;
    Float top;
    int chance;

    switch (work->submode) {
    case 0:
        if (chao->record->skills.fly < 2000) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 37;
        } else {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 14;
        }
        ++work->submode;
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->speed.y = 0.5F;
        /* @hack spelling: the cosine scaled in place, which keeps it in FR4
         * and 1.5 in FR5 (0x0cb843d0, 0x0cb843de); as cosine * 1.5F, the two
         * trade and 5 of 443 words differ */
        cosine *= 1.5F;
        chao->speed.x = cosine;
        chao->speed.z = -sine * 1.5F;
        chao->terrain.query_height = 7.0F;
        racer->voice = 6;
        race_chao_sound(work, SE_CHAO_SQUEAK);
        work->timer = 0;
        work->tick = 0;
        /* fall through */
    case 1:
        if (chao->record->skills.fly < 2000) {
            alife_face_set_eyes(chao, 1, 2);
        } else {
            alife_face_set_eyes(chao, 2, 2);
        }
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x +=
            (cosine * 0.02F) - (chao->speed.x * njAbs(sine) * 0.012F);
        chao->accel.z +=
            (-sine * 0.02F) - (chao->speed.z * njAbs(cosine) * 0.012F);
        speed = njScalor(&chao->speed);
        top = race_chao_take_skill_value(racer, chao->record->skills.fly, 0.6F,
                                         1.0F);
        if (speed > top) {
            njUnitVector(&chao->speed);
            chao->speed.x *= top;
            chao->speed.y *= top;
            chao->speed.z *= top;
        }
        chao->motion.rate = njScalor(&chao->speed);
        chance = (int)RANDOM(0.0F, 1000.0F);
        if (chance < chao->record->skills.fly * 0.13F) {
            chao->accel.y = CHAO_GRAVITY * 0.9F;
        } else {
            chao->accel.y = race_chao_take_skill_value(
                racer, chao->record->skills.fly, 0.05F * 0.05F, 0.0375F);
        }
        if (TASK_TIMER(work) > 300) {
            SET_MODE(work, MODE_RUN);
            break;
        }
        if (work->tick > 60.0F * 1.6F) {
            racer->voice = 3;
            work->tick = (int)RANDOM(20.0F, 30.0F);
        }
        work->flags |= CHAO_MAY_SETTLE;
        work->flags |= CHAO_MAY_LEAVE;
        break;
    }
}

/* climb the wall ahead, straining now and then. its speed is at most
 * race_chao_take_skill_value's for power, from 0.15 to 0.25, or 0.25 while bit
 * 0 of the racer's overhang is set. once no wall stands ahead, the Chao moves
 * on over the top, and runs once it lands or 15 steps on */
static void race_chao_climb(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    Angle ang;
    Float sine;
    Float cosine;
    Float speed;
    Float top;
    Float climb_top;
    Float scale;

    chao->motion.rate = chao->speed.y * 6.3F;
    racer->flags |= RACER_CLIMBING;
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 55;
        chao->terrain.query_height = 7.0F;
        ++work->submode;
        work->timer = (int)RANDOM(0.0F, 15.0F);
        work->tick = 0;
        /* fall through */
    case 1:
        if (racer->unk_64_gain < 5) {
            racer->unk_64_gain = 5;
        }
        if (racer->sleep_gain < 3) {
            racer->sleep_gain = 3;
        }
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x +=
            (cosine * 0.01F) - (chao->speed.x * njAbs(sine) * 0.012F);
        chao->accel.z +=
            (-sine * 0.01F) - (chao->speed.z * njAbs(cosine) * 0.012F);
        chao->accel.y += CHAO_GRAVITY * 1.1F;
        speed = njScalor(&chao->speed);
        if (!(racer->overhang & 1)) {
            climb_top = race_chao_take_skill_value(
                racer, chao->record->skills.power, 0.15F, 0.25F);
        } else {
            climb_top = 0.25F;
        }
        if (speed > climb_top) {
            /* @hack spelling: the limit copied to scale, which loads it into
             * FR4 and moves it to FR15 across njUnitVector (0x0cb84842,
             * 0x0cb8484e); used directly, it loads into FR15, 274 of 480
             * words differ and the function runs 32 bytes short */
            scale = climb_top;
            njUnitVector(&chao->speed);
            chao->speed.x *= scale;
            chao->speed.y *= scale;
            chao->speed.z *= scale;
        }
        if (work->tick > 60.0F * 2.2F) {
            if (chao->record->skills.power > 5000) {
                racer->voice = 8;
            } else {
                racer->voice = 5;
            }
            work->tick = (int)RANDOM(0.0F, 10.0F);
            chao->icon.kind = ICON_HURT;
            chao->icon.symbol.state = 0;
            chao->icon.ball.state = 0;
            chao->icon.timer = 30;
        }
        if (!(work->flags & CHAO_WALL_AHEAD)) {
            ++work->submode;
            work->timer = 0;
            chao->terrain.query_height = 3.0F;
        }
        break;
    case 2:
        if (!(work->flags & CHAO_WALL_AHEAD)) {
            if (TASK_TIMER(work) > 14 || (work->flags & CHAO_ON_GROUND)) {
                SET_MODE(work, MODE_RUN);
                break;
            }
        } else {
            work->timer = 0;
        }
        ang = work->ang.y;
        sine = njSin(ang);
        cosine = njCos(ang);
        chao->accel.x +=
            (cosine * 0.002F) - (chao->speed.x * njAbs(sine) * 0.012F);
        chao->accel.z +=
            (-sine * 0.002F) - (chao->speed.z * njAbs(cosine) * 0.012F);
        chao->accel.y += CHAO_GRAVITY * 1.1F;
        speed = njScalor(&chao->speed);
        top = (chao->record->skills.power + 10000) * 0.00001F;
        if (speed > top) {
            njUnitVector(&chao->speed);
            chao->speed.x *= top;
            chao->speed.y *= top;
            chao->speed.z *= top;
        }
        break;
    }
}

void race_chao_dance_moves(struct task_work *work, struct alife_chao *chao)
{
    int motion;

    switch (work->submode) {
    case 0:
        motion = (int)RANDOM(32.0F, 36.0F);
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = motion;
        chao->terrain.query_height = 7.0F;
        ++work->submode;
        work->timer = 0;
        work->tick = (int)RANDOM(0.0F, 30.0F);
        chao->motion.rate = 4.0F;
        alife_face_set_mouth(chao, 3, 35);
        /* fall through */
    case 1:
        chao->accel.x += -chao->speed.x * 0.125F;
        chao->accel.z += -chao->speed.z * 0.125F;
        if (work->tick > 50) {
            work->submode = 0;
        }
        break;
    }
}

/* dance as race_chao_dance_moves does, with SE_DANCE or one of the two after it
 * at each new motion, until COMMAND_RACE sends the Chao running */
static void race_chao_dance(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;

    switch (work->submode) {
    case 0:
        race_chao_sound(work, (int)RANDOM(SE_DANCE, SE_DANCE + 3));
        break;
    }
    race_chao_dance_moves(work, chao);
    racer->flags |= RACER_HELD;
    if (racer_take_command(racer, COMMAND_RACE)) {
        SET_MODE(work, MODE_RUN);
        return;
    }
    work->flags |= CHAO_MAY_LEAVE;
}

/* pose with motion 82, as race_chao_pose does. from the second step on,
 * keep eyes 5, mouth 6 and icon kind ICON_HOLD_2 */
static void race_chao_finish_4(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    /* @hack redundant: a case 0 that does nothing, whose test the switch makes
     * first (0x0cb84bd6); without it, the switch tests 1 alone, 31 of 74
     * words differ and the function runs 4 bytes short */
    case 0:
        break;
    case 1:
        chao->icon.kind = ICON_HOLD_2;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 3;
        alife_face_set_mouth(chao, 6, 35);
        alife_face_set_eyes(chao, 5, 35);
        break;
    }
    race_chao_pose(work, chao, 82);
}

/* as race_chao_finish_4, with motion 83 */
static void race_chao_finish_6(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    /* @hack redundant: a case 0 that does nothing, whose test the switch makes
     * first (0x0cb84c6a); without it, the switch tests 1 alone, 30 of 37
     * words differ and race_chao_pose, which this runs into, starts 4 bytes
     * early */
    case 0:
        break;
    case 1:
        chao->icon.kind = ICON_HOLD_2;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 3;
        alife_face_set_mouth(chao, 6, 35);
        alife_face_set_eyes(chao, 5, 35);
        break;
    }
    race_chao_pose(work, chao, 83);
}

/* take motion at four times speed, slowing to a stop */
static void race_chao_pose(struct task_work *work, struct alife_chao *chao,
                           int motion)
{
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = motion;
        chao->terrain.query_height = 7.0F;
        ++work->submode;
        chao->motion.rate = 4.0F;
        /* fall through */
    case 1:
        chao->accel.x += -chao->speed.x * 0.125F;
        chao->accel.z += -chao->speed.z * 0.125F;
        break;
    }
}

/* clear CHAO_MAY_SETTLE, CHAO_MAY_LEAVE and CHAO_TILT, and turn toward the
 * target unless in MODE_WAIT. then run the work's mode, giving the racer the
 * mode's gait, and let race_chao_obey choose the mode that follows */
static void race_chao_move(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;

    work->flags &= ~CHAO_MAY_SETTLE;
    work->flags &= ~CHAO_MAY_LEAVE;
    chao->turn_speed = 0x200;
    if (work->mode) {
        alife_turn_to_target(work, chao);
    }
    work->flags &= ~CHAO_TILT;
    switch (work->mode) {
    case MODE_WAIT:
        racer->gait = GAIT_WAIT;
        race_chao_wait(work, chao);
        break;
    case MODE_FLOAT:
        racer->gait = GAIT_WAIT;
        race_chao_float(work, chao);
        break;
    case MODE_KOKE:
        work->flags |= CHAO_TILT;
        racer->gait = GAIT_FOOT;
        race_chao_koke(work, chao);
        break;
    case MODE_PAUSE_SLEEP:
        racer->gait = GAIT_FOOT;
        race_chao_pause(work, chao, 78);
        break;
    /* @hack spelling: MODE_PAUSE_MILD's case ahead of MODE_PAUSE_AGRES's, which
     * puts the body for motion 84 ahead of the one for 63, which runs into the
     * call the three share (0x0cb84e14, 0x0cb84e42); in order, 2 of 211 words
     * and 2 of 121 other words differ */
    case MODE_PAUSE_MILD:
        racer->gait = GAIT_FOOT;
        race_chao_pause(work, chao, 84);
        break;
    case MODE_PAUSE_AGRES:
        racer->gait = GAIT_FOOT;
        race_chao_pause(work, chao, 63);
        break;
    case MODE_RUN:
        racer->gait = GAIT_FOOT;
        race_chao_run(work, chao);
        break;
    case MODE_WALK:
        racer->gait = GAIT_FOOT;
        race_chao_walk(work, chao);
        break;
    case MODE_FLY:
        racer->gait = GAIT_FLY;
        race_chao_fly(work, chao);
        break;
    case MODE_SWIM:
        racer->gait = GAIT_SWIM;
        race_chao_swim(work, chao);
        break;
    case MODE_CLIMB:
        racer->gait = GAIT_CLIMB;
        race_chao_climb(work, chao);
        break;
    case MODE_GOAL:
        racer->gait = GAIT_STILL;
        race_chao_goal(work, chao);
        break;
    /* @hack spelling: a body for each, which SHC merges into one (0x0cb84ee0)
     * and which keeps GAIT_STILL in R4 and GAIT_FOOT in R5 (0x0cb84d72,
     * 0x0cb84d76); with three labels on one body, the two trade and 42 of 211
     * words differ */
    case MODE_PLAY_RAPPA:
        racer->gait = GAIT_STILL;
        race_chao_play(work, chao);
        break;
    case MODE_PLAY_JYOURO:
        racer->gait = GAIT_STILL;
        race_chao_play(work, chao);
        break;
    case MODE_PLAY_SCOOP:
        racer->gait = GAIT_STILL;
        race_chao_play(work, chao);
        break;
    case MODE_SWING:
        racer->gait = GAIT_STILL;
        race_chao_swing(work, chao);
        break;
    case MODE_PUSH:
        racer->gait = GAIT_FOOT;
        race_chao_push(work, chao);
        break;
    case MODE_EAT:
        racer->gait = GAIT_STILL;
        race_chao_eat(work, chao, 58, 0.0F);
        break;
    case MODE_REST:
        racer->gait = GAIT_FOOT;
        race_chao_rest(work, chao);
        break;
    case MODE_DROWN:
        racer->gait = GAIT_SWIM;
        race_chao_drown(work, chao);
        break;
    case MODE_DANCE:
        racer->gait = GAIT_STILL;
        race_chao_dance(work, chao);
        break;
    case MODE_FINISH_6:
        racer->gait = GAIT_STILL;
        race_chao_finish_6(work, chao);
        break;
    case MODE_FINISH_4:
        racer->gait = GAIT_STILL;
        race_chao_finish_4(work, chao);
        break;
    }
    race_chao_obey(work, chao);
    /* @hack redundant: a return where control returns anyway, which keeps the
     * BRA into race_chao_obey at 0x0cb84fd4; without it, the epilogue runs into
     * race_chao_obey, which starts 2 bytes early: 2 of 211 words differ here,
     * and 47 of 647 there */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* choose the mode that follows by the racer's gait. the choice reads the
 * work's flags, what the last race_chao_collide found, and the racer's
 * command. each test of a command takes it with racer_take_command, which
 * leaves COMMAND_NONE. with CHAO_MAY_LEAVE, the last of these that holds sets
 * the mode:
 *
 *   GAIT_FOOT   a play, swing, eat or dance command for its mode,
 *               COMMAND_HAPPY for MODE_GOAL, COMMAND_COMMENDATION for the
 *               place's mode,
 *               then CHAO_OVER_DROP for MODE_FLY, water above for
 *               MODE_SWIM, with a splash, and CHAO_WALL_AHEAD for MODE_CLIMB
 *   GAIT_SWIM   CHAO_WALL_AHEAD for MODE_CLIMB
 *   GAIT_FLY    CHAO_WALL_AHEAD for MODE_CLIMB, water above for MODE_SWIM,
 *               with a splash, and CHAO_ON_GROUND for MODE_RUN
 *   GAIT_WAIT   water above for MODE_SWIM, or else MODE_RUN
 *   GAIT_STILL  COMMAND_RACE or COMMAND_RETURN for MODE_RUN
 *
 * when none does, with CHAO_MAY_SETTLE, the first of these that holds sets
 * the mode. a Chao already in that mode keeps its submode and timer:
 *
 *   GAIT_FOOT   COMMAND_DEBUG for MODE_KOKE, then, but for a child,
 *               COMMAND_WALK or COMMAND_PUSH for its mode, or else MODE_RUN.
 *               a COMMAND_REST is taken first, and sets no mode
 *   GAIT_SWIM   MODE_SWIM
 *   GAIT_FLY    MODE_FLY
 *   GAIT_CLIMB  MODE_CLIMB
 *   GAIT_WAIT   water above for MODE_FLOAT, or else MODE_WAIT
 *   GAIT_STILL  a play, swing, eat or dance command for its mode,
 *               COMMAND_HAPPY for MODE_GOAL, or COMMAND_COMMENDATION for the
 *               place's mode, for a Chao in a place's mode already
 *
 * a place's mode is MODE_GOAL for the first, MODE_DANCE for the second and
 * third, MODE_FINISH_4 for the fourth and fifth, and MODE_FINISH_6 for the
 * rest. COMMAND_COMMENDATION on foot also stops the Chao along x and z */
static void race_chao_obey(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    int gait = racer->gait;
    int changed = 0;
    int unplaced;

    if (work->flags & CHAO_MAY_LEAVE) {
        switch (gait) {
        case GAIT_FOOT:
            if (racer_take_command(racer, COMMAND_PLAY_RAPPA)) {
                SET_MODE(work, MODE_PLAY_RAPPA);
                changed = 1;
            }
            if (racer_take_command(racer, COMMAND_PLAY_JYOURO)) {
                SET_MODE(work, MODE_PLAY_JYOURO);
                changed = 1;
            }
            if (racer_take_command(racer, COMMAND_PLAY_SCOOP)) {
                SET_MODE(work, MODE_PLAY_SCOOP);
                changed = 1;
            }
            if (racer_take_command(racer, COMMAND_SWING)) {
                SET_MODE(work, MODE_SWING);
                changed = 1;
            }
            if (racer_take_command(racer, COMMAND_EAT)) {
                SET_MODE(work, MODE_EAT);
                changed = 1;
            }
            if (racer_take_command(racer, COMMAND_DANCE)) {
                SET_MODE(work, MODE_DANCE);
                changed = 1;
            }
            if (racer_take_command(racer, COMMAND_HAPPY)) {
                SET_MODE(work, MODE_GOAL);
                changed = 1;
            }
            if (racer_take_command(racer, COMMAND_COMMENDATION)) {
                chao->speed.x = 0.0F;
                chao->speed.z = 0.0F;
                switch (racer->place) {
                case 0:
                    SET_MODE(work, MODE_GOAL);
                    break;
                case 1:
                case 2:
                    SET_MODE(work, MODE_DANCE);
                    break;
                case 3:
                case 4:
                    SET_MODE(work, MODE_FINISH_4);
                    break;
                default:
                    SET_MODE(work, MODE_FINISH_6);
                    break;
                }
                changed = 1;
            }
            if (work->flags & CHAO_OVER_DROP) {
                SET_MODE(work, MODE_FLY);
                changed = 1;
            }
            if (chao->terrain.flags & TERRAIN_WATER_ABOVE) {
                race_chao_splash(work, chao);
                SET_MODE(work, MODE_SWIM);
                changed = 1;
            }
            if (work->flags & CHAO_WALL_AHEAD) {
                SET_MODE(work, MODE_CLIMB);
                changed = 1;
            }
            break;
        case GAIT_SWIM:
            if (work->flags & CHAO_WALL_AHEAD) {
                SET_MODE(work, MODE_CLIMB);
                changed = 1;
            }
            break;
        case GAIT_FLY:
            if (work->flags & CHAO_WALL_AHEAD) {
                SET_MODE(work, MODE_CLIMB);
                changed = 1;
            }
            if (chao->terrain.flags & TERRAIN_WATER_ABOVE) {
                race_chao_splash(work, chao);
                SET_MODE(work, MODE_SWIM);
                changed = 1;
            }
            if (work->flags & CHAO_ON_GROUND) {
                SET_MODE(work, MODE_RUN);
                changed = 1;
            }
            break;
        case GAIT_WAIT:
            if (chao->terrain.flags & TERRAIN_WATER_ABOVE) {
                SET_MODE(work, MODE_SWIM);
                changed = 1;
            } else {
                SET_MODE(work, MODE_RUN);
                changed = 1;
            }
            break;
        case GAIT_STILL:
            if (racer_take_command(racer, COMMAND_RACE)) {
                SET_MODE(work, MODE_RUN);
                changed = 1;
                break;
            }
            if (racer_take_command(racer, COMMAND_RETURN)) {
                SET_MODE(work, MODE_RUN);
                changed = 1;
                break;
            }
            break;
        }
    }

    if (changed) {
        return;
    }
    if (work->flags & CHAO_MAY_SETTLE) {
        switch (gait) {
        case GAIT_FOOT:
            /* @hack redundant: the result of taking COMMAND_REST tested and
             * dropped, TST R0,R0 at 0x0cb85318; as a plain call, 16 of 647
             * words differ */
            if (racer_take_command(racer, COMMAND_REST)) {
            }
            if (racer_take_command(racer, COMMAND_DEBUG)) {
                if (work->mode != MODE_KOKE) {
                    SET_MODE(work, MODE_KOKE);
                }
                break;
            }
            if ((chao->record->type & 0x7f) != CHAO_CHILD) {
                if (racer_take_command(racer, COMMAND_WALK)) {
                    if (work->mode != MODE_WALK) {
                        SET_MODE(work, MODE_WALK);
                    }
                    break;
                }
                if (racer_take_command(racer, COMMAND_PUSH)) {
                    if (work->mode != MODE_PUSH) {
                        SET_MODE(work, MODE_PUSH);
                    }
                    break;
                }
            }
            if (work->mode != MODE_RUN) {
                SET_MODE(work, MODE_RUN);
            }
            break;
        case GAIT_SWIM:
            if (work->mode != MODE_SWIM) {
                SET_MODE(work, MODE_SWIM);
            }
            break;
        case GAIT_FLY:
            if (work->mode != MODE_FLY) {
                SET_MODE(work, MODE_FLY);
            }
            break;
        case GAIT_CLIMB:
            if (work->mode != MODE_CLIMB) {
                SET_MODE(work, MODE_CLIMB);
            }
            break;
        case GAIT_WAIT:
            if (chao->terrain.flags & TERRAIN_WATER_ABOVE) {
                if (work->mode != MODE_FLOAT) {
                    SET_MODE(work, MODE_FLOAT);
                }
            } else if (work->mode != MODE_WAIT) {
                SET_MODE(work, MODE_WAIT);
            }
            break;
        case GAIT_STILL:
            if (racer_take_command(racer, COMMAND_PLAY_RAPPA)) {
                if (work->mode != MODE_PLAY_RAPPA) {
                    SET_MODE(work, MODE_PLAY_RAPPA);
                }
            } else if (racer_take_command(racer, COMMAND_PLAY_JYOURO)) {
                if (work->mode != MODE_PLAY_JYOURO) {
                    SET_MODE(work, MODE_PLAY_JYOURO);
                }
            } else if (racer_take_command(racer, COMMAND_PLAY_SCOOP)) {
                if (work->mode != MODE_PLAY_SCOOP) {
                    SET_MODE(work, MODE_PLAY_SCOOP);
                }
            } else if (racer_take_command(racer, COMMAND_SWING)) {
                if (work->mode != MODE_SWING) {
                    SET_MODE(work, MODE_SWING);
                }
            } else if (racer_take_command(racer, COMMAND_EAT)) {
                if (work->mode != MODE_EAT) {
                    SET_MODE(work, MODE_EAT);
                }
            } else if (racer_take_command(racer, COMMAND_HAPPY)) {
                if (work->mode != MODE_GOAL) {
                    SET_MODE(work, MODE_GOAL);
                }
            } else if (racer_take_command(racer, COMMAND_DANCE)) {
                if (work->mode != MODE_DANCE) {
                    SET_MODE(work, MODE_DANCE);
                }
            } else if (racer_take_command(racer, COMMAND_COMMENDATION)) {
                unplaced = 0;
                switch (work->mode) {
                case MODE_GOAL:
                case MODE_DANCE:
                case MODE_FINISH_4:
                case MODE_FINISH_6:
                    break;
                default:
                    unplaced = 1;
                    break;
                }
                if (!unplaced) {
                    switch (racer->place) {
                    case 0:
                        if (work->mode != MODE_GOAL) {
                            SET_MODE(work, MODE_GOAL);
                        }
                        break;
                    case 1:
                    case 2:
                        if (work->mode != MODE_DANCE) {
                            SET_MODE(work, MODE_DANCE);
                        }
                        break;
                    case 3:
                    case 4:
                        if (work->mode != MODE_FINISH_4) {
                            SET_MODE(work, MODE_FINISH_4);
                        }
                        break;
                    default:
                        if (work->mode != MODE_FINISH_6) {
                            SET_MODE(work, MODE_FINISH_6);
                        }
                        break;
                    }
                }
            }
            break;
        }
    }
}

/* find the ground, water and walls around the Chao from query_height above
 * its pos. the work's flags and the racer's overhang and rise take what it
 * finds. the Chao stands on the ground below that point where the ground is
 * above its feet and no water lies below. failing that, it stands on the
 * surface above the point. that surface must face up, lie less than 1.5
 * above the point and have no water above. where no water is found, a Chao
 * below -0.25 is put at -0.25. the call then records water above it, at
 * level 0. a wall within 1.5, or 3 while the Chao climbs, pushes it out to
 * 1.35 from the wall. a climber turns a quarter of the way to face the walls.
 * then, while its racer races, three stretches of the course hold its z
 * within bounds */
static void race_chao_collide(struct task_work *work, struct alife_chao *chao)
{
    struct sides_query *query = &chao->terrain.query;
    struct racer *racer = RACER_VARS(work)->racer;
    /* @hack spelling: normal reached through its address, which SHC forms from
     * R15 at each access (0x0cb85768, 0x0cb8577a, 0x0cb85784); as normal.x, SHC
     * addresses normal off R15 itself, and 311 of 653 words differ */
    NJS_VECTOR normal;
    Float floor;
    Float above;
    Float reach;
    Angle ang;

    query->pos = work->pos;
    query->pos.y += chao->terrain.query_height;
    alife_find_sides(&chao->terrain);
    work->flags &= ~CHAO_ON_GROUND;
    racer->overhang &= ~1;
    floor = query->sides[SIDE_BELOW].level;
    above = query->sides[SIDE_ABOVE].level;
    if (floor > work->pos.y && !(chao->terrain.flags & TERRAIN_WATER_BELOW)) {
        work->pos.y = floor;
        chao->speed.y = 0.0F;
        work->flags |= CHAO_ON_GROUND;
    } else if (query->sides[SIDE_ABOVE].normal.y > 0.0F &&
               query->pos.y + 1.5F > above &&
               !(chao->terrain.flags & TERRAIN_WATER_ABOVE)) {
        work->pos.y = above;
        chao->speed.y = 0.0F;
        work->flags |= CHAO_ON_GROUND;
    }
    if (!(chao->terrain.flags & TERRAIN_WATER_ABOVE) &&
        !(chao->terrain.flags & TERRAIN_WATER_BELOW) && work->pos.y < -0.25F) {
        work->pos.y = -0.25F;
        chao->terrain.flags |= TERRAIN_WATER_ABOVE;
        chao->terrain.query.sides[SIDE_ABOVE].level = 0.0F;
    }
    work->flags &= ~CHAO_AT_WALL;
    work->flags &= ~CHAO_WALL_AHEAD;
    (&normal)->x = 0.0F;
    (&normal)->y = 0.0F;
    (&normal)->z = 0.0F;
    if (work->mode == MODE_CLIMB) {
        reach = 3.0F;
        if (work->pos.y + reach > query->sides[SIDE_ABOVE].level) {
            if (-njSin(OVERHANG_ANGLE) > query->sides[SIDE_ABOVE].normal.y) {
                racer->overhang |= 1;
            }
        }
    } else {
        reach = 1.5F;
    }
    if (query->sides[SIDE_MINUS_X].level > work->pos.x - reach) {
        if (query->sides[SIDE_MINUS_X].normal.x > 0.0F) {
            work->pos.x = query->sides[SIDE_MINUS_X].level + 1.35F;
        } else {
            work->pos.x = query->sides[SIDE_MINUS_X].level - 1.35F;
        }
        if (query->sides[SIDE_MINUS_X].normal.y < -njSin(OVERHANG_ANGLE)) {
            racer->overhang |= 1;
        }
        work->flags |= CHAO_AT_WALL;
        (&normal)->x += query->sides[SIDE_MINUS_X].normal.x;
        (&normal)->y += query->sides[SIDE_MINUS_X].normal.y;
        (&normal)->z += query->sides[SIDE_MINUS_X].normal.z;
        if (chao->target.x - work->pos.x < 0.0F) {
            work->flags |= CHAO_WALL_AHEAD;
        }
    }
    if (query->sides[SIDE_PLUS_X].level < work->pos.x + reach) {
        if (query->sides[SIDE_PLUS_X].normal.x < 0.0F) {
            work->pos.x = query->sides[SIDE_PLUS_X].level - 1.35F;
        } else {
            work->pos.x = query->sides[SIDE_PLUS_X].level + 1.35F;
        }
        if (query->sides[SIDE_PLUS_X].normal.y < -njSin(OVERHANG_ANGLE)) {
            racer->overhang |= 1;
        }
        work->flags |= CHAO_AT_WALL;
        (&normal)->x += query->sides[SIDE_PLUS_X].normal.x;
        (&normal)->y += query->sides[SIDE_PLUS_X].normal.y;
        (&normal)->z += query->sides[SIDE_PLUS_X].normal.z;
        if (chao->target.x - work->pos.x > 0.0F) {
            work->flags |= CHAO_WALL_AHEAD;
        }
    }
    if (work->pos.z - reach < query->sides[SIDE_MINUS_Z].level) {
        if (query->sides[SIDE_MINUS_Z].normal.z > 0.0F) {
            work->pos.z = query->sides[SIDE_MINUS_Z].level + 1.35F;
        } else {
            work->pos.z = query->sides[SIDE_MINUS_Z].level - 1.35F;
        }
        if (query->sides[SIDE_MINUS_Z].normal.y < -njSin(OVERHANG_ANGLE)) {
            racer->overhang |= 1;
        }
        work->flags |= CHAO_AT_WALL;
        (&normal)->x += query->sides[SIDE_MINUS_Z].normal.x;
        (&normal)->y += query->sides[SIDE_MINUS_Z].normal.y;
        (&normal)->z += query->sides[SIDE_MINUS_Z].normal.z;
        if (chao->target.z - work->pos.z < 0.0F) {
            work->flags |= CHAO_WALL_AHEAD;
        }
    }
    if (work->pos.z + reach > query->sides[SIDE_PLUS_Z].level) {
        if (query->sides[SIDE_PLUS_Z].normal.z < 0.0F) {
            work->pos.z = query->sides[SIDE_PLUS_Z].level - 1.35F;
        } else {
            work->pos.z = query->sides[SIDE_PLUS_Z].level + 1.35F;
        }
        if (query->sides[SIDE_PLUS_Z].normal.y < -njSin(OVERHANG_ANGLE)) {
            racer->overhang |= 1;
        }
        work->flags |= CHAO_AT_WALL;
        (&normal)->x += query->sides[SIDE_PLUS_Z].normal.x;
        (&normal)->y += query->sides[SIDE_PLUS_Z].normal.y;
        (&normal)->z += query->sides[SIDE_PLUS_Z].normal.z;
        if (chao->target.z - work->pos.z > 0.0F) {
            work->flags |= CHAO_WALL_AHEAD;
        }
    }
    if (work->mode == MODE_CLIMB && ACROSS_SQUARED(&normal) > 0.1F * 0.1F) {
        /* ang is the heading that faces into the walls, as TARGET_HEADING
         * forms one. the Chao turns a quarter of the way to it, the short way
         * round: the difference's low 16 bits, sign extended */
        ang = (Angle)(atan2f(-normal.x, -normal.z) * ANGLE_PER_RADIAN) -
              NJM_DEG_ANG(90);
        /* @hack spelling: the sign extension written out, which tests bit 15
         * (TST R4,R2 at 0x0cb85b52); as a (Sint16) cast, SHC writes EXTS.W, and
         * 155 of 653 words and 64 of 184 other words differ */
        work->ang.y += (Angle)(((ang - work->ang.y) & 0x8000)
                                   ? ((ang - work->ang.y) | 0xffff0000)
                                   : ((ang - work->ang.y) & 0xffff)) >>
                       2;
    }
    if (!(work->flags & CHAO_WALL_AHEAD) && chao->last_pos.y - 40.0F > floor) {
        work->flags |= CHAO_OVER_DROP;
    } else {
        work->flags &= ~CHAO_OVER_DROP;
    }
    racer->rise = work->pos.y - chao->last_pos.y;
    if (racer_is_active(racer)) {
        if (racer->distance < 360.0F) {
            if (work->pos.z < -99.3F) {
                work->pos.z = -99.3F;
            } else if (work->pos.z > 38.0F) {
                work->pos.z = 38.0F;
            }
        } else if (racer->distance > 1540.0F && racer->distance < 2000.0F) {
            if (work->pos.z < 100.3F) {
                work->pos.z = 100.3F;
            }
        } else if (racer->distance > 2350.0F && work->mode == MODE_FLY) {
            if (work->pos.z < 190.0F) {
                work->pos.z = 190.0F;
            } else if (work->pos.z > 400.0F) {
                work->pos.z = 400.0F;
            }
        }
    }
}

struct alife_chao *race_chao_init(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct racer *racer;
    int chance;

    SET_MODE(work, MODE_WAIT);
    collision_init(task, race_chao_shapes, COLLISION_ENEMY, 1, 1);
    work->collision->flags &= ~COLLISION_TARGET;
    if (work->ang.z) {
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 12;
    }
    work->tick = 0;
    work->timer = 0;
    alife_motion_init(work, chao);
    alife_terrain_init(work, chao);
    fun_0c90d634(work, chao);
    alife_icon_clear(work, chao);
    alife_face_init(work, chao);
    alife_shape_init(work, chao);
    alife_shape_blend(work, chao);
    alife_shape_set_parts(work, chao);
    alife_shape_set_wins(work, chao);
    alife_set_home(work, chao);
    race_chao_face_target(work, chao);
    racer = RACER_VARS(work)->racer;
    racer_attach(racer, work, chao);
    chance = (int)RANDOM(0.0F, 100.0F);
    if (chao->record->curio + chance < 70 &&
        (chao->record->skills.swim * 0.01F) + chance < 100.0F) {
        racer->flags |= RACER_SKIPS_FIRST_PATH;
    }
    chance = (int)RANDOM(0.0F, 100.0F);
    if ((chao->record->type & 0x7f) == CHAO_CHILD ||
        chance + chao->record->curio > 75) {
        racer->flags |= RACER_CURIOUS;
    }
    race_chao_clear_gait(work, chao);
    return chao;
}

void race_chao_step(struct task *task)
{
    struct task_work *work;
    struct alife_chao *chao;
    struct racer *racer;

    /* @hack redundant: a call of stage_get_stage_act whose result nothing
     * reads, which the image makes (JSR at 0x0cb85e76). the call only reads the
     * stage and act, so it changes nothing; without it, 102 of 104 words
     * differ */
    stage_get_stage_act();
    work = task->work;
    chao = (struct alife_chao *)task->motion;
    if (edit_mode == 0) {
        chao->unk_080 = 0;
        chao->spin = 0;
        racer = RACER_VARS(work)->racer;
        racer->flags &= ~RACER_HELD;
        racer->flags &= ~RACER_CLIMBING;
        alife_face_step(work, chao);
        race_chao_add_gains(work, chao);
        race_chao_move(work, chao);
        race_chao_integrate(work, chao);
        race_chao_collide(work, chao);
        race_chao_lean(work, chao);
        race_chao_set_in_zone(work, chao);
        race_chao_voice(work, chao);
        race_chao_loop_sound(work, chao);
        alife_motion_step(work, chao);
        race_chao_show(work, chao);
        alife_save_pos(work, chao);
        ++work->timer;
        ++work->tick;
        /* past 0x7fff the timer goes back to 0x1000, which is still past
         * every limit the modes test it against */
        if (TASK_TIMER(work) > 0x7fff) {
            work->timer = 0x1000;
        }
        collision_register(work);
        task_step_children(task);
    }
}

void race_chao_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    race_chao_show(work, chao);
}

void race_chao_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    racer_detach(RACER_VARS(work)->racer);
    fun_0c9002ac(work, chao);
}

void race_chao_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    SET_MODE(work, MODE_WAIT);
    VECTOR_CLEAR(&chao->speed);
    VECTOR_CLEAR(&chao->accel);
    race_chao_face_target(work, chao);
    if (work->ang.z) {
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 12;
    }
    work->tick = 0;
    work->timer = 0;
}

/* the value for skill while it is below 9999: low at a skill of 0, rising
 * to high at 10000, and at most 1.25. a skill of 9999 or more gives 0.9
 * instead, or 0.75 while the Chao climbs or is between 1250 and 1340 along
 * the course. the first call after a cheer gives that value too, whatever
 * skill is, and clears RACER_CHEERED */
static Float race_chao_take_skill_value(struct racer *racer, Float skill,
                                        Float low, Float high)
{
    Float most = 1.25F;

    if (racer->flags & RACER_CHEERED) {
        racer->flags &= ~RACER_CHEERED;
    } else if (skill < 9999.0F) {
        high = (high - low) * 0.0001F;
        skill = (high * skill) + low;
        return skill < most ? skill : most;
    }
    if (racer->work->mode == MODE_CLIMB ||
        (racer->distance > 1250.0F && racer->distance < 1340.0F)) {
        return 0.75F;
    }
    return 0.9F;
}

/* throw a splash up at the Chao's falling speed, at most 3, and make two
 * ripples at the water's level. SE_SPLASH plays while the race follows
 * the lane */
static void race_chao_splash(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    NJS_VECTOR speed;
    Float level = race_chao_water_level(work, chao);

    speed.x = 0.0F;
    speed.y = -chao->speed.y;
    if (speed.y > 3.0F) {
        speed.y = 3.0F;
    }
    speed.z = 0.0F;
    splash_create(&work->pos, &speed, 0.2F);
    race_chao_ripple(work, chao, level);
    race_chao_ripple(work, chao, level);
    if (racer_is_followed(racer)) {
        sound_play(SE_SPLASH, 0, 0, 0);
    }
}

/* throw the racer's spray from the Chao's pos, back at 1.25 times its speed
 * and up at 0.5 */
static void race_chao_wake(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    NJS_VECTOR speed;

    speed.x = -chao->speed.x * 1.25F;
    speed.y = 0.5F;
    speed.z = -chao->speed.z * 1.25F;
    racer_spray(racer, &work->pos, &speed);
}

/* make a ripple at height level, at the Chao's x and z plus up to 0.05 */
static void race_chao_ripple(struct task_work *work, struct alife_chao *chao,
                             Float level)
{
    NJS_POINT3 pos;
    NJS_VECTOR growth;

    pos.x = work->pos.x + RANDOM(0.0F, 0.05F);
    pos.y = level;
    pos.z = work->pos.z + RANDOM(0.0F, 0.05F);
    growth.x = 0.02F;
    growth.y = 0.0F;
    growth.z = 0.6F;
    ripple_create(&pos, &growth, 0.5F);
}

/* the height of the water above the Chao, or else below it, or NO_WATER
 * with none */
static Float race_chao_water_level(struct task_work *work,
                                   struct alife_chao *chao)
{
    Float level;

    if (chao->terrain.flags & TERRAIN_WATER_ABOVE) {
        level = chao->terrain.query.sides[SIDE_ABOVE].level;
    } else if (chao->terrain.flags & TERRAIN_WATER_BELOW) {
        level = chao->terrain.query.sides[SIDE_BELOW].level;
    } else {
        level = NO_WATER;
    }
    return level;
}

/* answer the racer's voice request, 1 to 17, and clear it. four times in
 * ten the Chao gives any of its VOICES voices, heard near its pos. otherwise,
 * while the race follows the Chao's lane, it gives the request's voice for
 * its mood. the mood is random. the record's kind, agres, unk_66, pleasure
 * and anger move it */
static void race_chao_voice(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    struct chao *record;
    int mood;
    int se;
    int index;

    /* @bug uninitialized: tests record before anything sets it, so the return
     * for a null record never runs. SHC reads record as R15, the stack pointer,
     * which is never zero: TST R15,R15 at 0x0cb86336 */
    if (!record) { /* NOLINT(clang-diagnostic-uninitialized) */
        return;
    }
    if (racer->voice == 0) {
        return;
    }
    if (racer->voice >= 18) {
        return;
    }
    mood = (int)RANDOM(-500.0F, 500.0F);
    if (mood > 100) {
        se = (int)RANDOM(FIRST_VOICE, FIRST_VOICE + VOICES);
        sound_play_near(se, &work->pos, &race_chao_falloff);
        racer->voice = 0;
        return;
    }
    if (!racer_is_followed(racer)) {
        racer->voice = 0;
        return;
    }
    record = chao->record;
    mood += record->kind;
    mood -= record->agres;
    mood *= 5;
    mood -= record->unk_66 - 100;
    mood += record->pleasure - 100;
    mood -= record->anger - 100;
    switch (racer->voice) {
    case 1:
    case 2:
    case 3:
        if (racer->voice == 2) {
            mood += 200;
        } else if (racer->voice == 1) {
            mood -= 200;
        }
        mood += 4000;
        mood = (int)(mood * 0.002F);
        /* @bug memory: the clamp lets 13 through, CMP/GT against 13
         * at 0x0cb8648e, and race_chao_moods_1[13] would read past the
         * table (MOV.B @(R0,R14),R4 at 0x0cb86510). by the record's fields'
         * ranges, the mood comes to 12 at most here, so the read never
         * runs */
        if (mood > 13) {
            mood = 12;
        } else if (mood < 0) {
            mood = 0;
        }
        se = race_chao_moods_1[mood];
        break;
    case 4:
    case 10:
    case 11:
        if (racer->voice == 10) {
            mood -= 200;
        } else if (racer->voice == 11) {
            mood -= 100;
        } else {
            mood += 700;
        }
        mood += 4000;
        mood = (int)(mood * 0.002F);
        /* @bug memory: the clamp lets 12 through, CMP/GT against 12
         * at 0x0cb864fe, and race_chao_moods_4[12] reads the byte after the
         * table (MOV.B @(R0,R14),R4 at 0x0cb86510): race_chao_voices_17's
         * first voice, 22 */
        if (mood > 12) {
            mood = 11;
        } else if (mood < 0) {
            mood = 0;
        }
        se = race_chao_moods_4[mood];
        break;
    case 5:
        if (mood > 700) {
            se = 54;
        } else if (mood > -400) {
            se = 3;
        } else if (mood > -700) {
            se = 18;
        } else {
            se = 70;
        }
        break;
    case 6:
        if (mood > 0) {
            se = 45;
        } else {
            se = 13;
        }
        break;
    case 7:
    case 8:
        if (mood > 0) {
            se = 60;
        } else {
            se = 59;
        }
        break;
    case 9:
        if (mood > 700) {
            se = 17;
        } else if (mood > -700) {
            se = 2;
        } else {
            se = 15;
        }
        break;
    case 12:
    case 13:
        if (racer->voice == 12) {
            mood += 600;
        }
        if (mood > 700) {
            se = 26;
        } else if (mood > -700) {
            se = 25;
        } else {
            se = 78;
        }
        break;
    case 14:
        if (mood > 0) {
            se = 80;
        } else {
            se = 83;
        }
        break;
    case 16:
        if (mood > 1000) {
            se = 0;
        } else if (mood > 500) {
            se = 1;
        } else if (mood > -700) {
            se = 11;
        } else {
            se = 44;
        }
        break;
    case 15:
        se = (int)RANDOM(0.0F, 2.0F);
        if (se == 0) {
            se = 86;
        } else {
            se = 73;
        }
        break;
    case 17:
        index = (int)RANDOM(0.0F, 7.0F);
        se = race_chao_voices_17[index];
        break;
    default:
        return;
    }
    se += FIRST_VOICE;
    sound_play_near(se, &work->pos, &race_chao_falloff);
    racer->voice = 0;
}

/* play se while the race follows the Chao's lane */
static void race_chao_sound(struct task_work *work, int se)
{
    struct racer *racer = RACER_VARS(work)->racer;

    if (racer_is_followed(racer)) {
        sound_play(se, 0, 0, 0);
    }
}

/* keep the sound of the Chao's mode playing while the race follows its
 * lane. sound_play_for refreshes it in MODE_FLY, MODE_SWIM, MODE_PUSH and
 * MODE_EAT. its id is the mode times 8 plus the lane */
static void race_chao_loop_sound(struct task_work *work,
                                 struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    int se;

    if (racer_is_followed(racer)) {
        switch (work->mode) {
        case MODE_FLY:
            se = 0x267;
            break;
        case MODE_SWIM:
            se = 0x26f;
            break;
        case MODE_PUSH:
            se = 0x271;
            break;
        case MODE_EAT:
            se = 0x26c;
            break;
        default:
            return;
        }
        sound_play_for(se, (work->mode * 8) + racer->lane, 1, 0, 2);
    }
}

int race_chao_node_pos(NJS_POINT3 *pos, struct task_work *work,
                       struct alife_chao *chao, int node)
{
    struct chao_motion *motion = &chao->motion;
    NJS_ACTION action;
    NJS_MATRIX matrix;
    NJS_POINT3 now;
    NJS_POINT3 next;
    Float ratio = motion->ratio;
    Float rest = 1.0F - ratio;

    action.object = chao->shape.model;
    action.motion = motion->table[motion->current].motion;
    if (pose_action(&_unit_matrix_, &action, motion->frame, node, &matrix) !=
        TRUE) {
        return 0;
    }
    njCalcPoint(&matrix, (NJS_POINT3 *)&race_origin, &now);
    action.motion = motion->table[motion->next].motion;
    if (pose_action(&_unit_matrix_, &action, motion->next_frame, node,
                    &matrix) != TRUE) {
        return 0;
    }
    njCalcPoint(&matrix, (NJS_POINT3 *)&race_origin, &next);
    pos->x = (now.x * rest) + (next.x * ratio);
    pos->y = (now.y * rest) + (next.y * ratio);
    pos->z = (now.z * rest) + (next.z * ratio);
    return 1;
}
