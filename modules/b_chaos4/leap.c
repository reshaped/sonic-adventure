/* Chaos 4's leap. he springs up out of the swamp, and a column of water
 * bursts up halfway to player 0 and runs on that way. more columns burst
 * up round him until he falls back in */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/column.h"
#include "b_chaos4/leap.h"
#include "b_chaos4/swamp.h"
#include "chaos/chaos.h"
#include "draw/motion.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

enum leap_mode { LEAP_START, LEAP_SPRING, LEAP_RISE, LEAP_END };

#define SPRING_FRAME 9.1F /* the frame of ACTION_LEAP he springs at */
#define SPRING_SPEED 2.68F /* his speed's y as he springs */
#define SE_LEAP 0x175 /* the sound of his spring */

/* each step from his spring until he falls back in, a column bursts up
 * round him by a chance of COLUMN_CHANCE, LEAP_COLUMNS at most. each bursts
 * within COLUMN_SPREAD of him in x and z, and runs away from him at
 * COLUMN_SPEED. the column halfway to player 0 runs at FIRST_SPEED */
#define LEAP_COLUMNS 4
#define COLUMN_CHANCE 0.2F
#define COLUMN_SPREAD 10.0F
#define FIRST_SPEED 4.0F
#define COLUMN_SPEED 2.0F

/* the columns left to burst up round him in this leap */
static Sint32 leap_columns;

void chaos4_leap(struct task_work *work, struct task_motion *motion,
                 struct chaos *chaos)
{
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Float length;

    switch (work->submode) {
    case LEAP_START:
        chaos4_set_shapes(work, SHAPES_ON);
        chaos->motion.wanted = ACTION_LEAP;
        record->move = MOVE_STILL;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->submode = LEAP_SPRING;
    case LEAP_SPRING:
        if (chaos->motion.current == ACTION_LEAP &&
            chaos->motion.frame >= SPRING_FRAME) {
            sound_play(SE_LEAP, 0, 0, 0);
            swamp_ripple_add(&work->pos);
            record->move = MOVE_FALLS | MOVE_DRIFT;
            record->speed.y = SPRING_SPEED;
            /* the first column bursts up halfway to player 0, and runs on
             * that way */
            VECTOR_SUM(&pos, &work->pos, &player_work[0]->pos);
            VECTOR_SCALE(&pos, &pos, 0.5F);
            pos.y = SWAMP_Y;
            VECTOR_DIFF(&speed, &pos, &work->pos);
            speed.y = 0.0F;
            if ((length = njSqrt(SQUARE(speed.x) + SQUARE(speed.z))) > 0.0F) {
                VECTOR_SCALE(&speed, &speed, FIRST_SPEED / length);
            } else {
                /* @hack spelling: speed zeroed through its address, each store
                 * through a fresh copy of R15 plus 16 (0x0c903c20 to
                 * 0x0c903c34); as members, 169 of 449 words differ */
                (&speed)->x = (&speed)->y = (&speed)->z = 0.0F;
            }
            column_create(&pos, &speed);
            leap_columns = LEAP_COLUMNS;
            work->submode = LEAP_RISE;
        }
        break;
    case LEAP_RISE:
        if (leap_columns > 0 && rand() * RAND_SCALE <= COLUMN_CHANCE) {
            VECTOR_COPY(&pos, &work->pos);
            pos.y = SWAMP_Y;
            pos.x += RANDOM(-COLUMN_SPREAD, COLUMN_SPREAD);
            pos.z += RANDOM(-COLUMN_SPREAD, COLUMN_SPREAD);
            VECTOR_DIFF(&speed, &pos, &work->pos);
            speed.y = 0.0F;
            if ((length = njSqrt(SQUARE(speed.x) + SQUARE(speed.z))) > 0.0F) {
                VECTOR_SCALE(&speed, &speed, COLUMN_SPEED / length);
            } else {
                /* @hack spelling: speed zeroed through its address, each store
                 * through a fresh copy of R15 plus 16 (0x0c903dc0 to
                 * 0x0c903dd4); as members, 80 of 449 words differ */
                (&speed)->x = (&speed)->y = (&speed)->z = 0.0F;
            }
            column_create(&pos, &speed);
            --leap_columns;
        }
        if (record->speed.y < 0.0F && work->pos.y < SWAMP_Y) {
            sound_play(SE_SPLASH, 0, 0, 0);
            swamp_ripple_add(&work->pos);
            work->submode = LEAP_END;
        }
        break;
    case LEAP_END:
        if (chaos->motion.current == ACTION_REST) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != BOSS_SWIM) {
            boss_next_mode = BOSS_SWIM;
            boss_last_mode = work->mode;
        }
        work->submode = LEAP_START;
    }
}
