/* a Chao in the race: the modes it moves in, its heading and lean, and how
 * it's drawn */
#ifndef AL_RACE_RACE_CHAO_H
#define AL_RACE_RACE_CHAO_H
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "system/task.h"

/* a racing Chao's modes, in its work's mode. the names up to MODE_DROWN are
 * the labels of the race's debug page, but for the three pauses. the page's
 * labels skip 5 to 7 and give 13 to 15 one PLAY, so any mode from 5 on shows
 * the wrong label */
enum race_chao_mode {
    MODE_WAIT = 0, /* at the start */
    MODE_FLOAT = 1, /* at the start, in water */
    MODE_RUN = 2,
    MODE_WALK = 3,
    MODE_KOKE = 4, /* a tumble */
    /* the pauses running starts, each race_chao_pause with its own motion:
     * 78 once the record's sleep passes 199, and 63 or 84 once its unk_65
     * does. a Chao whose agres is above 0 takes 63 */
    MODE_PAUSE_SLEEP = 5,
    MODE_PAUSE_AGRES = 6,
    MODE_PAUSE_MILD = 7,
    MODE_FLY = 8,
    MODE_SWIM = 9,
    MODE_CLIMB = 10,
    MODE_PUSH = 11,
    MODE_GOAL = 12,
    MODE_PLAY_RAPPA = 13, /* with the bugle */
    MODE_PLAY_JYOURO = 14, /* with the watering can */
    MODE_PLAY_SCOOP = 15,
    MODE_SWING = 16,
    MODE_EAT = 17,
    MODE_REST = 18,
    MODE_DROWN = 19,
    MODE_DANCE = 20,
    MODE_FINISH_4 = 21, /* fourth or fifth at the finish */
    MODE_FINISH_6 = 22 /* sixth or below */
};

/* in a racing Chao's work flags. each step clears CHAO_MAY_SETTLE,
 * CHAO_MAY_LEAVE and CHAO_TILT before the mode runs, and the mode may set them.
 * race_chao_obey reads the first two after it. race_chao_collide sets or
 * clears CHAO_AT_WALL, CHAO_OVER_DROP and CHAO_WALL_AHEAD each step */
#define CHAO_AT_WALL 0x100 /* a wall pushes it back */
/* the ground is over 40 below its last pos, and no wall stands ahead */
#define CHAO_OVER_DROP 0x200
#define CHAO_MAY_SETTLE 0x1000 /* it may take another mode of its gait */
#define CHAO_MAY_LEAVE 0x2000 /* it may take another gait's mode */
#define CHAO_WALL_AHEAD 0x4000 /* a wall stands between it and its target */
#define CHAO_TILT 0x8000 /* it leans with the ground under it */

#define SET_MODE(work, to)                                                     \
    do {                                                                       \
        (work)->mode = (to);                                                   \
        (work)->submode = 0;                                                   \
        (work)->timer = 0;                                                     \
    } while (0)

/* a racing Chao's task. its motion is the Chao, and its work must keep its
 * racer in its vars before the task first runs */

/* set task's Chao up in MODE_WAIT, give it to its racer and return it. the
 * racer takes RACER_CURIOUS for a child, and else at random, the likelier the
 * higher the Chao's curio. it takes RACER_SKIPS_FIRST_PATH at random, the
 * likelier the lower the Chao's curio and swim skill */
struct alife_chao *race_chao_init(struct task *task);
/* while edit_mode is clear, run the Chao's mode, move and draw it, and step
 * task's children */
void race_chao_step(struct task *task);
/* detach the Chao's racer and run fun_0c9002ac's cleanup */
void race_chao_destroy(struct task *task);
/* draw task's Chao as its step does, by race_chao_show */
void race_chao_draw(struct task *task);
/* put task's Chao back in MODE_WAIT, with its submode, tick and timer
 * cleared, at rest and facing its target. the Chao takes motion 12 when
 * its work's ang.z is set */
void race_chao_reset(struct task *task);

/* turn the work about y to face the Chao's target */
void race_chao_face_target(struct task_work *work, struct alife_chao *chao);
/* store in angle how far task's Chao heads past the heading to its target,
 * from -0x8000 to 0x7fff, and return 1. 0, and angle unchanged, for a null
 * task */
int race_chao_angle_to_target(struct task *task, Angle *angle);
/* set work's racer's gait to GAIT_FOOT, and drop its voice request */
void race_chao_clear_gait(struct task_work *work, struct alife_chao *chao);
/* while CHAO_TILT is set and there's ground below, lean toward its slope
 * by 2 degrees a step about x and z. otherwise level up by 2 degrees a step
 * about x and 1 about z */
void race_chao_lean(struct task_work *work, struct alife_chao *chao);

/* set blending by source alpha, then draw the Chao and its shadow while its
 * racer is RACER_SHOWN. while RACER_NAMED, draw its name 6.5 above it and
 * clear the flag. the name is blue for an ENTRY_RIVAL entry's Chao, and
 * else black */
void race_chao_show(struct task_work *work, struct alife_chao *chao);

/* store in pos where node of the Chao's model lies in the model's space,
 * and return 1. the place is blended from the Chao's current motion toward
 * its next by the motion's ratio. node must be below 128 and in the model's
 * tree. a failed pose_action would return 0, but pose_action always returns
 * TRUE */
int race_chao_node_pos(NJS_POINT3 *pos, struct task_work *work,
                       struct alife_chao *chao, int node);

/* take motion 11 and slow to a stop, with eyes 2, mouth 1 and icon kind 5.
 * a speed along x or z below 0.01, a negative one included, is cut to 0 */
void race_chao_stop(struct task_work *work, struct alife_chao *chao);
/* take motion 8 at half speed, and from the next step keep eyes 4 */
void fun_0cb83ec0(struct task_work *work, struct alife_chao *chao);
/* take one of motions 32 to 35 at random, at four times speed, and another
 * 23 to 52 steps later, slowing to a stop */
void race_chao_dance_moves(struct task_work *work, struct alife_chao *chao);

#endif
